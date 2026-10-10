#pragma once

#include <filesystem>
#include <fstream>
#include <string>

enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

class Logger
{
public:

    explicit Logger(
        const std::filesystem::path& logFile
    );

    bool initialize();

    void debug(const std::string& message);

    void info(const std::string& message);

    void warning(const std::string& message);

    void error(const std::string& message);

private:

    void log(
        LogLevel level,
        const std::string& message
    );

    std::string logLevelToString(
        LogLevel level
    ) const;

    std::string getCurrentTimestamp() const;

    std::filesystem::path logFilePath_;

    std::ofstream logFile_;
};