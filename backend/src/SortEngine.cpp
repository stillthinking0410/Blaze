#include "SortEngine.hpp"

#include <iostream>
#include <stdexcept>

SortEngine::SortEngine(
    const Config& config,
    Logger& logger
)
    : logger_(logger),
      scanner_(logger),
      metadataReader_(config, logger),
      fileCopier_(logger),
      database_(config.databasePath, logger)
{
}

bool SortEngine::run(
    const std::filesystem::path& sourceFolder,
    const std::filesystem::path& destinationFolder,
    ProgressCallback callback,
    CancellationCallback cancellationCallback
)
{
    if (!database_.initialize())
    {
        logger_.error(
            "Failed to initialize database."
        );

        return false;
    }

    logger_.info(
        "Database initialized successfully."
    );

    std::vector<std::filesystem::path> files;

    try
    {
        files = scanner_.scan(sourceFolder);

        logger_.info(
            "Found " +
            std::to_string(files.size()) +
            " files."
        );
    }
    catch (const std::exception& e)
    {
        logger_.error(e.what());
        return false;
    }

    ProgressInfo progress;
    progress.totalFiles = files.size();

    if (callback)
    {
        callback(progress);
    }

    for (const auto& file : files)
    {
        // Check for cancellation before starting
        // the next file.
        if (cancellationCallback &&
            cancellationCallback())
        {
            logger_.info(
                "Sorting cancellation requested."
            );

            return true;
        }

        progress.currentFile =
            file.filename().string();

        try
        {
            // Generate SHA-256 hash.
            std::string hash =
                hashGenerator_.generateHash(file);

            // Skip if already processed.
            if (database_.isProcessed(hash))
            {
                progress.skippedFiles++;

                logger_.info(
                    "Skipped duplicate: " +
                    file.filename().string()
                );

                if (callback)
                {
                    callback(progress);
                }

                continue;
            }

            // Read the original capture date from metadata.
            auto captureDate =
                metadataReader_.getCaptureDate(file);

            // Fall back to the file modification time when metadata is absent.
            if (!captureDate)
            {
                std::error_code error;
                const auto modifiedTime =
                    std::filesystem::last_write_time(file, error);

                if (!error)
                {
                    const auto systemTime =
                        std::chrono::time_point_cast<
                            std::chrono::system_clock::duration
                        >(
                            modifiedTime -
                            std::filesystem::file_time_type::clock::now() +
                            std::chrono::system_clock::now()
                        );

                    captureDate = systemTime;

                    logger_.info(
                        "Using file modification time: " +
                        file.filename().string()
                    );
                }
            }

            // Fail only if neither metadata nor modification time is available.
            if (!captureDate)
            {
                progress.failedFiles++;

                logger_.warning(
                    "Capture date and modification time unavailable: " +
                    file.filename().string()
                );

                if (callback)
                {
                    callback(progress);
                }

                continue;
            }

            // Generate destination folder.
            auto destinationFolderPath =
                destinationGenerator_.generateFolderPath(
                    destinationFolder,
                    *captureDate
                );

            // Generate destination filename.
            auto targetFileName =
                destinationGenerator_.generateFileName(
                    *captureDate,
                    file.extension().string()
                );

            // Copy file.
            auto copiedFile =
                fileCopier_.copyFile(
                    file,
                    destinationFolderPath,
                    targetFileName
                );

            // Mark as processed.
            if (!database_.addProcessed(
                    hash,
                    file.string(),
                    copiedFile.string()))
            {
                // Remove the copied file because
                // database tracking failed.
                std::filesystem::remove(copiedFile);

                logger_.error(
                    "Database update failed."
                );

                throw std::runtime_error(
                    "Database update failed."
                );
            }

            progress.processedFiles++;

            logger_.info(
                "Copied: " +
                copiedFile.string()
            );

            if (callback)
            {
                callback(progress);
            }
        }
        catch (const std::exception& e)
        {
            progress.failedFiles++;

            if (callback)
            {
                callback(progress);
            }

            logger_.error(
                "Failed to process " +
                file.filename().string() +
                ". Reason: " +
                e.what()
            );
        }
    }

    // Sorting completed normally.
    progress.currentFile.clear();

    if (callback)
    {
        callback(progress);
    }

    std::cout
        << "\n========== Blaze Summary ==========\n";

    std::cout
        << "Total Files    : "
        << progress.totalFiles
        << '\n';

    std::cout
        << "Copied         : "
        << progress.processedFiles
        << '\n';

    std::cout
        << "Skipped        : "
        << progress.skippedFiles
        << '\n';

    std::cout
        << "Failed         : "
        << progress.failedFiles
        << '\n';

    std::cout
        << "Percentage     : "
        << progress.percentage()
        << "%\n";

    std::cout
        << "===================================\n";

    return true;
}