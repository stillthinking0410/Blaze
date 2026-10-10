#include "MetaDataReader.hpp"

#include "Process.hpp"

#include <array>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <vector>

namespace
{
    const std::vector<std::string> DATE_TAGS =
    {
        "DateTimeOriginal",
        "CreateDate",
        "MediaCreateDate",
        "CreationDate",
        "ModifyDate"
    };
}

MetaDataReader::MetaDataReader(
    const Config& config,
    Logger& logger
)
    : config_(config),
    logger_(logger)
{
}

std::optional<std::chrono::system_clock::time_point>
MetaDataReader::parseCaptureDate(
    const std::string& output
) const
{
    std::istringstream input(output);

    std::string line;

    while (std::getline(input, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::tm tm = {};

        std::istringstream stream(line);

        stream >> std::get_time(
            &tm,
            "%Y:%m:%d %H:%M:%S"
        );

        if (!stream.fail())
        {
            std::time_t time = std::mktime(&tm);

            return std::chrono::system_clock::from_time_t(time);
        }
    }

    return std::nullopt;
}

std::optional<std::chrono::system_clock::time_point>
MetaDataReader::getCaptureDate(
    const std::filesystem::path& file
) const
{
    logger_.debug(
        "Reading metadata: " +
        file.filename().string()
    );

    std::ostringstream command;

    command << "\""
        << config_.exifToolPath.string()
        << "\" -s3 ";

    for (const auto& tag : DATE_TAGS)
    {
        command << "-" << tag << " ";
    }

    command << "\"" << file.string() << "\"";

    std::array<char, 256> buffer;

    std::string result;

    std::string commandString = command.str();

    FILE* pipe =
        blaze_popen(commandString.c_str(), "r");

    if (!pipe)
    {
        logger_.error(
            "Failed to start ExifTool: " +
            file.filename().string()
        );

        return std::nullopt;
    }

    while(
        fgets(
            buffer.data(),
            buffer.size(),
            pipe
        ) != nullptr
    )
    {
        result += buffer.data();
    }

    int exitCode = blaze_pclose(pipe);

    if (exitCode != 0)
    {
        logger_.error(
            "ExifTool failed for: " +
            file.filename().string()
        );

        return std::nullopt;
    }

    if (!result.empty() &&
        result.back() == '\n')
    {
        result.pop_back();
    }

    if (result.empty())
    {
        logger_.warning(
            "No capture date found for: " +
            file.filename().string()
        );

        return std::nullopt;
    }

    auto captureDate = 
        parseCaptureDate(result);
    
    if (!captureDate)
    {
        logger_.warning(
            "Unable to parse capture date for: " +
            file.filename().string()
        );
    }

    return captureDate;
}