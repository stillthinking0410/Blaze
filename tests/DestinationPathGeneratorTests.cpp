#include "DestinationPathGenerator.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <ctime>

TEST(
    DestinationPathGeneratorTest,
    GeneratesCorrectFolderPath
)
{
    DestinationPathGenerator generator;

    std::tm tm = {};

    tm.tm_year = 2026 - 1900;
    tm.tm_mon = 6;
    tm.tm_mday = 18;

    std::time_t time = std::mktime(&tm);

    auto captureDate =
        std::chrono::system_clock::from_time_t(time);
    
    auto result =
        generator.generateFolderPath(
            "/Photos",
            captureDate
        );
    
    EXPECT_EQ(
        result,
        "/Photos/2026/July"
    );
}

TEST(
    DestinationPathGeneratorTest,
    GeneratesCorrectFileName
)
{
    DestinationPathGenerator generator;

    std::tm tm = {};

    tm.tm_year = 2026 - 1900;
    tm.tm_mon = 6;
    tm.tm_mday = 18;
    tm.tm_hour = 10;
    tm.tm_min = 42;
    tm.tm_sec = 53;

    std::time_t time = std::mktime(&tm);

    auto captureDate =
        std::chrono::system_clock::from_time_t(time);

    auto result =
        generator.generateFileName(
            captureDate,
            ".jpg"
        );

    EXPECT_EQ(
        result,
        "2026-07-18_10-42-53.jpg"
    );
}

TEST(
    DestinationPathGeneratorTest,
    PreservesFileExtension
)
{
    DestinationPathGenerator generator;

    std::tm tm = {};

    tm.tm_year = 2026 - 1900;
    tm.tm_mon = 6;
    tm.tm_mday = 18;

    std::time_t time = std::mktime(&tm);

    auto captureDate =
        std::chrono::system_clock::from_time_t(time);

    EXPECT_EQ(
        generator.generateFileName(
            captureDate,
            ".mov"
        ),
        "2026-07-18_00-00-00.mov"
    );

    EXPECT_EQ(
        generator.generateFileName(
            captureDate,
            ".HEIC"
        ),
        "2026-07-18_00-00-00.HEIC"
    );
}

TEST(
    DestinationPathGeneratorTest,
    UsesProvidedDestinationRoot
)
{
    DestinationPathGenerator generator;

    std::tm tm = {};

    tm.tm_year = 2025 - 1900;
    tm.tm_mon = 11;
    tm.tm_mday = 25;

    std::time_t time = std::mktime(&tm);

    auto captureDate =
        std::chrono::system_clock::from_time_t(time);

    auto result =
        generator.generateFolderPath(
            "/Users/Test/Photos",
            captureDate
        );

    EXPECT_EQ(
        result,
        "/Users/Test/Photos/2025/December"
    );
}

