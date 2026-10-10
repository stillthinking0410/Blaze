#pragma once

#include <string>
#include <chrono>
#include <filesystem>

struct MediaFile 
{
    std::string sourcePath;
    std::string destinationPath;
    std::string hash;
    std::string extension;

    std::chrono::system_clock::time_point captureDate;
};

struct ProgressInfo
{
    std::size_t totalFiles {0};
    std::size_t processedFiles {0};
    std::size_t skippedFiles {0};
    std::size_t failedFiles{0};
    std::filesystem::path currentFile;

    double percentage() const
    {
        if (totalFiles == 0)
        {
            return 0.0;
        }

        return static_cast<double>(
            processedFiles + 
            skippedFiles +
            failedFiles
        ) * 100.0 / totalFiles;
    }
};