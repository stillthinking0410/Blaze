#pragma once

#include <cstdint>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include <drogon/drogon.h>

#include "ConfigManager.hpp"
#include "SortingController.hpp"

class ApiServer
{
public:
    ApiServer(
        ConfigManager& configManager,
        SortingController& sortingController
    );

    ~ApiServer();

    void start(
        const std::string& host,
        uint16_t port
    );

private:
    void registerRoutes();

    ConfigManager& configManager_;
    SortingController& sortingController_;

    std::atomic_bool shuttingDown_{false};
    std::mutex workerMutex_;
    std::vector<std::thread> workers_;
};