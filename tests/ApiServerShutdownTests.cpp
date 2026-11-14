#include <arpa/inet.h>
#include <drogon/drogon.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>

#include "ApiServer.hpp"
#include "ConfigManager.hpp"
#include "Logger.hpp"
#include "SortingController.hpp"

class ApiServerShutdownTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::string directoryTemplate =
        (std::filesystem::temp_directory_path() / "blaze_shutdown_test_XXXXXX").string();
    char* directory = ::mkdtemp(directoryTemplate.data());
    ASSERT_NE(directory, nullptr);
    testDirectory = directory;

    logger = std::make_unique<Logger>(testDirectory / "test.log");
    configManager = std::make_unique<ConfigManager>(testDirectory / "config.json", *logger);
    ASSERT_TRUE(configManager->initialize());

    sortingController = std::make_unique<SortingController>(
        configManager->getConfig(), *logger,
        [this](const std::filesystem::path&, const std::filesystem::path&, ProgressCallback,
               CancellationCallback) {
          sortingStarted.store(true);
          while (holdSorting.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
          }
          return true;
        });

    server = std::make_unique<ApiServer>(*configManager, *sortingController);

    socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(socketFd, 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    ASSERT_EQ(::bind(socketFd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)), 0);
    socklen_t addressLength = sizeof(address);
    ASSERT_EQ(::getsockname(socketFd, reinterpret_cast<sockaddr*>(&address), &addressLength), 0);
    port = ntohs(address.sin_port);
    ::close(socketFd);
    socketFd = -1;

    serverThread = std::thread([this] { drogon::app().addListener("127.0.0.1", port).run(); });
  }

  void TearDown() override {
    holdSorting.store(false);
    if (socketFd >= 0) {
      ::close(socketFd);
    }
    if (serverThread.joinable()) {
      drogon::app().quit();
      serverThread.join();
    }
    server.reset();
    sortingController.reset();
    configManager.reset();
    logger.reset();
    if (!testDirectory.empty()) {
      std::filesystem::remove_all(testDirectory);
    }
  }

  std::filesystem::path testDirectory;
  std::unique_ptr<Logger> logger;
  std::unique_ptr<ConfigManager> configManager;
  std::unique_ptr<SortingController> sortingController;
  std::unique_ptr<ApiServer> server;
  std::thread serverThread;
  std::atomic_bool holdSorting{true};
  std::atomic_bool sortingStarted{false};
  uint16_t port = 0;
  int socketFd = -1;
};

TEST_F(ApiServerShutdownTest, ShutdownWithActiveProgressStream) {
  ASSERT_TRUE(sortingController->start());
  const auto startDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (!sortingStarted.load() && std::chrono::steady_clock::now() < startDeadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  ASSERT_TRUE(sortingStarted.load());

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(port);
  const auto connectDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (std::chrono::steady_clock::now() < connectDeadline) {
    socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(socketFd, 0);
    if (::connect(socketFd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0) {
      break;
    }
    ::close(socketFd);
    socketFd = -1;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  ASSERT_GE(socketFd, 0) << "Drogon did not accept a connection";

  timeval receiveTimeout{0, 250000};
  ASSERT_EQ(
      ::setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &receiveTimeout, sizeof(receiveTimeout)), 0);
  const std::string request =
      "GET /api/sorting/progress/stream HTTP/1.1\r\n"
      "Host: 127.0.0.1\r\n"
      "Connection: keep-alive\r\n\r\n";
  ASSERT_EQ(::send(socketFd, request.data(), request.size(), 0),
            static_cast<ssize_t>(request.size()));

  std::string response;
  const auto responseDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (response.find("data:") == std::string::npos &&
         std::chrono::steady_clock::now() < responseDeadline) {
    char buffer[4096];
    const auto received = ::recv(socketFd, buffer, sizeof(buffer), 0);
    if (received > 0) {
      response.append(buffer, static_cast<std::size_t>(received));
    } else if (received == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) {
      break;
    }
  }
  ASSERT_NE(response.find("HTTP/1.1 200"), std::string::npos);
  ASSERT_NE(response.find("data:"), std::string::npos);
  ASSERT_EQ(response.find("\r\n0\r\n\r\n"), std::string::npos);
  ASSERT_EQ(sortingController->getStatus(), SortingStatus::Running);

  drogon::app().quit();
  serverThread.join();
  server.reset();
  EXPECT_EQ(sortingController->getStatus(), SortingStatus::Running);
}
