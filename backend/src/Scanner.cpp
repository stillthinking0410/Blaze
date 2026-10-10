#include "Scanner.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

Scanner::Scanner(
    Logger &logger)
    : logger_(logger)
{
}

bool Scanner::isSupportedFile(
    const std::filesystem::path &file) const
{
    if (!file.has_extension())
    {
        return false;
    }

    std::string extension = file.extension().string();

    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });

    return extension == ".heic" ||
           extension == ".jpg" ||
           extension == ".jpeg" ||
           extension == ".png" ||
           extension == ".mov" ||
           extension == ".mp4";
}

std::vector<std::filesystem::path>
Scanner::scan(
    const std::filesystem::path &root) const
{
    std::vector<std::filesystem::path> files;

    logger_.info(
        "Starting scan: " +
        root.string());

    if (!std::filesystem::exists(root))
    {
        logger_.error(
            "Source folder does not exist: " +
            root.string());

        throw std::runtime_error(
            "Source folder does not exist: " +
            root.string());
    }

    if (!std::filesystem::is_directory(root))
    {
        logger_.error(
            "Path is not a directory: " +
            root.string());

        throw std::runtime_error(
            "Path is not a directory: " +
            root.string());
    }

    try
    {
        for (const auto &entry :
             std::filesystem::recursive_directory_iterator(
                 root,
                 std::filesystem::directory_options::skip_permission_denied))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            if (isSupportedFile(entry.path()))
            {
                files.push_back(entry.path());
            }
        }

        std::sort(files.begin(), files.end());
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        logger_.error(
            "Filesystem error while scanning: " +
            std::string(e.what())
        );

        throw;
    }

    logger_.info(
        "Scan completed. Files found: "+
        std::to_string(files.size())
    );

    return files;
}