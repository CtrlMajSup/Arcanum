#pragma once

#include "core/Result.h"
#include "core/Logger.h"

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <memory>
#include <string>

namespace CF::Infra {

/**
 * DatabaseManager owns the single SQLite connection for the application.
 *
 * Responsibilities:
 *  - Open / close the database file
 *  - Expose a configured QSqlQuery factory
 *  - Run the migration chain at startup
 *  - Provide transaction helpers
 *
 * All repositories receive a reference to this manager via constructor
 * injection — they never open their own connections.
 */
class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& dbPath,
                             Core::Logger&      logger);
    ~DatabaseManager();

    // Opens the database and runs pending migrations.
    // Returns false if the DB cannot be opened or migrations fail.
    [[nodiscard]] Core::Result<void> initialize();

    // Creates a query bound to this connection
    [[nodiscard]] QSqlQuery makeQuery() const;

    // Execute a raw SQL string — use only for DDL/migrations
    [[nodiscard]] Core::Result<void> execute(const QString& sql);

    // Transaction helpers — repos use these for multi-statement writes
    [[nodiscard]] bool beginTransaction();
    [[nodiscard]] bool commitTransaction();
    void               rollbackTransaction();

    // Returns the underlying Qt connection name (for diagnostics)
    [[nodiscard]] QString connectionName() const;

private:
    [[nodiscard]] Core::Result<void> runMigrations();

    std::string    m_dbPath;
    Core::Logger&  m_logger;
    QSqlDatabase   m_db;

    static constexpr const char* CONNECTION_NAME = "cf_main";
};

} // namespace CF::Infra