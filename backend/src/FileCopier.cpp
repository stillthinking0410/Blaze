#include "FileCopier.hpp"

#include <filesystem>

FileCopier::FileCopier(
    Logger& logger
)
    : logger_(logger)
{
}

std::filesystem::path
FileCopier::copyFile(
    const std::filesystem::path& source,
    const std::filesystem::path& destinationFolder,
    const std::string& targetFileName
) const
{
    logger_.debug(
        "Copying file: " +
        source.filename().string()
    );

    std::filesystem::create_directories(
        destinationFolder
    );

    auto destination = 
        generateUniquePath(
            destinationFolder,
            targetFileName
        );
    
    logger_.debug(
        "Copy destination: " +
        destination.string()
    );
    
    std::filesystem::copy_file(
        source,
        destination,
        std::filesystem::copy_options::none
    );

    logger_.info(
        "File Copied successfully: " +
        source.filename().string()
    );

    return destination;
}

std::filesystem::path
FileCopier::generateUniquePath(
    const std::filesystem::path& destinationFolder,
    const std::string& targetFileName
) const
{
    auto destination = 
        destinationFolder / targetFileName;
    
    if(!std::filesystem::exists(destination))
    {
        return destination;
    }

    const auto original = std::filesystem::path(targetFileName);

    const auto stem = original.stem().string();
    const auto extension = original.extension().string();
    
    int counter = 1;
    
    while (true)
    {
        auto candidate = 
            destinationFolder /
            (stem + "_" +
            std::to_string(counter)+
            extension);
        
        if(!std::filesystem::exists(candidate))
        {
            return candidate;
        }

        ++counter;
    }
}
