#pragma once

#include <chrono>
#include <filesystem>
#include <string>

class DestinationPathGenerator
{
public:

    std::filesystem::path generateFolderPath(
        const std::filesystem::path& destinationRoot,
        const std::chrono::system_clock::time_point& captureDate
    ) const;

    std::string generateFileName(
        const std::chrono::system_clock::time_point& captureDate,
        const std::string& extension
    ) const;

};