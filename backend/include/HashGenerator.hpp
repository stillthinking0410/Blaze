#pragma once

#include <filesystem>
#include <string>

class HashGenerator
{
    public:

        std::string generateHash(
            const std::filesystem::path& file
        ) const;
};