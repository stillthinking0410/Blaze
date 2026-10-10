#include "FileCopier.hpp"
#include "Logger.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    class FileCopierTest : public ::testing::Test
    {
    protected:
        
        std::filesystem::path testDirectory_;

        void SetUp() override
        {
            testDirectory_ = 
                std::filesystem::temp_directory_path()
                / "blaze_file_copier_test";
            
            std::filesystem::remove_all(
                testDirectory_
            );

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
            const std::filesystem::path& path,
            const std::string& content
        )
        {
            std::filesystem::create_directories(
                path.parent_path()
            );

            std::ofstream file(path);

            ASSERT_TRUE(file.is_open());

            file << content;
        }
    };
}


TEST_F(
    FileCopierTest,
    CopiesFileSuccessfully
)
{
    const auto source =
        testDirectory_ / "source.jpg";

    const auto destinationDirectory =
        testDirectory_ / "destination";

    createFile(
        source,
        "Blaze test content"
    );

    Logger logger(
        testDirectory_ / "copier.log"
    );

    FileCopier copier(logger);

    const auto result =
        copier.copyFile(
            source,
            destinationDirectory,
            "photo.jpg"
        );

    EXPECT_EQ(
        result,
        destinationDirectory / "photo.jpg"
    );

    ASSERT_TRUE(
        std::filesystem::exists(result)
    );

    std::ifstream copiedFile(result);

    ASSERT_TRUE(copiedFile.is_open());

    std::string content;

    std::getline(
        copiedFile,
        content
    );

    EXPECT_EQ(
        content,
        "Blaze test content"
    );
}

TEST_F(
    FileCopierTest,
    CreatesDestinationDirectory
)
{
    const auto source =
        testDirectory_ / "source.jpg";

    const auto destinationDirectory =
        testDirectory_
        / "nested"
        / "destination";

    createFile(
        source,
        "Test content"
    );

    Logger logger(
        testDirectory_ / "copier.log"
    );

    FileCopier copier(logger);

    const auto result =
        copier.copyFile(
            source,
            destinationDirectory,
            "photo.jpg"
        );

    EXPECT_TRUE(
        std::filesystem::exists(
            destinationDirectory
        )
    );

    EXPECT_TRUE(
        std::filesystem::exists(result)
    );
}

TEST_F(
    FileCopierTest,
    CopiedFileHasSameContent
)
{
    const auto source =
        testDirectory_ / "video.mov";

    const auto destinationDirectory =
        testDirectory_ / "destination";

    const std::string originalContent =
        "This represents video file data.";

    createFile(
        source,
        originalContent
    );

    Logger logger(
        testDirectory_ / "copier.log"
    );

    FileCopier copier(logger);

    const auto result =
        copier.copyFile(
            source,
            destinationDirectory,
            "video.mov"
        );

    std::ifstream file(result);

    ASSERT_TRUE(file.is_open());

    std::istreambuf_iterator<char> begin(file);
    std::istreambuf_iterator<char> end;

    std::string copiedContent(
        begin,
        end
    );

    // Alternative initialization syntax:
    // std::string copiedContent{
    //     std::istreambuf_iterator<char>(file),
    //     std::istreambuf_iterator<char>()
    // };

    EXPECT_EQ(
        copiedContent,
        originalContent
    );
}

TEST_F(
    FileCopierTest,
    ThrowsWhenSourceDoesNotExist
)
{
    const auto source =
        testDirectory_ / "missing.jpg";

    const auto destinationDirectory =
        testDirectory_ / "destination";

    Logger logger(
        testDirectory_ / "copier.log"
    );

    FileCopier copier(logger);

    EXPECT_THROW(
        copier.copyFile(
            source,
            destinationDirectory,
            "photo.jpg"
        ),
        std::exception
    );
}