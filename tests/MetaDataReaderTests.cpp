#include "Config.hpp"
#include "MetaDataReader.hpp"
#include "Logger.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    class MetaDataReaderTest : public ::testing::Test
    {
    protected:
        
        std::filesystem::path testDirectory_;

        std::filesystem::path logPath_;

        void SetUp() override
        {
            testDirectory_ = 
                std::filesystem::temp_directory_path()
                / "blaze_metadata_reader_test";

            std::filesystem::remove_all(
                testDirectory_
            );

            std::filesystem::create_directories(
                testDirectory_
            );

            logPath_ = 
                testDirectory_ / "metadata.log";
        }

        void TearDown() override
        {
            std::filesystem::remove_all(
                testDirectory_
            );
        }

        void createFile(
            const std::filesystem::path& path,
            const std::string& content = ""
        )
        {
            std::filesystem::create_directories(
                path.parent_path()
            );

            std::ofstream file(path);

            ASSERT_TRUE(
                file.is_open()
            );

            file << content;
        }
    };
}


TEST_F(
    MetaDataReaderTest,
    ReturnsNulloptForMissingFile
)
{
    const auto file =
        testDirectory_ / "missing.jpg";

    Logger logger(logPath_);

    Config config;

    MetaDataReader reader(config, logger);

    auto result =
        reader.getCaptureDate(file);

    EXPECT_FALSE(
        result.has_value()
    );
}

TEST_F(
    MetaDataReaderTest,
    ReturnsNulloptWhenNoCaptureDateExists
)
{
    const auto file =
        testDirectory_ / "test.txt";

    createFile(
        file,
        "This file contains no EXIF metadata."
    );

    Logger logger(logPath_);

    Config config;

    MetaDataReader reader(config, logger);

    auto result =
        reader.getCaptureDate(file);

    EXPECT_FALSE(
        result.has_value()
    );
}

TEST_F(
    MetaDataReaderTest,
    ReturnsNulloptForEmptyFile
)
{
    const auto file =
        testDirectory_ / "empty.jpg";

    createFile(file);

    Logger logger(logPath_);

    Config config;

    MetaDataReader reader(config, logger);

    auto result =
        reader.getCaptureDate(file);

    EXPECT_FALSE(
        result.has_value()
    );
}

TEST_F(
    MetaDataReaderTest,
    ReadsCaptureDateFromExifMetadata
)
{
    const auto file =
        testDirectory_ / "photo.jpg";

    // Minimal valid 1x1 JPEG image.
    const unsigned char jpegData[] =
    {
        0xFF, 0xD8,
        0xFF, 0xE0, 0x00, 0x10,
        0x4A, 0x46, 0x49, 0x46, 0x00, 0x01,
        0x01, 0x01, 0x00, 0x48, 0x00, 0x48,
        0x00, 0x00,

        0xFF, 0xDB, 0x00, 0x43, 0x00,
        0x08, 0x06, 0x06, 0x07, 0x06, 0x05, 0x08, 0x07,
        0x07, 0x07, 0x09, 0x09, 0x08, 0x0A, 0x0C, 0x14,
        0x0D, 0x0C, 0x0B, 0x0B, 0x0C, 0x19, 0x12, 0x13,
        0x0F, 0x14, 0x1D, 0x1A, 0x1F, 0x1E, 0x1D, 0x1A,
        0x1C, 0x1C, 0x20, 0x24, 0x2E, 0x27, 0x20, 0x22,
        0x2C, 0x23, 0x1C, 0x1C, 0x28, 0x37, 0x29, 0x2C,
        0x30, 0x31, 0x34, 0x34, 0x34, 0x1F, 0x27, 0x39,
        0x3D, 0x38, 0x32, 0x3C, 0x2E, 0x33, 0x34, 0x32,

        0xFF, 0xC0, 0x00, 0x0B,
        0x08,
        0x00, 0x01,
        0x00, 0x01,
        0x01,
        0x01, 0x11, 0x00,

        0xFF, 0xC4, 0x00, 0x14,
        0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00,

        0xFF, 0xDA, 0x00, 0x08,
        0x01,
        0x01, 0x00,
        0x00,
        0x3F, 0x00,

        0x00,

        0xFF, 0xD9
    };

    {
        std::ofstream image(
            file,
            std::ios::binary
        );

        ASSERT_TRUE(
            image.is_open()
        );

        image.write(
            reinterpret_cast<const char*>(jpegData),
            sizeof(jpegData)
        );
    }

    const std::string expectedDate =
        "2024:07:15 14:30:45";

    const std::string command =
        "exiftool "
        "-DateTimeOriginal=\"" + expectedDate + "\" "
        "-CreateDate=\"" + expectedDate + "\" "
        "-overwrite_original "
        "\"" + file.string() + "\"";

    const int result =
        std::system(command.c_str());

    ASSERT_EQ(
        result,
        0
    );

    Logger logger(logPath_);

    Config config;

    MetaDataReader reader(config, logger);

    const auto captureDate =
        reader.getCaptureDate(file);

    ASSERT_TRUE(
        captureDate.has_value()
    );

    std::tm expectedTm = {};

    expectedTm.tm_year = 2024 - 1900;
    expectedTm.tm_mon = 7 - 1;
    expectedTm.tm_mday = 15;
    expectedTm.tm_hour = 14;
    expectedTm.tm_min = 30;
    expectedTm.tm_sec = 45;

    const std::time_t expectedTime =
        std::mktime(&expectedTm);

    const auto expectedTimePoint =
        std::chrono::system_clock::from_time_t(
            expectedTime
        );

    EXPECT_EQ(
        captureDate.value(),
        expectedTimePoint
    );
}
