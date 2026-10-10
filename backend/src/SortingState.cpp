#include "SortingState.hpp"

SortingState::SortingState()
    : status_(SortingStatus::Idle),
    cancellationRequested_(false)
{
}

void SortingState::start()
{
    cancellationRequested_.store(false);

    status_.store(
        SortingStatus::Running
    );
}

void SortingState::requestCancellation()
{
    cancellationRequested_.store(true);

    status_.store(
        SortingStatus::Cancelling
    );
}

bool SortingState::isCancellationRequested() const
{
    return cancellationRequested_.load();
}

void SortingState::updateProgress(
    const ProgressInfo& progress
)
{
    std::lock_guard<std::mutex> lock(
        progressMutex_
    );

    progress_ = progress;
}

void SortingState::complete()
{
    status_.store(
        SortingStatus::Completed
    );
}

void SortingState::fail()
{
    status_.store(
        SortingStatus::Failed
    );
}

void SortingState::cancel()
{
    status_.store(
        SortingStatus::Cancelled
    );
}

SortingStatus SortingState::getStatus() const
{
    return status_.load();
}

ProgressInfo SortingState::getProgress() const
{
    std::lock_guard<std::mutex> lock(
        progressMutex_
    );

    return progress_;
}