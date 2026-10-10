#include "Database.hpp"

#include <sqlite3.h>

#include <string>

Database::Database(
    const std::filesystem::path& databasePath,
    Logger& logger
)
    : databasePath_(databasePath),
      logger_(logger)
{    
}

Database::~Database()
{
    if (database_ != nullptr)
    {
        sqlite3_close(database_);

        logger_.info(
            "Database connection closed."
        );

        database_ = nullptr;
    }
}

bool Database::initialize()
{
    std::filesystem::create_directories(databasePath_.parent_path());

    logger_.info(
        "Opening database: " +
        databasePath_.string()
    );
    
    int result = sqlite3_open(
        databasePath_.string().c_str(),
        &database_
    );

    if (result != SQLITE_OK)
    {
        logger_.error(
            "Failed to open database: " +
            std::string(sqlite3_errmsg(database_))
        );

        return false;
    }

    const char* createTableSql = 
        R"(
        CREATE TABLE IF NOT EXISTS processed_files
        (
            hash TEXT PRIMARY KEY,
            source_path TEXT NOT NULL,
            destination_path TEXT NOT NULL,
            processed_at TEXT NOT NULL
        );
        )";
    
    char* errorMessage = nullptr;

    result = sqlite3_exec(
        database_,
        createTableSql,
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK)
    {
        logger_.error(
            "Failed to create processed_files table: " +
            std::string(errorMessage)
        );
        
        sqlite3_free(errorMessage);

        return false;
    }

    logger_.info(
        "Database initialized successfully"
    );

    return true;
}

bool Database::isProcessed(const std::string& hash) const
{
    const char* sql = 
        "SELECT 1 FROM processed_files "
        "WHERE hash = ? LIMIT 1;";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        database_,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK)
    {
        logger_.error(
            "Failed to prepare duplicate check statement: " +
            std::string(sqlite3_errmsg(database_))
        );
        
        return false;
    }

    sqlite3_bind_text(
        statement,
        1,
        hash.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    result = sqlite3_step(statement);

    bool exists = (result == SQLITE_ROW);

    sqlite3_finalize(statement);

    return exists;
}

bool Database::addProcessed(
    const std::string& hash,
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& destinationPath
)
{
    const char* sql =
        "INSERT INTO processed_files "
        "(hash, source_path, destination_path, processed_at) "
        "VALUES (?, ?, ?, datetime('now'));";

        sqlite3_stmt* statement = nullptr;

        int result = sqlite3_prepare_v2(
            database_,
            sql,
            -1,
            &statement,
            nullptr
        );

        if (result != SQLITE_OK)
        {
            logger_.error(
                "Failed to prepare insert statement: " +
                std::string(sqlite3_errmsg(database_))
            );

            return false;
        }

        sqlite3_bind_text(
            statement,
            1,
            hash.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        sqlite3_bind_text(
            statement,
            2,
            sourcePath.string().c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        sqlite3_bind_text(
            statement,
            3,
            destinationPath.string().c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        result = sqlite3_step(statement);

        sqlite3_finalize(statement);

        if (result != SQLITE_DONE)
        {
            logger_.error(
                "Failed to insert processed file: " +
                std::string(sqlite3_errmsg(database_))
            );

            return false;
        }

        return true;
}
