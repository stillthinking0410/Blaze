#pragma once

#include <functional>
#include <mutex>
#include <thread>

#include "Config.hpp"
#include "Logger.hpp"
#include "Models.hpp"
#include "SortEngine.hpp"
#include "SortingState.hpp"

using SortingOperation =
    std::function<bool(
        const std::filesystem::path&,
        const std::filesystem::path&,
        ProgressCallback,
        CancellationCallback
    )>;

class SortingController
{
public:

    SortingController(
        const Config& config,
        Logger& logger
    );

    SortingController(
        const Config& config,
        Logger& logger,
        SortingOperation sortingOperation
    );

    ~SortingController();

    bool start();

    void requestCancellation();

    SortingStatus getStatus() const;

    ProgressInfo getProgress() const;

private:

    void runSorting();

    const Config& config_;

    Logger& logger_;

    SortEngine engine_;

    SortingOperation sortingOperation_;

    SortingState state_;

    mutable std::mutex controllerMutex_;

    std::thread worker_;
};