#include "ConfigManager.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

ConfigManager::ConfigManager(
    const std::filesystem::path& configPath,
    Logger& logger
)
    : configPath_(configPath),
      logger_(logger)
{
}

Config&
ConfigManager::getConfig()
{
    return config_;
}

const Config&
ConfigManager::getConfig() const
{
    return config_;
}

bool ConfigManager::writeConfig() const
{
    try
    {
        std::filesystem::create_directories(
            configPath_.parent_path());

        nlohmann::json json;

        json["sourceFolder"] =
            config_.sourceFolder.string();

        json["destinationFolder"] =
            config_.destinationFolder.string();

        json["databasePath"] = 
            config_.databasePath.string();

        json["exifToolPath"] =
            config_.exifToolPath.string();

        json["logFile"] =
            config_.logFile.string();

        json["maxThreads"] =
            config_.maxThreads;

        std::ofstream file(configPath_);

        if (!file.is_open())
        {
            return false;
        }

        file << json.dump(4);

        logger_.info(
            "Configuration saved successfully."
        );

        return true;
    }
    catch (const std::exception &e)
    {
        logger_.error(
            "Failed to write configuration: " +
            std::string(e.what())
        );

        return false;
    }
}

bool ConfigManager::load()
{
    try
    {
        std::ifstream file(configPath_);

        if(!file.is_open())
        {
            logger_.error(
                "Failed to open configuration file."
            );
            return false;
        }

        nlohmann::json json;

        file >> json;

        config_.sourceFolder =
            json.value(
                "sourceFolder",
                ""
            );
        
        config_.destinationFolder =
            json.value(
                "destinationFolder",
                ""
            );

        config_.databasePath = 
            json.value(
                "databasePath",
                "database/blaze.db"
            );
        
        config_.exifToolPath = 
            json.value(
                "exifToolPath",
                "exiftool"
            );
        
        config_.logFile =
            json.value(
                "logFile",
                "logs/blaze.log"
            );

        config_.maxThreads = 
            json.value(
                "maxThreads",
                4u
            );
        
        return true;
    }
    catch(const std::exception& e)
    {
        logger_.error(
            "Failed to load configuration: " +
            std::string(e.what())
        );

        return false;
    }
    
}

bool ConfigManager::createDefaultConfig() const
{
    return writeConfig();
}

bool ConfigManager::initialize()
{
    logger_.info(
        "Initializing configuration."
    );

    if (!std::filesystem::exists(configPath_))
    {
        logger_.warning(
            "Configuration file not found. Creating default configuration."
        );

        if (!createDefaultConfig())
        {
            return false;
        }

        logger_.info(
            "Default configuration created."
        );
    }

    if (!load())
    {
        return false;
    }

    logger_.info(
        "Configuration loaded successfully."
    );

    return true;
}

bool ConfigManager::save() const
{
    logger_.info(
        "Saving configuration."
    );

    return writeConfig();
}

bool ConfigManager::validate() const
{
    if (config_.sourceFolder.empty())
    {
        logger_.error(
            "Source folder is not configured."
        );

        return false;
    }

    if (!std::filesystem::exists(config_.sourceFolder) ||
        !std::filesystem::is_directory(config_.sourceFolder))
    {
        logger_.error(
            "Source folder does not exist or is not a directory."
        );

        return false;
    }

    if (config_.destinationFolder.empty())
    {
        logger_.error(
            "Destination folder is not configured."
        );

        return false;
    }

    if (config_.maxThreads == 0)
    {
        logger_.error(
            "Maximum thread count must be greater than zero."
        );

        return false;
    }

    return true;
}

bool ConfigManager::updateFolders(
    const std::filesystem::path& sourceFolder,
    const std::filesystem::path& destinationFolder
)
{
    if (sourceFolder.empty())
    {
        logger_.error(
            "Source folder is not configured."
        );

        return false;
    }

    if (!std::filesystem::exists(sourceFolder) ||
        !std::filesystem::is_directory(sourceFolder))
    {
        logger_.error(
            "Source folder does not exist or is not a directory."
        );

        return false;
    }

    if (destinationFolder.empty())
    {
        logger_.error(
            "Destination folder is not configured."
        );

        return false;
    }

    Config updatedConfig = config_;

    updatedConfig.sourceFolder = sourceFolder;
    updatedConfig.destinationFolder = destinationFolder;

    const Config originalConfig = config_;

    config_ = updatedConfig;

    if (!save())
    {
        config_ = originalConfig;
        return false;
    }

    logger_.info(
        "Source and destination folders updated successfully."
    );

    return true;
}
