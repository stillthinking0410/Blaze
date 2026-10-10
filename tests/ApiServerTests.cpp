#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>
#include <iostream>
#include <drogon/drogon.h>

#include "ApiServer.hpp"
#include "ConfigManager.hpp"
#include "Logger.hpp"
#include "SortingController.hpp"

class ApiServerTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        testDirectory =
            std::filesystem::temp_directory_path() /
            "blaze_api_test";

        std::filesystem::create_directories(
            testDirectory);

        logger = std::make_unique<Logger>(
            testDirectory / "test.log");

        configManager =
            std::make_unique<ConfigManager>(
                testDirectory / "config.json",
                *logger);

        ASSERT_TRUE(
            configManager->initialize());

        sortingController =
            std::make_unique<SortingController>(
                configManager->getConfig(),
                *logger,
                [](
                    const std::filesystem::path&,
                    const std::filesystem::path&,
                    ProgressCallback,
                    CancellationCallback cancellationCallback
                )
                {
                    ApiServerTest::sortingStarted.store(
                        true);

                    while (
                        ApiServerTest::holdSorting.load())
                    {
                        if (cancellationCallback &&
                            cancellationCallback())
                        {
                            return true;
                        }

                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(10));
                    }

                    return true;
                });

        server =
            std::make_unique<ApiServer>(
                *configManager,
                *sortingController);

        serverThread =
            std::thread(
                []
                {
                    drogon::app()
                        .addListener(
                            "127.0.0.1",
                            port)
                        .run();
                });

        std::this_thread::sleep_for(
            std::chrono::milliseconds(200));
    }

    static void TearDownTestSuite()
    {
        // Make sure a test does not leave
        // the fake sorting operation running.
        holdSorting.store(false);

        drogon::app().quit();

        if (serverThread.joinable())
        {
            serverThread.join();
        }

        server.reset();
        sortingController.reset();
        configManager.reset();
        logger.reset();

        std::filesystem::remove_all(
            testDirectory);
    }

    static constexpr uint16_t port = 18080;

    static inline std::filesystem::path
        testDirectory;

    static inline std::unique_ptr<Logger>
        logger;

    static inline std::unique_ptr<ConfigManager>
        configManager;

    static inline std::unique_ptr<SortingController>
        sortingController;

    static inline std::unique_ptr<ApiServer>
        server;

    static inline std::thread
        serverThread;

    static inline std::atomic<bool>
        holdSorting{false};

    static inline std::atomic<bool>
        sortingStarted{false};
};


TEST_F(
    ApiServerTest,
    GetConfigReturnsConfiguration)
{
    configManager->getConfig().sourceFolder =
        "/test/source";

    configManager->getConfig().destinationFolder =
        "/test/destination";

    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    auto request =
        drogon::HttpRequest::newHttpRequest();

    request->setMethod(drogon::Get);
    request->setPath("/api/config");

    auto result =
        client->sendRequest(request);

    ASSERT_EQ(
        result.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        result.second,
        nullptr);

    EXPECT_EQ(
        result.second->getStatusCode(),
        drogon::k200OK);

    const auto json =
        result.second->getJsonObject();

    ASSERT_NE(
        json,
        nullptr);

    EXPECT_EQ(
        (*json)["sourceFolder"].asString(),
        "/test/source");

    EXPECT_EQ(
        (*json)["destinationFolder"].asString(),
        "/test/destination");
}


TEST_F(
    ApiServerTest,
    PutConfigUpdatesConfiguration)
{
    const auto newSource =
        testDirectory / "new_source";

    const auto newDestination =
        testDirectory / "new_destination";

    std::filesystem::create_directories(
        newSource);

    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    auto request =
        drogon::HttpRequest::newHttpRequest();

    request->setMethod(drogon::Put);
    request->setPath("/api/config");

    Json::Value body;

    body["sourceFolder"] =
        newSource.string();

    body["destinationFolder"] =
        newDestination.string();

    request->setContentTypeCode(
        drogon::CT_APPLICATION_JSON);

    request->setBody(
        Json::writeString(
            Json::StreamWriterBuilder(),
            body));

    auto result =
        client->sendRequest(request);

    ASSERT_EQ(
        result.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        result.second,
        nullptr);

    EXPECT_EQ(
        result.second->getStatusCode(),
        drogon::k200OK);

    const auto json =
        result.second->getJsonObject();

    ASSERT_NE(
        json,
        nullptr);

    EXPECT_EQ(
        (*json)["status"].asString(),
        "updated");

    const Config& config =
        configManager->getConfig();

    EXPECT_EQ(
        config.sourceFolder,
        newSource);

    EXPECT_EQ(
        config.destinationFolder,
        newDestination);
}


TEST_F(
    ApiServerTest,
    PutConfigPersistsConfiguration)
{
    const auto newSource =
        testDirectory / "persistent_source";

    const auto newDestination =
        testDirectory / "persistent_destination";

    std::filesystem::create_directories(
        newSource);

    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    auto request =
        drogon::HttpRequest::newHttpRequest();

    request->setMethod(drogon::Put);
    request->setPath("/api/config");

    Json::Value body;

    body["sourceFolder"] =
        newSource.string();

    body["destinationFolder"] =
        newDestination.string();

    request->setContentTypeCode(
        drogon::CT_APPLICATION_JSON);

    request->setBody(
        Json::writeString(
            Json::StreamWriterBuilder(),
            body));

    auto result =
        client->sendRequest(request);

    ASSERT_EQ(
        result.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        result.second,
        nullptr);

    ASSERT_EQ(
        result.second->getStatusCode(),
        drogon::k200OK);

    Logger reloadedLogger(
        testDirectory / "reload.log");

    ConfigManager reloadedManager(
        testDirectory / "config.json",
        reloadedLogger);

    ASSERT_TRUE(
        reloadedManager.initialize());

    const Config& reloadedConfig =
        reloadedManager.getConfig();

    EXPECT_EQ(
        reloadedConfig.sourceFolder,
        newSource);

    EXPECT_EQ(
        reloadedConfig.destinationFolder,
        newDestination);
}


TEST_F(
    ApiServerTest,
    PutConfigRejectsMissingSourceFolder)
{
    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    auto request =
        drogon::HttpRequest::newHttpRequest();

    request->setMethod(drogon::Put);
    request->setPath("/api/config");

    Json::Value body;

    body["destinationFolder"] =
        (testDirectory / "destination").string();

    request->setContentTypeCode(
        drogon::CT_APPLICATION_JSON);

    request->setBody(
        Json::writeString(
            Json::StreamWriterBuilder(),
            body));

    auto result =
        client->sendRequest(request);

    ASSERT_EQ(
        result.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        result.second,
        nullptr);

    EXPECT_EQ(
        result.second->getStatusCode(),
        drogon::k400BadRequest);

    const auto json =
        result.second->getJsonObject();

    ASSERT_NE(
        json,
        nullptr);

    EXPECT_EQ(
        (*json)["error"].asString(),
        "sourceFolder and destinationFolder are required.");
}


TEST_F(
    ApiServerTest,
    PutConfigRejectsMissingDestinationFolder)
{
    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    auto request =
        drogon::HttpRequest::newHttpRequest();

    request->setMethod(drogon::Put);
    request->setPath("/api/config");

    Json::Value body;

    body["sourceFolder"] =
        (testDirectory / "source").string();

    request->setContentTypeCode(
        drogon::CT_APPLICATION_JSON);

    request->setBody(
        Json::writeString(
            Json::StreamWriterBuilder(),
            body));

    auto result =
        client->sendRequest(request);

    ASSERT_EQ(
        result.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        result.second,
        nullptr);

    EXPECT_EQ(
        result.second->getStatusCode(),
        drogon::k400BadRequest);

    const auto json =
        result.second->getJsonObject();

    ASSERT_NE(
        json,
        nullptr);

    EXPECT_EQ(
        (*json)["error"].asString(),
        "sourceFolder and destinationFolder are required.");
}


TEST_F(
    ApiServerTest,
    PutConfigRejectsNonexistentSourceFolder)
{
    const auto missingSource =
        testDirectory / "missing_source";

    const auto destination =
        testDirectory / "destination";

    ASSERT_FALSE(
        std::filesystem::exists(
            missingSource));

    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    auto request =
        drogon::HttpRequest::newHttpRequest();

    request->setMethod(drogon::Put);
    request->setPath("/api/config");

    Json::Value body;

    body["sourceFolder"] =
        missingSource.string();

    body["destinationFolder"] =
        destination.string();

    request->setContentTypeCode(
        drogon::CT_APPLICATION_JSON);

    request->setBody(
        Json::writeString(
            Json::StreamWriterBuilder(),
            body));

    auto result =
        client->sendRequest(request);

    ASSERT_EQ(
        result.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        result.second,
        nullptr);

    EXPECT_EQ(
        result.second->getStatusCode(),
        drogon::k400BadRequest);

    const auto json =
        result.second->getJsonObject();

    ASSERT_NE(
        json,
        nullptr);

    EXPECT_EQ(
        (*json)["error"].asString(),
        "Invalid configuration.");
}


TEST_F(
    ApiServerTest,
    StartSortingReturnsRunning)
{
    // Make sure this test does not inherit
    // the blocking state from another test.
    holdSorting.store(false);
    sortingStarted.store(false);

    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    auto request =
        drogon::HttpRequest::newHttpRequest();

    request->setMethod(drogon::Post);
    request->setPath("/api/sorting/start");

    auto result =
        client->sendRequest(request);

    ASSERT_EQ(
        result.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        result.second,
        nullptr);

    EXPECT_EQ(
        result.second->getStatusCode(),
        drogon::k200OK);

    const auto json =
        result.second->getJsonObject();

    ASSERT_NE(
        json,
        nullptr);

    EXPECT_EQ(
        (*json)["status"].asString(),
        "running");

    // The fake operation completes immediately.
    // Wait until the controller reaches Completed
    // before another test tries to start sorting.
    for (int i = 0; i < 100; ++i)
    {
        if (sortingController->getStatus() ==
            SortingStatus::Completed)
        {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }

    EXPECT_EQ(
        sortingController->getStatus(),
        SortingStatus::Completed);
}


TEST_F(
    ApiServerTest,
    StartSortingReturnsConflictWhenAlreadyRunning)
{
    holdSorting.store(true);
    sortingStarted.store(false);

    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    auto request =
        drogon::HttpRequest::newHttpRequest();

    request->setMethod(drogon::Post);
    request->setPath("/api/sorting/start");

    // First request starts sorting.
    auto firstResult =
        client->sendRequest(request);

    ASSERT_EQ(
        firstResult.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        firstResult.second,
        nullptr);

    EXPECT_EQ(
        firstResult.second->getStatusCode(),
        drogon::k200OK);

    // Make sure the background sorting operation
    // has actually started before sending the second request.
    for (int i = 0;
         i < 100 && !sortingStarted.load();
         ++i)
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(
        sortingStarted.load());

    // Second request must be rejected.
    auto secondRequest =
        drogon::HttpRequest::newHttpRequest();

    secondRequest->setMethod(drogon::Post);
    secondRequest->setPath("/api/sorting/start");

    auto secondResult =
        client->sendRequest(secondRequest);

    ASSERT_EQ(
        secondResult.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        secondResult.second,
        nullptr);

    EXPECT_EQ(
        secondResult.second->getStatusCode(),
        drogon::k409Conflict);

    const auto json =
        secondResult.second->getJsonObject();

    ASSERT_NE(
        json,
        nullptr);

    EXPECT_EQ(
        (*json)["error"].asString(),
        "Sorting is already running.");

    // Allow the first sorting operation to finish.
    holdSorting.store(false);

    // Wait for the worker thread to complete.
    for (int i = 0; i < 100; ++i)
    {
        if (sortingController->getStatus() ==
            SortingStatus::Completed)
        {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }

    EXPECT_EQ(
        sortingController->getStatus(),
        SortingStatus::Completed);
}

TEST_F(
    ApiServerTest,
    GetSortingStatusReturnsCompleted)
{
    // Start sorting with the fake operation.
    holdSorting.store(false);
    sortingStarted.store(false);

    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    auto startRequest =
        drogon::HttpRequest::newHttpRequest();

    startRequest->setMethod(drogon::Post);
    startRequest->setPath("/api/sorting/start");

    auto startResult =
        client->sendRequest(startRequest);

    ASSERT_EQ(
        startResult.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        startResult.second,
        nullptr);

    ASSERT_EQ(
        startResult.second->getStatusCode(),
        drogon::k200OK);

    // Wait for the background operation to finish.
    for (int i = 0; i < 100; ++i)
    {
        if (sortingController->getStatus() ==
            SortingStatus::Completed)
        {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }

    ASSERT_EQ(
        sortingController->getStatus(),
        SortingStatus::Completed);

    // Now query the API.
    auto statusRequest =
        drogon::HttpRequest::newHttpRequest();

    statusRequest->setMethod(drogon::Get);
    statusRequest->setPath("/api/sorting/status");

    auto statusResult =
        client->sendRequest(statusRequest);

    ASSERT_EQ(
        statusResult.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        statusResult.second,
        nullptr);

    EXPECT_EQ(
        statusResult.second->getStatusCode(),
        drogon::k200OK);

    const auto json =
        statusResult.second->getJsonObject();

    ASSERT_NE(
        json,
        nullptr);

    EXPECT_EQ(
        (*json)["status"].asString(),
        "completed");
}

TEST_F(
    ApiServerTest,
    GetSortingProgressReturnsProgress)
{
    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    auto request =
        drogon::HttpRequest::newHttpRequest();

    request->setMethod(drogon::Get);
    request->setPath("/api/sorting/progress");

    auto result =
        client->sendRequest(request);

    ASSERT_EQ(
        result.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        result.second,
        nullptr);

    EXPECT_EQ(
        result.second->getStatusCode(),
        drogon::k200OK);

    const auto json =
        result.second->getJsonObject();

    ASSERT_NE(
        json,
        nullptr);

    EXPECT_TRUE(
        json->isMember("totalFiles"));

    EXPECT_TRUE(
        json->isMember("processedFiles"));

    EXPECT_TRUE(
        json->isMember("skippedFiles"));

    EXPECT_TRUE(
        json->isMember("failedFiles"));

    EXPECT_TRUE(
        json->isMember("currentFile"));

    EXPECT_TRUE(
        json->isMember("percentage"));
}

TEST_F(
    ApiServerTest,
    SortingProgressStreamClosesWhenSortingCompletes)
{
    holdSorting.store(false);
    sortingStarted.store(false);

    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    auto startRequest =
        drogon::HttpRequest::newHttpRequest();

    startRequest->setMethod(drogon::Post);
    startRequest->setPath("/api/sorting/start");

    auto startResult =
        client->sendRequest(startRequest);

    ASSERT_EQ(
        startResult.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        startResult.second,
        nullptr);

    ASSERT_EQ(
        startResult.second->getStatusCode(),
        drogon::k200OK);

    for (int i = 0; i < 100; ++i)
    {
        if (sortingController->getStatus() ==
            SortingStatus::Completed)
        {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }

    ASSERT_EQ(
        sortingController->getStatus(),
        SortingStatus::Completed);

    auto request =
        drogon::HttpRequest::newHttpRequest();

    request->setMethod(drogon::Get);
    request->setPath(
        "/api/sorting/progress/stream");

    auto result =
        client->sendRequest(request);

    ASSERT_EQ(
        result.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        result.second,
        nullptr);

    EXPECT_EQ(
        result.second->getStatusCode(),
        drogon::k200OK);

    EXPECT_EQ(
        result.second->getHeader("Content-Type"),
        "text/event-stream");

    const std::string_view body =
        result.second->getBody();

    EXPECT_NE(
        body.find("data:"),
        std::string::npos);
}

TEST_F(
    ApiServerTest,
    CancelSortingReturnsCancelling)
{
    holdSorting.store(true);
    sortingStarted.store(false);

    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    // Start sorting.
    auto startRequest =
        drogon::HttpRequest::newHttpRequest();

    startRequest->setMethod(drogon::Post);
    startRequest->setPath(
        "/api/sorting/start");

    auto startResult =
        client->sendRequest(startRequest);

    ASSERT_EQ(
        startResult.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        startResult.second,
        nullptr);

    ASSERT_EQ(
        startResult.second->getStatusCode(),
        drogon::k200OK);

    // Wait until the worker is actually running.
    for (int i = 0;
         i < 100 && !sortingStarted.load();
         ++i)
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(
        sortingStarted.load());

    // Request cancellation.
    auto cancelRequest =
        drogon::HttpRequest::newHttpRequest();

    cancelRequest->setMethod(drogon::Post);
    cancelRequest->setPath(
        "/api/sorting/cancel");

    auto cancelResult =
        client->sendRequest(cancelRequest);

    ASSERT_EQ(
        cancelResult.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        cancelResult.second,
        nullptr);

    EXPECT_EQ(
        cancelResult.second->getStatusCode(),
        drogon::k200OK);

    const auto json =
        cancelResult.second->getJsonObject();

    ASSERT_NE(
        json,
        nullptr);

    EXPECT_EQ(
        (*json)["status"].asString(),
        "cancelling");

    // Release the fake sorting operation.
    holdSorting.store(false);

    // Wait for cancellation to propagate.
    for (int i = 0; i < 100; ++i)
    {
        if (sortingController->getStatus() ==
            SortingStatus::Cancelled)
        {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }

    EXPECT_EQ(
        sortingController->getStatus(),
        SortingStatus::Cancelled);
}

TEST_F(
    ApiServerTest,
    CancelSortingReturnsConflictWhenNotRunning)
{
    holdSorting.store(false);

    // Make sure any previous worker has finished.
    for (int i = 0; i < 100; ++i)
    {
        const SortingStatus status =
            sortingController->getStatus();

        if (status != SortingStatus::Running &&
            status != SortingStatus::Cancelling)
        {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }

    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    auto request =
        drogon::HttpRequest::newHttpRequest();

    request->setMethod(drogon::Post);
    request->setPath(
        "/api/sorting/cancel");

    auto result =
        client->sendRequest(request);

    ASSERT_EQ(
        result.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        result.second,
        nullptr);

    EXPECT_EQ(
        result.second->getStatusCode(),
        drogon::k409Conflict);

    const auto json =
        result.second->getJsonObject();

    ASSERT_NE(
        json,
        nullptr);

    EXPECT_EQ(
        (*json)["error"].asString(),
        "Sorting is not running.");
}

TEST_F(
    ApiServerTest,
    SortingWorkflowCompletesSuccessfully)
{
    holdSorting.store(false);
    sortingStarted.store(false);

    auto client =
        drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:" +
            std::to_string(port));

    // 1. Start sorting.
    auto startRequest =
        drogon::HttpRequest::newHttpRequest();

    startRequest->setMethod(drogon::Post);
    startRequest->setPath(
        "/api/sorting/start");

    auto startResult =
        client->sendRequest(startRequest);

    ASSERT_EQ(
        startResult.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        startResult.second,
        nullptr);

    ASSERT_EQ(
        startResult.second->getStatusCode(),
        drogon::k200OK);

    // 2. Wait for the background operation to complete.
    for (int i = 0; i < 100; ++i)
    {
        if (sortingController->getStatus() ==
            SortingStatus::Completed)
        {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }

    ASSERT_EQ(
        sortingController->getStatus(),
        SortingStatus::Completed);

    // 3. Query sorting status through the API.
    auto statusRequest =
        drogon::HttpRequest::newHttpRequest();

    statusRequest->setMethod(drogon::Get);
    statusRequest->setPath(
        "/api/sorting/status");

    auto statusResult =
        client->sendRequest(statusRequest);

    ASSERT_EQ(
        statusResult.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        statusResult.second,
        nullptr);

    EXPECT_EQ(
        statusResult.second->getStatusCode(),
        drogon::k200OK);

    const auto statusJson =
        statusResult.second->getJsonObject();

    ASSERT_NE(
        statusJson,
        nullptr);

    EXPECT_EQ(
        (*statusJson)["status"].asString(),
        "completed");

    // 4. Query progress through the API.
    auto progressRequest =
        drogon::HttpRequest::newHttpRequest();

    progressRequest->setMethod(drogon::Get);
    progressRequest->setPath(
        "/api/sorting/progress");

    auto progressResult =
        client->sendRequest(progressRequest);

    ASSERT_EQ(
        progressResult.first,
        drogon::ReqResult::Ok);

    ASSERT_NE(
        progressResult.second,
        nullptr);

    EXPECT_EQ(
        progressResult.second->getStatusCode(),
        drogon::k200OK);

    const auto progressJson =
        progressResult.second->getJsonObject();

    ASSERT_NE(
        progressJson,
        nullptr);

    EXPECT_TRUE(
        progressJson->isMember("totalFiles"));

    EXPECT_TRUE(
        progressJson->isMember("processedFiles"));

    EXPECT_TRUE(
        progressJson->isMember("skippedFiles"));

    EXPECT_TRUE(
        progressJson->isMember("failedFiles"));

    EXPECT_TRUE(
        progressJson->isMember("currentFile"));

    EXPECT_TRUE(
        progressJson->isMember("percentage"));
}
