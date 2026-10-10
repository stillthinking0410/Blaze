#include "Scanner.hpp"
#include "Logger.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
    class ScannerTest : public ::testing::Test
    {
    protected:
        
        std::filesystem::path testDirectory_;

        void SetUp() override
        {
            testDirectory_ = 
                std::filesystem::temp_directory_path()
                / "blaze_scanner_test";

            std::filesystem::remove_all(testDirectory_);

            std::filesystem::create_directories(
            testDirectory_
            );
        }

        void TearDown() override
        {
            std::filesystem::remove_all(
                testDirectory_
            );
        }

        void createFile(
            const std::filesystem::path& path
        )
        {
            std::filesystem::create_directories(
                path.parent_path()
            );

            std::ofstream file(path);

            ASSERT_TRUE(file.is_open());
        }
    };
}

TEST_F(
    ScannerTest,
    ScansFilesRecursively)
{
    createFile(
        testDirectory_ / "photo.jpg"
    );

    createFile(
        testDirectory_ / "folder1" / "photo2.jpg"
    );

    createFile(
        testDirectory_ / "folder1" / "folder2" / "video.mov"
    );

    Logger logger(
        testDirectory_ / "scanner.log"
    );

    Scanner scanner(logger);

    auto files =
        scanner.scan(testDirectory_);

    ASSERT_EQ(
        files.size(),
        3
    );
}

TEST_F(
    ScannerTest,
    DetectsSupportedExtensionsCaseInsensitively)
{
    createFile(testDirectory_ / "photo.jpg");
    createFile(testDirectory_ / "photo.JPEG");
    createFile(testDirectory_ / "photo.HEIC");
    createFile(testDirectory_ /"photo.PNG");
    createFile(testDirectory_ / "video.MOV");
    createFile(testDirectory_ / "video.Mp4");

    Logger logger(
        testDirectory_ / "scanner.log"
    );

    Scanner scanner(logger);

    auto files =
        scanner.scan(testDirectory_);

    EXPECT_EQ(
        files.size(),
        6
    );
}

TEST_F(
    ScannerTest,
    IgnoresUnsupportedFiles)
{
    createFile(testDirectory_ / "photo.jpg");
    createFile(testDirectory_ / "document.pdf");
    createFile(testDirectory_ / "text.txt");
    createFile(testDirectory_ / "archive.zip");

    Logger logger(
        testDirectory_ / "scanner.log"
    );

    Scanner scanner(logger);

    auto files =
        scanner.scan(testDirectory_);

    ASSERT_EQ(
        files.size(),
        1
    );

    EXPECT_EQ(
        files[0].filename(),
        "photo.jpg"
    );
}

TEST_F(
    ScannerTest,
    ReturnsFilesInSortedOrder)
{
    createFile(testDirectory_ / "C.jpg");
    createFile(testDirectory_ / "A.jpg");
    createFile(testDirectory_ / "B.jpg");

    Logger logger(
        testDirectory_ / "scanner.log"
    );

    Scanner scanner(logger);

    auto files =
        scanner.scan(testDirectory_);

    ASSERT_EQ(
        files.size(),
        3
    );

    EXPECT_EQ(
        files[0].filename(),
        "A.jpg"
    );

    EXPECT_EQ(
        files[1].filename(),
        "B.jpg"
    );

    EXPECT_EQ(
        files[2].filename(),
        "C.jpg"
    );
}

TEST_F(
    ScannerTest,
    ReturnsEmptyVectorForEmptyDirectory)
{
    Logger logger(
        testDirectory_ / "scanner.log"
    );

    Scanner scanner(logger);

    auto files =
        scanner.scan(testDirectory_);

    EXPECT_TRUE(
        files.empty()
    );
}

TEST_F(
    ScannerTest,
    ThrowsWhenDirectoryDoesNotExist)
{
    const auto missingDirectory =
        testDirectory_ / "does_not_exist";

    Logger logger(
        testDirectory_ / "scanner.log"
    );

    Scanner scanner(logger);

    EXPECT_THROW(
        scanner.scan(missingDirectory),
        std::runtime_error
    );
}

TEST_F(
    ScannerTest,
    ThrowsWhenPathIsAFile)
{
    const auto file =
        testDirectory_ / "photo.jpg";

    createFile(file);

    Logger logger(
        testDirectory_ / "scanner.log"
    );

    Scanner scanner(logger);

    EXPECT_THROW(
        scanner.scan(file),
        std::runtime_error
    );
}
