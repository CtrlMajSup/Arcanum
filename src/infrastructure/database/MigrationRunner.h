#pragma once

#include "core/Result.h"
#include "core/Logger.h"

#include <QString>
#include <vector>
#include <string>
#include <functional>

namespace CF::Infra {

class DatabaseManager;

/**
 * A Migration is a versioned DDL change.
 * Each migration has a unique integer version and an up() function.
 * Migrations are applied exactly once and never re-run.
 *
 * The schema_migrations table tracks which versions have been applied.
 */
struct Migration {
    int         version;
    std::string description;
    // Returns the SQL statements to execute for this migration
    std::function<std::vector<QString>()> up;
};

/**
 * MigrationRunner checks which migrations have been applied,
 * then runs pending ones in ascending version order.
 */
class MigrationRunner {
public:
    explicit MigrationRunner(DatabaseManager& db, Core::Logger& logger);

    [[nodiscard]] Core::Result<void> run();

private:
    [[nodiscard]] Core::Result<void>      ensureMigrationsTable();
    [[nodiscard]] std::vector<int>        appliedVersions();
    [[nodiscard]] Core::Result<void>      applyMigration(const Migration& m);
    [[nodiscard]] std::vector<Migration>  allMigrations();

    DatabaseManager& m_db;
    Core::Logger&    m_logger;
};

} // namespace CF::Infra