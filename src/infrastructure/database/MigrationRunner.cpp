#include "MigrationRunner.h"
#include "DatabaseManager.h"
#include "migrations/Migration_001_initial.h"
#include "migrations/Migration_002_fts.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace CF::Infra {

MigrationRunner::MigrationRunner(DatabaseManager& db, Core::Logger& logger)
    : m_db(db)
    , m_logger(logger)
{}

Core::Result<void> MigrationRunner::run()
{
    auto ensureResult = ensureMigrationsTable();
    if (ensureResult.isErr()) return ensureResult;

    const auto applied = appliedVersions();
    const auto all     = allMigrations();

    for (const auto& migration : all) {
        const bool alreadyApplied = std::find(
            applied.begin(), applied.end(), migration.version
        ) != applied.end();

        if (alreadyApplied) continue;

        m_logger.info("Applying migration " + std::to_string(migration.version)
                      + ": " + migration.description, "MigrationRunner");

        auto result = applyMigration(migration);
        if (result.isErr()) {
            m_logger.error("Migration " + std::to_string(migration.version)
                           + " failed: " + result.error().message,
                           "MigrationRunner");
            return result;
        }
    }

    return Core::Result<void>::ok();
}

Core::Result<void> MigrationRunner::ensureMigrationsTable()
{
    return m_db.execute(R"(
        CREATE TABLE IF NOT EXISTS schema_migrations (
            version     INTEGER PRIMARY KEY,
            description TEXT    NOT NULL,
            applied_at  TEXT    NOT NULL DEFAULT (datetime('now'))
        )
    )");
}

std::vector<int> MigrationRunner::appliedVersions()
{
    std::vector<int> versions;
    auto q = m_db.makeQuery();
    q.exec("SELECT version FROM schema_migrations ORDER BY version ASC");
    while (q.next()) {
        versions.push_back(q.value(0).toInt());
    }
    return versions;
}

Core::Result<void> MigrationRunner::applyMigration(const Migration& m)
{
    if (!m_db.beginTransaction()) {
        return Core::Result<void>::err(
            Core::AppError::dbError("Failed to begin transaction for migration "
                                    + std::to_string(m.version)));
    }

    for (const auto& sql : m.up()) {
        auto result = m_db.execute(sql);
        if (result.isErr()) {
            m_db.rollbackTransaction();
            return result;
        }
    }

    // Record that this migration has been applied
    auto q = m_db.makeQuery();
    q.prepare("INSERT INTO schema_migrations (version, description) VALUES (?, ?)");
    q.addBindValue(m.version);
    q.addBindValue(QString::fromStdString(m.description));
    if (!q.exec()) {
        m_db.rollbackTransaction();
        return Core::Result<void>::err(
            Core::AppError::dbError("Failed to record migration: "
                                    + q.lastError().text().toStdString()));
    }

    if (!m_db.commitTransaction()) {
        m_db.rollbackTransaction();
        return Core::Result<void>::err(
            Core::AppError::dbError("Failed to commit migration "
                                    + std::to_string(m.version)));
    }

    m_logger.info("Migration " + std::to_string(m.version) + " applied OK",
                  "MigrationRunner");
    return Core::Result<void>::ok();
}

std::vector<Migration> MigrationRunner::allMigrations()
{
    // Register every migration here in ascending version order
    return {
        Migrations::migration_001_initial(),
        Migrations::migration_002_fts()
    };
}

} // namespace CF::Infra