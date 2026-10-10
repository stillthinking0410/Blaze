#pragma once

#include "Config.hpp"
#include "Logger.hpp"

#include <filesystem>

class ConfigManager
{
public:

    explicit ConfigManager(
        const std::filesystem::path& configPath,
        Logger& logger
    );

    bool initialize();

    bool load();

    bool save() const;

    bool validate() const;

    bool updateFolders(
        const std::filesystem::path& sourceFolder,
        const std::filesystem::path& destinationFolder
    );

    Config& getConfig();

    const Config& getConfig() const;

private:

    bool createDefaultConfig() const;

    bool writeConfig() const;

    std::filesystem::path configPath_;

    Config config_;

    Logger& logger_;

};