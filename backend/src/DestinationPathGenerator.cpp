#include "DestinationPathGenerator.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

std::filesystem::path
DestinationPathGenerator::generateFolderPath(
    const std::filesystem::path& destinationRoot,
    const std::chrono::system_clock::time_point& captureDate
) const
{
    std::time_t time =
        std::chrono::system_clock::to_time_t(captureDate);

    std::tm localTime = *std::localtime(&time);

    std::ostringstream yearStream;
    yearStream << std::put_time(&localTime, "%Y");

    std::ostringstream monthStream;
    monthStream << std::put_time(&localTime, "%B");

    return destinationRoot /
           yearStream.str() /
           monthStream.str();
}

std::string
DestinationPathGenerator::generateFileName(
    const std::chrono::system_clock::time_point& captureDate,
    const std::string& extension
) const
{
    std::time_t time =
        std::chrono::system_clock::to_time_t(captureDate);

    std::tm localTime = *std::localtime(&time);

    std::ostringstream fileName;

    fileName
        << std::put_time(
               &localTime,
               "%Y-%m-%d_%H-%M-%S")
        << extension;

    return fileName.str();
}
