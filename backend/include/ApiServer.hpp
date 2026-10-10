#pragma once

#include <cstdint>
#include <string>

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

    void start(
        const std::string& host,
        uint16_t port
    );

private:
    void registerRoutes();

    ConfigManager& configManager_;
    SortingController& sortingController_;
};