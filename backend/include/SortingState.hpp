#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>

#include "Models.hpp"

enum class SortingStatus
{
    Idle,
    Running,
    Cancelling,
    Completed,
    CompletedWithErrors,
    Failed,
    Cancelled
};

class SortingState
{
public:

    SortingState();

    void start();

    void requestCancellation();

    bool isCancellationRequested() const;

    void updateProgress(
        const ProgressInfo& progress
    );

    void complete();

    void fail();

    void cancel();

    SortingStatus getStatus() const;

    ProgressInfo getProgress() const;

private:
    
    std::atomic<SortingStatus> status_;

    std::atomic<bool> cancellationRequested_;

    mutable std::mutex progressMutex_;

    ProgressInfo progress_;
};