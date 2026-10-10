#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>

#include "SortingController.hpp"

class SortingControllerTest
    : public ::testing::Test
{
protected:

    void SetUp() override
    {
        testDirectory =
            std::filesystem::temp_directory_path()
            / "blaze_sorting_controller_tests";

        std::filesystem::remove_all(testDirectory);

        std::filesystem::create_directories(
            testDirectory
        );

        config.sourceFolder =
            testDirectory / "source";

        config.destinationFolder =
            testDirectory / "destination";

        config.databasePath =
            testDirectory / "database" / "blaze.db";

        config.logFile =
            testDirectory / "blaze.log";

        std::filesystem::create_directories(
            config.sourceFolder
        );
    }

    void TearDown() override
    {
        std::filesystem::remove_all(
            testDirectory
        );
    }

    Config config;

    Logger logger{
        std::filesystem::temp_directory_path()
        / "blaze_controller_test.log"
    };

    std::filesystem::path testDirectory;
};

TEST_F(
    SortingControllerTest,
    StartReturnsTrue)
{
    SortingOperation operation =
        [](
            const auto&,
            const auto&,
            auto,
            auto
        )
        {
            return true;
        };

    SortingController controller(
        config,
        logger,
        operation
    );

    EXPECT_TRUE(controller.start());
}

TEST_F(
    SortingControllerTest,
    WorkerCompletesSuccessfully)
{
    SortingOperation operation =
        [](
            const auto&,
            const auto&,
            auto,
            auto
        )
        {
            return true;
        };

    SortingController controller(
        config,
        logger,
        operation
    );

    ASSERT_TRUE(controller.start());

    for (int i = 0; i < 100; ++i)
    {
        if (controller.getStatus() ==
            SortingStatus::Completed)
        {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    EXPECT_EQ(
        controller.getStatus(),
        SortingStatus::Completed
    );
}

TEST_F(
    SortingControllerTest,
    CancellationStopsOperation)
{
    std::mutex mutex;
    std::condition_variable condition;

    bool started = false;

    SortingOperation operation =
        [&](
            const auto&,
            const auto&,
            auto,
            auto cancellationCallback
        )
        {
            {
                std::lock_guard<std::mutex> lock(mutex);
                started = true;
            }

            condition.notify_one();

            while (!cancellationCallback())
            {
                std::this_thread::yield();
            }

            return true;
        };

    SortingController controller(
        config,
        logger,
        operation
    );

    ASSERT_TRUE(controller.start());

    {
        std::unique_lock<std::mutex> lock(mutex);

        ASSERT_TRUE(
            condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&]()
                {
                    return started;
                }
            )
        );
    }

    controller.requestCancellation();

    for (int i = 0; i < 100; ++i)
    {
        if (controller.getStatus() ==
            SortingStatus::Cancelled)
        {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    EXPECT_EQ(
        controller.getStatus(),
        SortingStatus::Cancelled
    );
}

TEST_F(
    SortingControllerTest,
    CannotStartWhileAlreadyRunning)
{
    std::mutex mutex;
    std::condition_variable condition;

    bool started = false;

    SortingOperation operation =
        [&](
            const auto&,
            const auto&,
            auto,
            auto cancellationCallback
        )
        {
            {
                std::lock_guard<std::mutex> lock(mutex);
                started = true;
            }

            condition.notify_one();

            while (!cancellationCallback())
            {
                std::this_thread::yield();
            }

            return true;
        };

    SortingController controller(
        config,
        logger,
        operation
    );

    ASSERT_TRUE(controller.start());

    {
        std::unique_lock<std::mutex> lock(mutex);

        ASSERT_TRUE(
            condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&]()
                {
                    return started;
                }
            )
        );
    }

    EXPECT_FALSE(
        controller.start()
    );

    controller.requestCancellation();
}

TEST_F(
    SortingControllerTest,
    FailedOperationSetsFailedStatus)
{
    SortingOperation operation =
        [](
            const auto&,
            const auto&,
            auto,
            auto
        )
        {
            return false;
        };

    SortingController controller(
        config,
        logger,
        operation
    );

    ASSERT_TRUE(controller.start());

    for (int i = 0; i < 100; ++i)
    {
        if (controller.getStatus() ==
            SortingStatus::Failed)
        {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    EXPECT_EQ(
        controller.getStatus(),
        SortingStatus::Failed
    );
}

TEST_F(
    SortingControllerTest,
    ProgressIsStoredInState)
{
    SortingOperation operation =
        [](
            const auto&,
            const auto&,
            auto progressCallback,
            auto
        )
        {
            ProgressInfo progress;

            progress.totalFiles = 10;
            progress.processedFiles = 6;
            progress.skippedFiles = 2;
            progress.failedFiles = 1;
            progress.currentFile = "photo.jpg";

            progressCallback(progress);

            return true;
        };

    SortingController controller(
        config,
        logger,
        operation
    );

    ASSERT_TRUE(controller.start());

    for (int i = 0; i < 100; ++i)
    {
        if (controller.getStatus() ==
            SortingStatus::Completed)
        {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    const ProgressInfo progress =
        controller.getProgress();

    EXPECT_EQ(progress.totalFiles, 10);
    EXPECT_EQ(progress.processedFiles, 6);
    EXPECT_EQ(progress.skippedFiles, 2);
    EXPECT_EQ(progress.failedFiles, 1);
    EXPECT_EQ(progress.currentFile, "photo.jpg");
    EXPECT_EQ(progress.percentage(), 90);
}
