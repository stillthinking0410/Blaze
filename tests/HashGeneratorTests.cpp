#include "HashGenerator.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    class HashGeneratorTest : public  ::testing::Test
    {
        protected:

            void TearDown() override
            {
                if (std::filesystem::exists(testFile_))
                {
                    std::filesystem::remove(testFile_);
                }
            }

            std::filesystem::path testFile_ = 
                std::filesystem::temp_directory_path()
                / "blaze_hash_test.txt";
    };
}

TEST_F(
    HashGeneratorTest,
    GeneratesCorrectSHA256Hash
)
{
    std::ofstream file(testFile_);

    ASSERT_TRUE(file.is_open());

    file << "hello";

    file.close();

    HashGenerator generator;

    const std::string hash =
        generator.generateHash(testFile_);

    EXPECT_EQ(
        hash,
        "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"
    );
}

TEST_F(
    HashGeneratorTest,
    SameContentProducesSameHash
)
{
    std::ofstream file(testFile_);

    ASSERT_TRUE(file.is_open());

    file << "Blaze test content";

    file.close();

    HashGenerator generator;

    const std::string firstHash =
        generator.generateHash(testFile_);

    const std::string secondHash =
        generator.generateHash(testFile_);

    EXPECT_EQ(
        firstHash,
        secondHash
    );
}

TEST_F(
    HashGeneratorTest,
    DifferentContentProducesDifferentHash
)
{
    const auto firstFile =
        std::filesystem::temp_directory_path()
        / "blaze_hash_test_1.txt";

    const auto secondFile =
        std::filesystem::temp_directory_path()
        / "blaze_hash_test_2.txt";

    
    std::filesystem::remove(firstFile);
    std::filesystem::remove(secondFile);

    {
        std::ofstream file(firstFile);

        ASSERT_TRUE(file.is_open());

        file << "First content";
    }

    {
        std::ofstream file(secondFile);

        ASSERT_TRUE(file.is_open());

        file << "Second content";
    }

    HashGenerator generator;

    const std::string firstHash =
        generator.generateHash(firstFile);

    const std::string secondHash =
        generator.generateHash(secondFile);

    EXPECT_NE(
        firstHash,
        secondHash
    );

    std::filesystem::remove(firstFile);
    std::filesystem::remove(secondFile);
}

TEST_F(
    HashGeneratorTest,
    ThrowsForMissingFile
)
{
    HashGenerator generator;

    const auto missingFile =
        std::filesystem::temp_directory_path()
        / "blaze_file_that_does_not_exist.txt";
    
    std::filesystem::remove(missingFile);

    EXPECT_THROW(
        generator.generateHash(missingFile),
        std::runtime_error
    );
}

TEST_F(
    HashGeneratorTest,
    GeneratesHashForEmptyFile
)
{
    std::ofstream file(testFile_);

    ASSERT_TRUE(file.is_open());

    file.close();

    HashGenerator generator;

    const std::string hash =
        generator.generateHash(testFile_);

    EXPECT_EQ(
        hash,
        "e3b0c44298fc1c149afbf4c8996fb924"
        "27ae41e4649b934ca495991b7852b855"
    );
}