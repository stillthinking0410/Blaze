#pragma once

#include <filesystem>
#include <string>

#include "Logger.hpp"

struct sqlite3;

class Database {
 public:
  explicit Database(const std::filesystem::path& databasePath, Logger& logger);

  ~Database();

  bool initialize();

  bool isProcessed(const std::string& hash) const;

  bool addProcessed(const std::string& hash, const std::filesystem::path& sourcePath,
                    const std::filesystem::path& destinationPath);

 private:
  std::filesystem::path databasePath_;

  Logger& logger_;

  sqlite3* database_ = nullptr;
};
