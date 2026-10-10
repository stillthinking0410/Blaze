#include "Database.hpp"
#include "Logger.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace
{
    class DatabaseTest : public ::testing::Test
    {
    protected:
        
        std::filesystem::path testDirectory_;

        std::filesystem::path databasePath_;

        std::filesystem::path logPath_;

        void SetUp() override
        {
            testDirectory_ = 
                std::filesystem::temp_directory_path()
                / "blaze_database_test";
            
            std::filesystem::remove_all(
                testDirectory_
            );

            std::filesystem::create_directories(
                testDirectory_
            );

            databasePath_ = 
                testDirectory_
                / "database"
                / "test.db";
            
            logPath_ = 
                testDirectory_
                / "database.log";
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
    DatabaseTest,
    InitializesSuccessfully
)
{
    Logger logger(logPath_);

    Database database(
        databasePath_,
        logger
    );

    EXPECT_TRUE(
        database.initialize()
    );

    EXPECT_TRUE(
        std::filesystem::exists(
            databasePath_
        )
    );
}

TEST_F(
    DatabaseTest,
    NewHashIsNotProcessed
)
{
    Logger logger(logPath_);

    Database database(
        databasePath_,
        logger
    );

    ASSERT_TRUE(
        database.initialize()
    );

    EXPECT_FALSE(
        database.isProcessed(
            "test-hash-123"
        )
    );
}

TEST_F(
    DatabaseTest,
    AddsProcessedFileSuccessfully
)
{
    Logger logger(logPath_);

    Database database(
        databasePath_,
        logger
    );

    ASSERT_TRUE(
        database.initialize()
    );

    const bool result =
        database.addProcessed(
            "hash-123",
            "/source/photo.jpg",
            "/destination/2024/07/photo.jpg"
        );

    EXPECT_TRUE(result);
}

TEST_F(
    DatabaseTest,
    AddedHashIsProcessed
)
{
    Logger logger(logPath_);

    Database database(
        databasePath_,
        logger
    );

    ASSERT_TRUE(
        database.initialize()
    );

    ASSERT_TRUE(
        database.addProcessed(
            "hash-123",
            "/source/photo.jpg",
            "/destination/photo.jpg"
        )
    );

    EXPECT_TRUE(
        database.isProcessed(
            "hash-123"
        )
    );
}

TEST_F(
    DatabaseTest,
    DifferentHashIsNotProcessed
)
{
    Logger logger(logPath_);

    Database database(
        databasePath_,
        logger
    );

    ASSERT_TRUE(
        database.initialize()
    );

    ASSERT_TRUE(
        database.addProcessed(
            "hash-123",
            "/source/photo.jpg",
            "/destination/photo.jpg"
        )
    );

    EXPECT_FALSE(
        database.isProcessed(
            "different-hash"
        )
    );
}

TEST_F(
    DatabaseTest,
    DuplicateHashIsRejected
)
{
    Logger logger(logPath_);

    Database database(
        databasePath_,
        logger
    );

    ASSERT_TRUE(
        database.initialize()
    );

    ASSERT_TRUE(
        database.addProcessed(
            "hash-123",
            "/source/photo1.jpg",
            "/destination/photo1.jpg"
        )
    );

    EXPECT_FALSE(
        database.addProcessed(
            "hash-123",
            "/source/photo2.jpg",
            "/destination/photo2.jpg"
        )
    );
}

TEST_F(
    DatabaseTest,
    DataPersistsAfterReopeningDatabase
)
{
    Logger logger(logPath_);

    {
        Database database(
            databasePath_,
            logger
        );

        ASSERT_TRUE(
            database.initialize()
        );

        ASSERT_TRUE(
            database.addProcessed(
                "persistent-hash",
                "/source/photo.jpg",
                "/destination/photo.jpg"
            )
        );
    }

    Database reopenedDatabase(
        databasePath_,
        logger
    );

    ASSERT_TRUE(
        reopenedDatabase.initialize()
    );

    EXPECT_TRUE(
        reopenedDatabase.isProcessed(
            "persistent-hash"
        )
    );
}

TEST_F(
    DatabaseTest,
    StoresMultipleProcessedFiles
)
{
    Logger logger(logPath_);

    Database database(
        databasePath_,
        logger
    );

    ASSERT_TRUE(
        database.initialize()
    );

    ASSERT_TRUE(
        database.addProcessed(
            "hash-1",
            "/source/photo1.jpg",
            "/destination/photo1.jpg"
        )
    );

    ASSERT_TRUE(
        database.addProcessed(
            "hash-2",
            "/source/photo2.jpg",
            "/destination/photo2.jpg"
        )
    );

    ASSERT_TRUE(
        database.addProcessed(
            "hash-3",
            "/source/photo3.jpg",
            "/destination/photo3.jpg"
        )
    );

    EXPECT_TRUE(
        database.isProcessed("hash-1")
    );

    EXPECT_TRUE(
        database.isProcessed("hash-2")
    );

    EXPECT_TRUE(
        database.isProcessed("hash-3")
    );

    EXPECT_FALSE(
        database.isProcessed("hash-4")
    );
}