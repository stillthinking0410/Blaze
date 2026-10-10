#include "ApiServer.hpp"

ApiServer::ApiServer(
    ConfigManager &configManager,
    SortingController &sortingController)
    : configManager_(configManager),
      sortingController_(sortingController)
{
    registerRoutes();
}

void ApiServer::registerRoutes()
{
    // GET /api/config
    drogon::app().registerHandler(
        "/api/config",
        [this](
            const drogon::HttpRequestPtr &,
            std::function<void(
                const drogon::HttpResponsePtr &)> &&callback)
        {
            const Config &config =
                configManager_.getConfig();

            Json::Value response;

            response["sourceFolder"] =
                config.sourceFolder.string();

            response["destinationFolder"] =
                config.destinationFolder.string();

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            callback(httpResponse);
        },
        {drogon::Get});

    // PUT /api/config
    drogon::app().registerHandler(
        "/api/config",
        [this](
            const drogon::HttpRequestPtr &request,
            std::function<void(
                const drogon::HttpResponsePtr &)> &&callback)
        {
            auto json =
                request->getJsonObject();

            if (json == nullptr)
            {
                Json::Value response;

                response["error"] =
                    "Invalid JSON request.";

                auto httpResponse =
                    drogon::HttpResponse::newHttpJsonResponse(
                        response);

                httpResponse->setStatusCode(
                    drogon::k400BadRequest);

                callback(httpResponse);
                return;
            }

            if (!json->isMember("sourceFolder") ||
                !(*json)["sourceFolder"].isString() ||
                !json->isMember("destinationFolder") ||
                !(*json)["destinationFolder"].isString())
            {
                Json::Value response;

                response["error"] =
                    "sourceFolder and destinationFolder are required.";

                auto httpResponse =
                    drogon::HttpResponse::newHttpJsonResponse(
                        response);

                httpResponse->setStatusCode(
                    drogon::k400BadRequest);

                callback(httpResponse);
                return;
            }

            const std::filesystem::path sourceFolder =
                (*json)["sourceFolder"].asString();

            const std::filesystem::path destinationFolder =
                (*json)["destinationFolder"].asString();

            if (!configManager_.updateFolders(
                    sourceFolder,
                    destinationFolder))
            {
                Json::Value response;

                response["error"] =
                    "Invalid configuration.";

                auto httpResponse =
                    drogon::HttpResponse::newHttpJsonResponse(
                        response);

                httpResponse->setStatusCode(
                    drogon::k400BadRequest);

                callback(httpResponse);
                return;
            }

            Json::Value response;

            response["status"] =
                "updated";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            httpResponse->setStatusCode(
                drogon::k200OK);

            callback(httpResponse);
        },
        {drogon::Put});

    drogon::app().registerHandler(
        "/api/sorting/start",
        [this](
            const drogon::HttpRequestPtr &,
            std::function<void(
                const drogon::HttpResponsePtr &)> &&callback)
        {
            if (!sortingController_.start())
            {
                Json::Value response;

                response["error"] =
                    "Sorting is already running.";

                auto httpResponse =
                    drogon::HttpResponse::newHttpJsonResponse(
                        response);

                httpResponse->setStatusCode(
                    drogon::k409Conflict);

                callback(httpResponse);
                return;
            }

            Json::Value response;

            response["status"] =
                "running";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            httpResponse->setStatusCode(
                drogon::k200OK);

            callback(httpResponse);
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/sorting/status",
        [this](
            const drogon::HttpRequestPtr &,
            std::function<void(
                const drogon::HttpResponsePtr &)> &&callback)
        {
            Json::Value response;

            const SortingStatus status =
                sortingController_.getStatus();

            switch (status)
            {
            case SortingStatus::Idle:
                response["status"] = "idle";
                break;

            case SortingStatus::Running:
                response["status"] = "running";
                break;

            case SortingStatus::Cancelling:
                response["status"] = "cancelling";
                break;

            case SortingStatus::Completed:
                response["status"] = "completed";
                break;

            case SortingStatus::CompletedWithErrors:
                response["status"] =
                    "completed_with_errors";
                break;

            case SortingStatus::Failed:
                response["status"] = "failed";
                break;

            case SortingStatus::Cancelled:
                response["status"] = "cancelled";
                break;
            }

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            httpResponse->setStatusCode(
                drogon::k200OK);

            callback(httpResponse);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/sorting/progress",
        [this](
            const drogon::HttpRequestPtr &,
            std::function<void(
                const drogon::HttpResponsePtr &)> &&callback)
        {
            const ProgressInfo progress =
                sortingController_.getProgress();

            Json::Value response;

            response["totalFiles"] =
                static_cast<Json::UInt64>(
                    progress.totalFiles);

            response["processedFiles"] =
                static_cast<Json::UInt64>(
                    progress.processedFiles);

            response["skippedFiles"] =
                static_cast<Json::UInt64>(
                    progress.skippedFiles);

            response["failedFiles"] =
                static_cast<Json::UInt64>(
                    progress.failedFiles);

            response["currentFile"] =
                progress.currentFile.string();

            response["percentage"] =
                progress.percentage();

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            httpResponse->setStatusCode(
                drogon::k200OK);

            callback(httpResponse);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/sorting/progress/stream",
        [this](
            const drogon::HttpRequestPtr &,
            std::function<void(
                const drogon::HttpResponsePtr &)> &&callback)
        {
            auto response =
                drogon::HttpResponse::newAsyncStreamResponse(
                    [this](
                        drogon::ResponseStreamPtr stream)
                    {
                        std::thread(
                            [this](
                                drogon::ResponseStreamPtr stream)
                            {
                                while (true)
                                {
                                    const ProgressInfo progress =
                                        sortingController_.getProgress();

                                    Json::Value json;

                                    json["totalFiles"] =
                                        static_cast<Json::UInt64>(
                                            progress.totalFiles);

                                    json["processedFiles"] =
                                        static_cast<Json::UInt64>(
                                            progress.processedFiles);

                                    json["skippedFiles"] =
                                        static_cast<Json::UInt64>(
                                            progress.skippedFiles);

                                    json["failedFiles"] =
                                        static_cast<Json::UInt64>(
                                            progress.failedFiles);

                                    json["currentFile"] =
                                        progress.currentFile.string();

                                    json["percentage"] =
                                        progress.percentage();

                                    const std::string data =
                                        "data: " +
                                        Json::writeString(
                                            Json::StreamWriterBuilder(),
                                            json) +
                                        "\n\n";

                                    if (!stream->send(data))
                                    {
                                        break;
                                    }

                                    const SortingStatus status =
                                        sortingController_.getStatus();

                                    if (status ==
                                            SortingStatus::Completed ||
                                        status ==
                                            SortingStatus::CompletedWithErrors ||
                                        status ==
                                            SortingStatus::Failed ||
                                        status ==
                                            SortingStatus::Cancelled)
                                    {
                                        break;
                                    }

                                    std::this_thread::sleep_for(
                                        std::chrono::milliseconds(250));
                                }

                                stream->close();
                            },
                            std::move(stream))
                            .detach();
                    },
                    true);

            response->setContentTypeString(
                "text/event-stream");

            response->addHeader(
                "Cache-Control",
                "no-cache");

            response->addHeader(
                "Connection",
                "keep-alive");

            callback(response);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/sorting/cancel",
        [this](
            const drogon::HttpRequestPtr &,
            std::function<void(
                const drogon::HttpResponsePtr &)> &&callback)
        {
            const SortingStatus status =
                sortingController_.getStatus();

            if (status != SortingStatus::Running &&
                status != SortingStatus::Cancelling)
            {
                Json::Value response;

                response["error"] =
                    "Sorting is not running.";

                auto httpResponse =
                    drogon::HttpResponse::newHttpJsonResponse(
                        response);

                httpResponse->setStatusCode(
                    drogon::k409Conflict);

                callback(httpResponse);
                return;
            }

            sortingController_.requestCancellation();

            Json::Value response;

            response["status"] =
                "cancelling";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            httpResponse->setStatusCode(
                drogon::k200OK);

            callback(httpResponse);
        },
        {drogon::Post});
}

void ApiServer::start(
    const std::string &host,
    uint16_t port)
{
    drogon::app()
        .addListener(host, port)
        .run();
}
