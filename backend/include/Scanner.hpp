#pragma once

#include "Logger.hpp"

#include <filesystem>
#include <vector>

class Scanner
{
public:

    explicit Scanner(
        Logger& logger
    );

    std::vector<std::filesystem::path>
    scan(const std::filesystem::path& root) const;

private:

    Logger& logger_;
    
    bool isSupportedFile(
        const std::filesystem::path& file
    ) const;
};