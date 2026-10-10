#include "SortEngine.hpp"
#include "Logger.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <iterator>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
    class SortEngineTest : public ::testing::Test
    {
    protected:
        std::filesystem::path testDirectory_;

        std::filesystem::path sourceDirectory_;

        std::filesystem::path destinationDirectory_;

        std::filesystem::path databasePath_;

        std::filesystem::path logPath_;

        void SetUp() override
        {
            testDirectory_ =
                std::filesystem::temp_directory_path() / "blaze_sort_engine_test";

            std::filesystem::remove_all(
                testDirectory_);

            std::filesystem::create_directories(
                testDirectory_);

            sourceDirectory_ =
                testDirectory_ / "source";

            destinationDirectory_ =
                testDirectory_ / "destination";

            databasePath_ =
                testDirectory_ / "database" / "blaze.db";

            logPath_ =
                testDirectory_ / "sort_engine.log";

            std::filesystem::create_directories(
                sourceDirectory_);

            std::filesystem::create_directories(
                destinationDirectory_);
        }

        void TearDown() override
        {
            std::filesystem::remove_all(
                testDirectory_);
        }

        Config createConfig()
        {
            Config config;

            config.sourceFolder =
                sourceDirectory_;

            config.destinationFolder =
                destinationDirectory_;

            config.databasePath =
                databasePath_;

            config.logFile =
                logPath_;

            return config;
        }

        void createFile(
            const std::filesystem::path &path,
            const std::string &content = "")
        {
            std::filesystem::create_directories(
                path.parent_path());

            std::ofstream file(path);

            ASSERT_TRUE(
                file.is_open());

            file << content;
        }

        void createValidJpeg(
            const std::filesystem::path &file)
        {
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

                    0xFF, 0xD9};

            std::ofstream image(
                file,
                std::ios::binary);

            ASSERT_TRUE(
                image.is_open());

            image.write(
                reinterpret_cast<const char *>(jpegData),
                sizeof(jpegData));
        }

        void addCaptureDate(
            const std::filesystem::path &file,
            const std::string &date)
        {
            const std::string command =
                "exiftool "
                "-DateTimeOriginal=\"" +
                date + "\" "
                       "-CreateDate=\"" +
                date + "\" "
                       "-overwrite_original "
                       "\"" +
                file.string() + "\"";

            ASSERT_EQ(
                std::system(command.c_str()),
                0);
        }
    };
}

TEST_F(
    SortEngineTest,
    ReturnsTrueForEmptySourceDirectory)
{
    Config config = createConfig();

    Logger logger(logPath_);

    SortEngine engine(
        config,
        logger);

    EXPECT_TRUE(
        engine.run(
            sourceDirectory_,
            destinationDirectory_));
}

TEST_F(
    SortEngineTest,
    IgnoresUnsupportedFiles)
{
    createFile(
        sourceDirectory_ / "document.txt",
        "This is not a supported media file.");

    createFile(
        sourceDirectory_ / "document.pdf",
        "PDF placeholder.");

    Config config = createConfig();

    Logger logger(logPath_);

    SortEngine engine(
        config,
        logger);

    EXPECT_TRUE(
        engine.run(
            sourceDirectory_,
            destinationDirectory_));

    EXPECT_TRUE(
        std::filesystem::is_empty(
            destinationDirectory_));
}

TEST_F(
    SortEngineTest,
    ReturnsFalseWhenSourceDirectoryDoesNotExist)
{
    const auto missingDirectory =
        testDirectory_ / "missing";

    Config config = createConfig();

    Logger logger(logPath_);

    SortEngine engine(
        config,
        logger);

    EXPECT_FALSE(
        engine.run(
            missingDirectory,
            destinationDirectory_));
}

TEST_F(
    SortEngineTest,
    ReportsInitialProgress)
{
    createFile(
        sourceDirectory_ / "photo.txt");

    bool callbackCalled = false;

    ProgressInfo receivedProgress;

    Config config = createConfig();

    Logger logger(logPath_);

    SortEngine engine(
        config,
        logger);

    const bool result =
        engine.run(
            sourceDirectory_,
            destinationDirectory_,
            [&](const ProgressInfo &progress)
            {
                if (!callbackCalled)
                {
                    callbackCalled = true;
                    receivedProgress = progress;
                }
            });

    EXPECT_TRUE(result);

    EXPECT_TRUE(
        callbackCalled);

    EXPECT_EQ(
        receivedProgress.totalFiles,
        0u);
}

TEST_F(
    SortEngineTest,
    ProcessesFileUsingModificationTimeWhenMetadataCannotBeRead)
{
    createFile(
        sourceDirectory_ / "photo.jpg",
        "not a real jpeg"
    );

    Config config = createConfig();
    Logger logger(logPath_);

    SortEngine engine(config, logger);

    ProgressInfo finalProgress;

    ASSERT_TRUE(
        engine.run(
            sourceDirectory_,
            destinationDirectory_,
            [&](const ProgressInfo& progress)
            {
                finalProgress = progress;
            }
        )
    );

    EXPECT_EQ(finalProgress.totalFiles, 1u);
    EXPECT_EQ(finalProgress.processedFiles, 1u);
    EXPECT_EQ(finalProgress.skippedFiles, 0u);
    EXPECT_EQ(finalProgress.failedFiles, 0u);
}

TEST_F(
    SortEngineTest,
    ProcessesFileSuccessfully)
{
    const auto sourceFile =
        sourceDirectory_ / "photo.jpg";

    createValidJpeg(sourceFile);

    addCaptureDate(
        sourceFile,
        "2024:07:15 14:30:45");

    Config config = createConfig();

    Logger logger(logPath_);

    SortEngine engine(
        config,
        logger);

    ProgressInfo finalProgress;

    ASSERT_TRUE(
        engine.run(
            sourceDirectory_,
            destinationDirectory_,
            [&](const ProgressInfo &progress)
            {
                finalProgress = progress;
            }));

    EXPECT_EQ(
        finalProgress.totalFiles,
        1u);

    EXPECT_EQ(
        finalProgress.processedFiles,
        1u);

    EXPECT_EQ(
        finalProgress.skippedFiles,
        0u);

    EXPECT_EQ(
        finalProgress.failedFiles,
        0u);

    const auto expectedDestination =
        destinationDirectory_ / "2024" / "July" / "2024-07-15_14-30-45.jpg";

    EXPECT_TRUE(
        std::filesystem::exists(
            expectedDestination));

    EXPECT_TRUE(
        std::filesystem::is_regular_file(
            expectedDestination));
    EXPECT_TRUE(
        std::filesystem::exists(
            sourceFile));

    EXPECT_TRUE(
        std::filesystem::is_regular_file(
            sourceFile));
}

TEST_F(
    SortEngineTest,
    SkipsPreviouslyProcessedFile)
{
    const auto sourceFile =
        sourceDirectory_ / "photo.jpg";

    createValidJpeg(sourceFile);

    addCaptureDate(
        sourceFile,
        "2024:07:15 14:30:45");

    Config config = createConfig();

    Logger logger(logPath_);

    SortEngine engine(
        config,
        logger);

    ProgressInfo firstRun;

    ASSERT_TRUE(
        engine.run(
            sourceDirectory_,
            destinationDirectory_,
            [&](const ProgressInfo &progress)
            {
                firstRun = progress;
            }));

    const auto destinationFileCount =
        std::distance(
            std::filesystem::recursive_directory_iterator(
                destinationDirectory_),
            std::filesystem::recursive_directory_iterator());

    EXPECT_EQ(
        firstRun.processedFiles,
        1u);

    ProgressInfo secondRun;

    ASSERT_TRUE(
        engine.run(
            sourceDirectory_,
            destinationDirectory_,
            [&](const ProgressInfo &progress)
            {
                secondRun = progress;
            }));

    const auto destinationFileCountAfterSecondRun =
        std::distance(
            std::filesystem::recursive_directory_iterator(
                destinationDirectory_),
            std::filesystem::recursive_directory_iterator());

    EXPECT_EQ(
        destinationFileCountAfterSecondRun,
        destinationFileCount);

    EXPECT_EQ(
        secondRun.totalFiles,
        1u);

    EXPECT_EQ(
        secondRun.processedFiles,
        0u);

    EXPECT_EQ(
        secondRun.skippedFiles,
        1u);

    EXPECT_EQ(
        secondRun.failedFiles,
        0u);
}

TEST_F(
    SortEngineTest,
    ProcessesMultipleFilesAndCountsFailures)
{
    const auto firstFile =
        sourceDirectory_ / "photo1.jpg";

    const auto secondFile =
        sourceDirectory_ / "photo2.jpg";

    const auto thirdFile =
        sourceDirectory_ / "photo3.jpg";

    createValidJpeg(firstFile);
    createValidJpeg(secondFile);

    addCaptureDate(
        firstFile,
        "2024:07:15 14:30:45");

    addCaptureDate(
        secondFile,
        "2024:08:20 10:15:30");

    createFile(
        thirdFile,
        "not a valid jpeg");

    Config config = createConfig();

    Logger logger(logPath_);

    SortEngine engine(
        config,
        logger);

    ProgressInfo finalProgress;

    ASSERT_TRUE(
        engine.run(
            sourceDirectory_,
            destinationDirectory_,
            [&](const ProgressInfo &progress)
            {
                finalProgress = progress;
            }));

    EXPECT_EQ(
        finalProgress.totalFiles,
        3u);

    EXPECT_EQ(
        finalProgress.processedFiles,
        3u);

    EXPECT_EQ(
        finalProgress.skippedFiles,
        0u);

    EXPECT_EQ(
        finalProgress.failedFiles,
        0u);
}

TEST_F(
    SortEngineTest,
    SkipsDuplicateFilesWithinSameRun)
{
    const auto firstFile =
        sourceDirectory_ / "photo1.jpg";

    const auto secondFile =
        sourceDirectory_ / "photo2.jpg";

    createValidJpeg(firstFile);

    addCaptureDate(
        firstFile,
        "2024:07:15 14:30:45");

    std::filesystem::copy_file(
        firstFile,
        secondFile);

    Config config = createConfig();

    Logger logger(logPath_);

    SortEngine engine(
        config,
        logger);

    ProgressInfo finalProgress;

    ASSERT_TRUE(
        engine.run(
            sourceDirectory_,
            destinationDirectory_,
            [&](const ProgressInfo &progress)
            {
                finalProgress = progress;
            }));

    EXPECT_EQ(
        finalProgress.totalFiles,
        2u);

    EXPECT_EQ(
        finalProgress.processedFiles,
        1u);

    EXPECT_EQ(
        finalProgress.skippedFiles,
        1u);

    EXPECT_EQ(
        finalProgress.failedFiles,
        0u);
}
