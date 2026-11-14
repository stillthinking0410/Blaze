#pragma once

#include <filesystem>
#include <functional>

#include "Config.hpp"
#include "Database.hpp"
#include "DestinationPathGenerator.hpp"
#include "FileCopier.hpp"
#include "HashGenerator.hpp"
#include "Logger.hpp"
#include "MetaDataReader.hpp"
#include "Models.hpp"
#include "Scanner.hpp"

using ProgressCallback = std::function<void(const ProgressInfo&)>;

using CancellationCallback = std::function<bool()>;

class SortEngine {
 public:
  explicit SortEngine(const Config& config, Logger& logger);

  bool run(const std::filesystem::path& sourceFolder,
           const std::filesystem::path& destinationFolder, ProgressCallback callback = nullptr,
           CancellationCallback cancellationCallback = nullptr);

 private:
  Logger& logger_;

  Scanner scanner_;

  MetaDataReader metadataReader_;

  DestinationPathGenerator destinationGenerator_;

  FileCopier fileCopier_;

  HashGenerator hashGenerator_;

  Database database_;
};
