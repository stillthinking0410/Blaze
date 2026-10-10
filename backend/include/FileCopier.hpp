#pragma once

#include "Logger.hpp"

#include <filesystem>
#include <string>

class FileCopier
{
    public:

        explicit FileCopier(
            Logger& logger
        );

        std::filesystem::path copyFile(
            const std::filesystem::path& source,
            const std::filesystem::path& destinationFolder,
            const std::string& targetFileName
        ) const;
    
    private:

        Logger& logger_;
        
        std::filesystem::path generateUniquePath(
            const std::filesystem::path& destinationFolder,
            const std::string& targetFileName
        ) const;
};