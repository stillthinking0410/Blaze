#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <thread>

#include "ApiServer.hpp"
#include "ConfigManager.hpp"
#include "Logger.hpp"
#include "SortingController.hpp"

namespace {

std::atomic_bool shutdownRequested{false};

static_assert(std::atomic_bool::is_always_lock_free,
              "Termination signal handling requires lock-free atomic_bool.");

}  // namespace

int main() {
  Logger logger("../logs/blaze.log");

  ConfigManager configManager("../config/config.json", logger);

  if (!logger.initialize()) {
    std::cerr << "Failed to initialize Blaze logger\n";
    return 1;
  }

  if (!configManager.initialize()) {
    std::cerr << "Config initialization failed\n";
    return 1;
  }

  if (!configManager.validate()) {
    std::cerr << "Invalid configuration\n";
    return 1;
  }

  SortingController sortingController(configManager.getConfig(), logger);

  ApiServer apiServer(configManager, sortingController);

  constexpr std::uint16_t port = 8080;

  std::cout << "Blaze API server starting on http://127.0.0.1:" << port << '\n';

  std::atomic_bool serverReady{false};
  std::atomic_bool serverFinished{false};
  const auto requestShutdown = [] { shutdownRequested.store(true); };
  drogon::app().setIntSignalHandler(requestShutdown);
  drogon::app().setTermSignalHandler(requestShutdown);
  drogon::app().registerBeginningAdvice([&serverReady] { serverReady.store(true); });

  std::thread shutdownThread([&] {
    while (!serverFinished.load()) {
      if (shutdownRequested.load()) {
        while (!serverReady.load() && !serverFinished.load()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (serverReady.load()) {
          apiServer.shutdown();
        }
        return;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  apiServer.start("127.0.0.1", port);
  serverFinished.store(true);
  shutdownThread.join();

  return 0;
}
