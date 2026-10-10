#include "SortingController.hpp"

#include <utility>

SortingController::SortingController(
    const Config& config,
    Logger& logger
)
    : config_(config),
      logger_(logger),
      engine_(config, logger),
      sortingOperation_(
          [this](
              const std::filesystem::path& sourceFolder,
              const std::filesystem::path& destinationFolder,
              ProgressCallback progressCallback,
              CancellationCallback cancellationCallback
          )
          {
              return engine_.run(
                  sourceFolder,
                  destinationFolder,
                  progressCallback,
                  cancellationCallback
              );
          }
      )
{
}

SortingController::SortingController(
    const Config& config,
    Logger& logger,
    SortingOperation sortingOperation
)
    : config_(config),
      logger_(logger),
      engine_(config, logger),
      sortingOperation_(std::move(sortingOperation))
{
}

SortingController::~SortingController()
{
    std::lock_guard<std::mutex> lock(
        controllerMutex_
    );

    if (worker_.joinable())
    {
        worker_.join();
    }
}

bool SortingController::start()
{
    std::lock_guard<std::mutex> lock(
        controllerMutex_
    );

    const SortingStatus status =
        state_.getStatus();

    if (status == SortingStatus::Running ||
        status == SortingStatus::Cancelling)
    {
        return false;
    }

    if (worker_.joinable())
    {
        worker_.join();
    }

    state_.start();

    worker_ = std::thread(
        &SortingController::runSorting,
        this
    );

    return true;
}

void SortingController::runSorting()
{
    const bool success =
        sortingOperation_(
            config_.sourceFolder,
            config_.destinationFolder,

            [this](const ProgressInfo& progress)
            {
                state_.updateProgress(progress);
            },

            [this]()
            {
                return state_.isCancellationRequested();
            }
        );

    if (state_.isCancellationRequested())
    {
        state_.cancel();
    }
    else if (success)
    {
        state_.complete();
    }
    else
    {
        state_.fail();
    }
}

void SortingController::requestCancellation()
{
    state_.requestCancellation();
}

SortingStatus SortingController::getStatus() const
{
    return state_.getStatus();
}

ProgressInfo SortingController::getProgress() const
{
    return state_.getProgress();
}