#include "Logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

Logger::Logger(
    const std::filesystem::path& logFile
)
    : logFilePath_(logFile)
{
}

bool Logger::initialize()
{
    try
    {
        std::filesystem::create_directories(
            logFilePath_.parent_path()
        );

        logFile_.open(
            logFilePath_,
            std::ios::app
        );

        return logFile_.is_open();
    }
    catch(const std::exception& e)
    {
        return false;
    }
}

void Logger::debug(
    const std::string& message
)
{
    log(
        LogLevel::Debug,
        message
    );
}

void Logger::info(
    const std::string& message
)
{
    log(
        LogLevel::Info,
        message
    );
}

void Logger::warning(
    const std::string& message
)
{
    log(
        LogLevel::Warning,
        message
    );
}

void Logger::error(
    const std::string& message
)
{
    log(
        LogLevel::Error,
        message
    );
}


void Logger::log(
    LogLevel level,
    const std::string& message
)
{
    if (!logFile_.is_open())
    {
        return;
    }
    
    logFile_
        << getCurrentTimestamp()
        << " ["
        << logLevelToString(level)
        << "] "
        << message 
        << '\n';
    
    logFile_.flush();
}

std::string
Logger::logLevelToString(
    LogLevel level
) const
{
    switch (level)
    {
        case LogLevel::Debug:
            return "DEBUG";
        
        case LogLevel::Info:
            return "INFO";

        case LogLevel::Warning:
            return "WARNING";
        
        case LogLevel::Error:
            return "ERROR";
    }

    return "UNKNOWN";
}

std::string
Logger::getCurrentTimestamp() const
{
    auto now = 
        std::chrono::system_clock::now();

    std::time_t currentTime = 
        std::chrono::system_clock::to_time_t(now);
    
    std::tm localTime = 
        *std::localtime(&currentTime);

    std::ostringstream timestamp;

    timestamp
        << std::put_time(
            &localTime,
            "%Y-%m-%d %H:%M:%S"
        );

    return timestamp.str();
}