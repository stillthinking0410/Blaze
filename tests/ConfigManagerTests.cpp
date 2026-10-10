#include "ConfigManager.hpp"
#include "Logger.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    class ConfigManagerTest : public ::testing::Test
    {
    protected:
        
        std::filesystem::path testDirectory_;

        std::filesystem::path configPath_;

        std::filesystem::path logPath_;

        void SetUp() override
        {
            testDirectory_ = 
                std::filesystem::temp_directory_path()
                / "blaze_config_manager_test";
            
            std::filesystem::remove_all(
                testDirectory_
            );

            std::filesystem::create_directories(
                testDirectory_
            );

            configPath_ = 
                testDirectory_ / "config.json";

            logPath_ =
                testDirectory_ / "test.log";
        }

        void TearDown() override
        {
            std::filesystem::remove_all(
                testDirectory_
            );
        }
    };
}


TEST_F(
    ConfigManagerTest,
    CreatesDefaultConfiguration)
{
    ASSERT_FALSE(
        std::filesystem::exists(configPath_)
    );

    Logger logger(logPath_);

    ConfigManager manager(
        configPath_,
        logger
    );

    EXPECT_TRUE(
        manager.initialize()
    );

    EXPECT_TRUE(
        std::filesystem::exists(configPath_)
    );
}

TEST_F(
    ConfigManagerTest,
    LoadsDefaultValues)
{
    Logger logger(logPath_);

    ConfigManager manager(
        configPath_,
        logger
    );

    ASSERT_TRUE(
        manager.initialize()
    );

    const Config& config =
        manager.getConfig();

    EXPECT_TRUE(
        config.sourceFolder.empty()
    );

    EXPECT_TRUE(
        config.destinationFolder.empty()
    );

    EXPECT_EQ(
        config.databasePath,
        std::filesystem::path("database/blaze.db")
    );

    EXPECT_EQ(
        config.exifToolPath,
        std::filesystem::path("exiftool")
    );

    EXPECT_EQ(
        config.logFile,
        std::filesystem::path("logs/blaze.log")
    );

    EXPECT_EQ(
        config.maxThreads,
        4u
    );
}

TEST_F(
    ConfigManagerTest,
    PreservesConfigurationAfterSaveAndLoad)
{
    Logger logger(logPath_);

    ConfigManager manager(
        configPath_,
        logger
    );

    ASSERT_TRUE(
        manager.initialize()
    );

    Config& config =
        manager.getConfig();

    config.sourceFolder =
        "/Photos";

    config.destinationFolder =
        "/SortedPhotos";

    config.databasePath =
        "/Data/blaze.db";

    config.exifToolPath =
        "/usr/local/bin/exiftool";

    config.logFile =
        "/Logs/blaze.log";

    config.maxThreads =
        8;

    ASSERT_TRUE(
        manager.save()
    );

    ConfigManager loadedManager(
        configPath_,
        logger
    );

    ASSERT_TRUE(
        loadedManager.load()
    );

    const Config& loadedConfig =
        loadedManager.getConfig();

    EXPECT_EQ(
        loadedConfig.sourceFolder,
        config.sourceFolder
    );

    EXPECT_EQ(
        loadedConfig.destinationFolder,
        config.destinationFolder
    );

    EXPECT_EQ(
        loadedConfig.databasePath,
        config.databasePath
    );

    EXPECT_EQ(
        loadedConfig.exifToolPath,
        config.exifToolPath
    );

    EXPECT_EQ(
        loadedConfig.logFile,
        config.logFile
    );

    EXPECT_EQ(
        loadedConfig.maxThreads,
        config.maxThreads
    );
}

TEST_F(
    ConfigManagerTest,
    LoadsExistingConfiguration)
{
    std::ofstream file(configPath_);

    ASSERT_TRUE(
        file.is_open()
    );

    file << R"({
        "sourceFolder": "/Input",
        "destinationFolder": "/Output",
        "databasePath": "/Database/blaze.db",
        "exifToolPath": "/usr/bin/exiftool",
        "logFile": "/Logs/blaze.log",
        "maxThreads": 12
    })";

    file.close();

    Logger logger(logPath_);

    ConfigManager manager(
        configPath_,
        logger
    );

    ASSERT_TRUE(
        manager.load()
    );

    const Config& config =
        manager.getConfig();

    EXPECT_EQ(
        config.sourceFolder,
        "/Input"
    );

    EXPECT_EQ(
        config.destinationFolder,
        "/Output"
    );

    EXPECT_EQ(
        config.databasePath,
        "/Database/blaze.db"
    );

    EXPECT_EQ(
        config.exifToolPath,
        "/usr/bin/exiftool"
    );

    EXPECT_EQ(
        config.logFile,
        "/Logs/blaze.log"
    );

    EXPECT_EQ(
        config.maxThreads,
        12u
    );
}

TEST_F(
    ConfigManagerTest,
    UsesDefaultsForMissingFields)
{
    std::ofstream file(configPath_);

    ASSERT_TRUE(
        file.is_open()
    );

    file << R"({
        "sourceFolder": "/Input"
    })";

    file.close();

    Logger logger(logPath_);

    ConfigManager manager(
        configPath_,
        logger
    );

    ASSERT_TRUE(
        manager.load()
    );

    const Config& config =
        manager.getConfig();

    EXPECT_EQ(
        config.sourceFolder,
        "/Input"
    );

    EXPECT_TRUE(
        config.destinationFolder.empty()
    );

    EXPECT_EQ(
        config.databasePath,
        "database/blaze.db"
    );

    EXPECT_EQ(
        config.exifToolPath,
        "exiftool"
    );

    EXPECT_EQ(
        config.logFile,
        "logs/blaze.log"
    );

    EXPECT_EQ(
        config.maxThreads,
        4u
    );
}

TEST_F(
    ConfigManagerTest,
    ReturnsFalseForInvalidJson)
{
    std::ofstream file(configPath_);

    ASSERT_TRUE(
        file.is_open()
    );

    file << "{ invalid json ";

    file.close();

    Logger logger(logPath_);

    ConfigManager manager(
        configPath_,
        logger
    );

    EXPECT_FALSE(
        manager.load()
    );
}

TEST_F(
    ConfigManagerTest,
    ReturnsFalseWhenConfigurationFileDoesNotExist)
{
    ASSERT_FALSE(
        std::filesystem::exists(configPath_)
    );

    Logger logger(logPath_);

    ConfigManager manager(
        configPath_,
        logger
    );

    EXPECT_FALSE(
        manager.load()
    );
}

TEST_F(
    ConfigManagerTest,
    RejectsMissingSourceFolder)
{
    Logger logger(logPath_);

    ConfigManager manager(
        configPath_,
        logger
    );

    Config& config =
        manager.getConfig();

    config.destinationFolder =
        testDirectory_ / "destination";

    config.maxThreads = 4;

    EXPECT_FALSE(
        manager.validate()
    );
}

TEST_F(
    ConfigManagerTest,
    RejectsMissingDestinationFolder)
{
    Logger logger(logPath_);

    ConfigManager manager(
        configPath_,
        logger
    );

    Config& config =
        manager.getConfig();

    config.sourceFolder =
        testDirectory_ / "source";

    config.maxThreads = 4;

    EXPECT_FALSE(
        manager.validate()
    );
}

TEST_F(
    ConfigManagerTest,
    RejectsZeroMaxThreads)
{
    Logger logger(logPath_);

    ConfigManager manager(
        configPath_,
        logger
    );

    Config& config =
        manager.getConfig();

    config.sourceFolder =
        testDirectory_ / "source";

    config.destinationFolder =
        testDirectory_ / "destination";

    config.maxThreads = 0;

    EXPECT_FALSE(
        manager.validate()
    );
}

TEST_F(
    ConfigManagerTest,
    AcceptsValidConfiguration)
{
    Logger logger(logPath_);

    ConfigManager manager(
        configPath_,
        logger
    );

    Config& config =
        manager.getConfig();

    config.sourceFolder =
        testDirectory_ / "source";

    config.destinationFolder =
        testDirectory_ / "destination";

    config.maxThreads = 4;

    std::filesystem::create_directories(
        config.sourceFolder
    );

    EXPECT_TRUE(
        manager.validate()
    );
}

TEST_F(
    ConfigManagerTest,
    UpdatesFoldersSuccessfully)
{
    Logger logger(logPath_);

    ConfigManager manager(
        configPath_,
        logger
    );

    std::filesystem::create_directories(
        testDirectory_ / "new_source"
    );

    EXPECT_TRUE(
        manager.updateFolders(
            testDirectory_ / "new_source",
            testDirectory_ / "new_destination"
        )
    );

    const Config& config =
        manager.getConfig();

    EXPECT_EQ(
        config.sourceFolder,
        testDirectory_ / "new_source"
    );

    EXPECT_EQ(
        config.destinationFolder,
        testDirectory_ / "new_destination"
    );
}

TEST_F(
    ConfigManagerTest,
    RejectsNonexistentSourceFolder)
{
    Logger logger(logPath_);

    ConfigManager manager(
        configPath_,
        logger
    );

    EXPECT_FALSE(
        manager.updateFolders(
            testDirectory_ / "missing_source",
            testDirectory_ / "destination"
        )
    );
}
