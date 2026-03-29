#include "DatabaseManager.h"
#include "MigrationRunner.h"

#include <QSqlError>
#include <QSqlQuery>

namespace CF::Infra {

DatabaseManager::DatabaseManager(const std::string& dbPath,
                                 Core::Logger&      logger)
    : m_dbPath(dbPath)
    , m_logger(logger)
{}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
        m_logger.info("Database connection closed", "DatabaseManager");
    }
    QSqlDatabase::removeDatabase(CONNECTION_NAME);
}

Core::Result<void> DatabaseManager::initialize()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", CONNECTION_NAME);
    m_db.setDatabaseName(QString::fromStdString(m_dbPath));

    if (!m_db.open()) {
        const std::string msg = "Failed to open database: "
            + m_db.lastError().text().toStdString();
        m_logger.error(msg, "DatabaseManager");
        return Core::Result<void>::err(Core::AppError::dbError(msg));
    }

    m_logger.info("Database opened: " + m_dbPath, "DatabaseManager");

    // Enable WAL mode for better concurrent read performance
    execute("PRAGMA journal_mode=WAL");
    execute("PRAGMA foreign_keys=ON");
    execute("PRAGMA synchronous=NORMAL");

    return runMigrations();
}

QSqlQuery DatabaseManager::makeQuery() const
{
    return QSqlQuery(m_db);
}

Core::Result<void> DatabaseManager::execute(const QString& sql)
{
    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        const std::string msg = "SQL error: "
            + q.lastError().text().toStdString()
            + " | SQL: " + sql.toStdString();
        m_logger.error(msg, "DatabaseManager");
        return Core::Result<void>::err(Core::AppError::dbError(msg));
    }
    return Core::Result<void>::ok();
}

bool DatabaseManager::beginTransaction()
{
    return m_db.transaction();
}

bool DatabaseManager::commitTransaction()
{
    return m_db.commit();
}

void DatabaseManager::rollbackTransaction()
{
    m_db.rollback();
}

QString DatabaseManager::connectionName() const
{
    return CONNECTION_NAME;
}

Core::Result<void> DatabaseManager::runMigrations()
{
    MigrationRunner runner(*this, m_logger);
    return runner.run();
}

} // namespace CF::Infra