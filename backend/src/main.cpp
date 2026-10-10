#include <cstdint>
#include <filesystem>
#include <iostream>

#include "ApiServer.hpp"
#include "ConfigManager.hpp"
#include "Logger.hpp"
#include "SortingController.hpp"

int main()
{
    Logger logger("../logs/blaze.log");

    ConfigManager configManager(
        "../config/config.json",
        logger
    );

    if (!logger.initialize())
    {
        std::cerr << "Failed to initialize Blaze logger\n";
        return 1;
    }

    if (!configManager.initialize())
    {
        std::cerr << "Config initialization failed\n";
        return 1;
    }

    if (!configManager.validate())
    {
        std::cerr << "Invalid configuration\n";
        return 1;
    }

    SortingController sortingController(
        configManager.getConfig(),
        logger
    );

    ApiServer apiServer(
        configManager,
        sortingController
    );

    constexpr std::uint16_t port = 8080;

    std::cout
        << "Blaze API server starting on http://127.0.0.1:"
        << port
        << '\n';

    apiServer.start("127.0.0.1", port);

    return 0;
}