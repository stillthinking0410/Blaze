#pragma once

#include "Config.hpp"
#include "Logger.hpp"

#include <chrono>
#include <filesystem>
#include <optional>

class MetaDataReader
{
public:

    explicit MetaDataReader(
        const Config& config,
        Logger& logger
    );

    std::optional<std::chrono::system_clock::time_point>
    getCaptureDate(
        const std::filesystem::path& file
    ) const;

private:

    const Config& config_;

    Logger& logger_;

    std::optional<std::chrono::system_clock::time_point>
    parseCaptureDate(
        const std::string& output
    ) const;
};