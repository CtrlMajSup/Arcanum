#include "SqlitePlaceRepository.h"
#include "core/Assert.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace CF::Infra {

using namespace CF::Domain;
using namespace CF::Core;

SqlitePlaceRepository::SqlitePlaceRepository(DatabaseManager& db, Core::Logger& logger)
    : m_db(db), m_logger(logger)
{}

// ── findById ──────────────────────────────────────────────────────────────────

Result<Place> SqlitePlaceRepository::findById(PlaceId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM places WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));

    if (!q.exec() || !q.next()) {
        return Result<Place>::err(
            AppError::notFound("Place not found: " + id.toString()));
    }
    return Result<Place>::ok(hydratePlace(q));
}

// ── findAll ───────────────────────────────────────────────────────────────────

Result<std::vector<Place>> SqlitePlaceRepository::findAll() const
{
    auto q = m_db.makeQuery();
    q.exec("SELECT * FROM places ORDER BY region, name");

    std::vector<Place> result;
    while (q.next()) result.push_back(hydratePlace(q));
    return Result<std::vector<Place>>::ok(std::move(result));
}

// ── save ──────────────────────────────────────────────────────────────────────

Result<Place> SqlitePlaceRepository::save(const Place& p)
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        INSERT INTO places
            (name, type, region, description, is_mobile, map_x, map_y, parent_id)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    q.addBindValue(QString::fromStdString(p.name));
    q.addBindValue(QString::fromStdString(p.type));
    q.addBindValue(QString::fromStdString(p.region));
    q.addBindValue(QString::fromStdString(p.description));
    q.addBindValue(p.isMobile ? 1 : 0);
    q.addBindValue(p.mapX);
    q.addBindValue(p.mapY);
    q.addBindValue(p.parentPlaceId
                   ? QVariant(static_cast<qlonglong>(p.parentPlaceId->value()))
                   : QVariant());

    if (!q.exec()) {
        return Result<Place>::err(AppError::dbError(
            "Insert place failed: " + q.lastError().text().toStdString()));
    }

    Place saved  = p;
    saved.id     = PlaceId(q.lastInsertId().toLongLong());
    m_logger.debug("Place saved: " + saved.name, "PlaceRepo");
    return Result<Place>::ok(std::move(saved));
}

// ── update ────────────────────────────────────────────────────────────────────

Result<Place> SqlitePlaceRepository::update(const Place& p)
{
    CF_REQUIRE(p.id.isValid(), "Cannot update place with invalid ID");

    auto q = m_db.makeQuery();
    q.prepare(R"(
        UPDATE places SET
            name=?, type=?, region=?, description=?,
            is_mobile=?, map_x=?, map_y=?, parent_id=?
        WHERE id=?
    )");
    q.addBindValue(QString::fromStdString(p.name));
    q.addBindValue(QString::fromStdString(p.type));
    q.addBindValue(QString::fromStdString(p.region));
    q.addBindValue(QString::fromStdString(p.description));
    q.addBindValue(p.isMobile ? 1 : 0);
    q.addBindValue(p.mapX);
    q.addBindValue(p.mapY);
    q.addBindValue(p.parentPlaceId
                   ? QVariant(static_cast<qlonglong>(p.parentPlaceId->value()))
                   : QVariant());
    q.addBindValue(static_cast<qlonglong>(p.id.value()));

    if (!q.exec()) {
        return Result<Place>::err(AppError::dbError(
            "Update place failed: " + q.lastError().text().toStdString()));
    }
    return Result<Place>::ok(p);
}

// ── remove ────────────────────────────────────────────────────────────────────

Result<void> SqlitePlaceRepository::remove(PlaceId id)
{
    auto q = m_db.makeQuery();
    q.prepare("DELETE FROM places WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Delete place failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── exists ────────────────────────────────────────────────────────────────────

bool SqlitePlaceRepository::exists(PlaceId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT 1 FROM places WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    return q.exec() && q.next();
}

// ── findByNameContaining ──────────────────────────────────────────────────────

Result<std::vector<Place>>
SqlitePlaceRepository::findByNameContaining(const std::string& fragment) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM places WHERE name LIKE ? ORDER BY name");
    q.addBindValue(QString::fromStdString("%" + fragment + "%"));

    std::vector<Place> result;
    if (q.exec()) while (q.next()) result.push_back(hydratePlace(q));
    return Result<std::vector<Place>>::ok(std::move(result));
}

// ── findByRegion ──────────────────────────────────────────────────────────────

Result<std::vector<Place>>
SqlitePlaceRepository::findByRegion(const std::string& region) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM places WHERE region = ? ORDER BY name");
    q.addBindValue(QString::fromStdString(region));

    std::vector<Place> result;
    if (q.exec()) while (q.next()) result.push_back(hydratePlace(q));
    return Result<std::vector<Place>>::ok(std::move(result));
}

// ── allRegions ────────────────────────────────────────────────────────────────

Result<std::vector<std::string>> SqlitePlaceRepository::allRegions() const
{
    auto q = m_db.makeQuery();
    q.exec("SELECT DISTINCT region FROM places WHERE region != '' ORDER BY region");

    std::vector<std::string> result;
    while (q.next()) result.push_back(q.value(0).toString().toStdString());
    return Result<std::vector<std::string>>::ok(std::move(result));
}

// ── findMobilePlaces ──────────────────────────────────────────────────────────

Result<std::vector<Place>> SqlitePlaceRepository::findMobilePlaces() const
{
    auto q = m_db.makeQuery();
    q.exec("SELECT * FROM places WHERE is_mobile = 1 ORDER BY name");

    std::vector<Place> result;
    while (q.next()) result.push_back(hydratePlace(q));
    return Result<std::vector<Place>>::ok(std::move(result));
}

// ── findChildren ──────────────────────────────────────────────────────────────

Result<std::vector<Place>>
SqlitePlaceRepository::findChildren(PlaceId parentId) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM places WHERE parent_id = ? ORDER BY name");
    q.addBindValue(static_cast<qlonglong>(parentId.value()));

    std::vector<Place> result;
    if (q.exec()) while (q.next()) result.push_back(hydratePlace(q));
    return Result<std::vector<Place>>::ok(std::move(result));
}

// ── saveEvolution ─────────────────────────────────────────────────────────────

Result<void> SqlitePlaceRepository::saveEvolution(
    PlaceId placeId, const PlaceEvolution& evolution)
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        INSERT OR REPLACE INTO place_evolutions
            (place_id, era, year, description)
        VALUES (?, ?, ?, ?)
    )");
    q.addBindValue(static_cast<qlonglong>(placeId.value()));
    q.addBindValue(evolution.at.era);
    q.addBindValue(evolution.at.year);
    q.addBindValue(QString::fromStdString(evolution.description));

    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Save place evolution failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── removeEvolution ───────────────────────────────────────────────────────────

Result<void> SqlitePlaceRepository::removeEvolution(
    PlaceId placeId, const TimePoint& at)
{
    auto q = m_db.makeQuery();
    q.prepare("DELETE FROM place_evolutions "
              "WHERE place_id=? AND era=? AND year=?");
    q.addBindValue(static_cast<qlonglong>(placeId.value()));
    q.addBindValue(at.era);
    q.addBindValue(at.year);

    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Remove place evolution failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── Private helpers ───────────────────────────────────────────────────────────

Place SqlitePlaceRepository::hydratePlace(QSqlQuery& q) const
{
    Place p;
    p.id          = PlaceId(q.value("id").toLongLong());
    p.name        = q.value("name").toString().toStdString();
    p.type        = q.value("type").toString().toStdString();
    p.region      = q.value("region").toString().toStdString();
    p.description = q.value("description").toString().toStdString();
    p.isMobile    = q.value("is_mobile").toInt() == 1;
    p.mapX        = q.value("map_x").toDouble();
    p.mapY        = q.value("map_y").toDouble();

    const auto parentVar = q.value("parent_id");
    if (!parentVar.isNull())
        p.parentPlaceId = PlaceId(parentVar.toLongLong());

    p.evolutions = loadEvolutions(p.id.value());
    return p;
}

std::vector<PlaceEvolution>
SqlitePlaceRepository::loadEvolutions(int64_t placeId) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT era, year, description FROM place_evolutions "
              "WHERE place_id=? ORDER BY era, year");
    q.addBindValue(static_cast<qlonglong>(placeId));
    q.exec();

    std::vector<PlaceEvolution> result;
    while (q.next()) {
        result.push_back({
            { q.value("era").toInt(), q.value("year").toInt() },
            q.value("description").toString().toStdString()
        });
    }
    return result;
}

} // namespace CF::Infra