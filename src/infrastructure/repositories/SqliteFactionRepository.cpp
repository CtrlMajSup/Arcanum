#include "SqliteFactionRepository.h"
#include "core/Assert.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace CF::Infra {

using namespace CF::Domain;
using namespace CF::Core;

SqliteFactionRepository::SqliteFactionRepository(DatabaseManager& db, Core::Logger& logger)
    : m_db(db), m_logger(logger)
{}

// ── findById ──────────────────────────────────────────────────────────────────

Result<Faction> SqliteFactionRepository::findById(FactionId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM factions WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));

    if (!q.exec() || !q.next()) {
        return Result<Faction>::err(
            AppError::notFound("Faction not found: " + id.toString()));
    }
    return Result<Faction>::ok(hydrateFaction(q));
}

// ── findAll ───────────────────────────────────────────────────────────────────

Result<std::vector<Faction>> SqliteFactionRepository::findAll() const
{
    auto q = m_db.makeQuery();
    q.exec("SELECT * FROM factions ORDER BY name");

    std::vector<Faction> result;
    while (q.next()) result.push_back(hydrateFaction(q));
    return Result<std::vector<Faction>>::ok(std::move(result));
}

// ── save ──────────────────────────────────────────────────────────────────────

Result<Faction> SqliteFactionRepository::save(const Faction& f)
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        INSERT INTO factions
            (name, type, description, icon_ref,
             founded_era, founded_year, dissolved_era, dissolved_year)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    q.addBindValue(QString::fromStdString(f.name));
    q.addBindValue(QString::fromStdString(f.type));
    q.addBindValue(QString::fromStdString(f.description));
    q.addBindValue(QString::fromStdString(f.iconRef));
    q.addBindValue(f.founded.era);
    q.addBindValue(f.founded.year);
    q.addBindValue(f.dissolved ? QVariant(f.dissolved->era)  : QVariant());
    q.addBindValue(f.dissolved ? QVariant(f.dissolved->year) : QVariant());

    if (!q.exec()) {
        return Result<Faction>::err(AppError::dbError(
            "Insert faction failed: " + q.lastError().text().toStdString()));
    }

    Faction saved = f;
    saved.id = FactionId(q.lastInsertId().toLongLong());
    m_logger.debug("Faction saved: " + saved.name, "FactionRepo");
    return Result<Faction>::ok(std::move(saved));
}

// ── update ────────────────────────────────────────────────────────────────────

Result<Faction> SqliteFactionRepository::update(const Faction& f)
{
    CF_REQUIRE(f.id.isValid(), "Cannot update faction with invalid ID");

    auto q = m_db.makeQuery();
    q.prepare(R"(
        UPDATE factions SET
            name=?, type=?, description=?, icon_ref=?,
            founded_era=?, founded_year=?,
            dissolved_era=?, dissolved_year=?
        WHERE id=?
    )");
    q.addBindValue(QString::fromStdString(f.name));
    q.addBindValue(QString::fromStdString(f.type));
    q.addBindValue(QString::fromStdString(f.description));
    q.addBindValue(QString::fromStdString(f.iconRef));
    q.addBindValue(f.founded.era);
    q.addBindValue(f.founded.year);
    q.addBindValue(f.dissolved ? QVariant(f.dissolved->era)  : QVariant());
    q.addBindValue(f.dissolved ? QVariant(f.dissolved->year) : QVariant());
    q.addBindValue(static_cast<qlonglong>(f.id.value()));

    if (!q.exec()) {
        return Result<Faction>::err(AppError::dbError(
            "Update faction failed: " + q.lastError().text().toStdString()));
    }
    return Result<Faction>::ok(f);
}

// ── remove ────────────────────────────────────────────────────────────────────

Result<void> SqliteFactionRepository::remove(FactionId id)
{
    auto q = m_db.makeQuery();
    q.prepare("DELETE FROM factions WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Delete faction failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── exists ────────────────────────────────────────────────────────────────────

bool SqliteFactionRepository::exists(FactionId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT 1 FROM factions WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    return q.exec() && q.next();
}

// ── findByNameContaining ──────────────────────────────────────────────────────

Result<std::vector<Faction>>
SqliteFactionRepository::findByNameContaining(const std::string& fragment) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM factions WHERE name LIKE ? ORDER BY name");
    q.addBindValue(QString::fromStdString("%" + fragment + "%"));

    std::vector<Faction> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateFaction(q));
    return Result<std::vector<Faction>>::ok(std::move(result));
}

// ── findActiveAt ──────────────────────────────────────────────────────────────

Result<std::vector<Faction>>
SqliteFactionRepository::findActiveAt(const TimePoint& when) const
{
    auto q = m_db.makeQuery();
    // A faction is active if it was founded at or before 'when'
    // AND either not dissolved, or dissolved after 'when'
    q.prepare(R"(
        SELECT * FROM factions
        WHERE (founded_era * 100000 + founded_year) <= ?
          AND (
              dissolved_era IS NULL
              OR (dissolved_era * 100000 + dissolved_year) >= ?
          )
        ORDER BY name
    )");
    const qlonglong sortKey = static_cast<qlonglong>(when.toSortKey());
    q.addBindValue(sortKey);
    q.addBindValue(sortKey);

    std::vector<Faction> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateFaction(q));
    return Result<std::vector<Faction>>::ok(std::move(result));
}

// ── findByType ────────────────────────────────────────────────────────────────

Result<std::vector<Faction>>
SqliteFactionRepository::findByType(const std::string& type) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM factions WHERE type = ? ORDER BY name");
    q.addBindValue(QString::fromStdString(type));

    std::vector<Faction> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateFaction(q));
    return Result<std::vector<Faction>>::ok(std::move(result));
}

// ── saveEvolution ─────────────────────────────────────────────────────────────

Result<void> SqliteFactionRepository::saveEvolution(
    FactionId factionId, const FactionEvolution& evolution)
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        INSERT OR REPLACE INTO faction_evolutions
            (faction_id, era, year, description)
        VALUES (?, ?, ?, ?)
    )");
    q.addBindValue(static_cast<qlonglong>(factionId.value()));
    q.addBindValue(evolution.at.era);
    q.addBindValue(evolution.at.year);
    q.addBindValue(QString::fromStdString(evolution.description));

    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Save faction evolution failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── removeEvolution ───────────────────────────────────────────────────────────

Result<void> SqliteFactionRepository::removeEvolution(
    FactionId factionId, const TimePoint& at)
{
    auto q = m_db.makeQuery();
    q.prepare("DELETE FROM faction_evolutions "
              "WHERE faction_id=? AND era=? AND year=?");
    q.addBindValue(static_cast<qlonglong>(factionId.value()));
    q.addBindValue(at.era);
    q.addBindValue(at.year);

    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Remove faction evolution failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── Private helpers ───────────────────────────────────────────────────────────

Faction SqliteFactionRepository::hydrateFaction(QSqlQuery& q) const
{
    Faction f;
    f.id          = FactionId(q.value("id").toLongLong());
    f.name        = q.value("name").toString().toStdString();
    f.type        = q.value("type").toString().toStdString();
    f.description = q.value("description").toString().toStdString();
    f.iconRef     = q.value("icon_ref").toString().toStdString();
    f.founded     = { q.value("founded_era").toInt(),
                      q.value("founded_year").toInt() };

    const auto dEra  = q.value("dissolved_era");
    const auto dYear = q.value("dissolved_year");
    if (!dEra.isNull())
        f.dissolved = TimePoint{ dEra.toInt(), dYear.toInt() };

    f.evolutions = loadEvolutions(f.id.value());
    return f;
}

std::vector<FactionEvolution>
SqliteFactionRepository::loadEvolutions(int64_t factionId) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT era, year, description FROM faction_evolutions "
              "WHERE faction_id=? ORDER BY era, year");
    q.addBindValue(static_cast<qlonglong>(factionId));
    q.exec();

    std::vector<FactionEvolution> result;
    while (q.next()) {
        result.push_back({
            { q.value("era").toInt(), q.value("year").toInt() },
            q.value("description").toString().toStdString()
        });
    }
    return result;
}

} // namespace CF::Infra