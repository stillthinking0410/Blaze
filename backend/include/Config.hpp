#pragma once

#include <filesystem>
#include <string>

struct Config
{
    std::filesystem::path sourceFolder;

    std::filesystem::path destinationFolder;

    std::filesystem::path databasePath = 
    "database/blaze.db";

    std::filesystem::path exifToolPath = 
    "exiftool";

    std::filesystem::path logFile = 
    "logs/blaze.log";

    unsigned int maxThreads = 4;
};