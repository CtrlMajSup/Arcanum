#include "SqliteSceneRepository.h"
#include "core/Assert.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace CF::Infra {

using namespace CF::Domain;
using namespace CF::Core;

SqliteSceneRepository::SqliteSceneRepository(DatabaseManager& db, Core::Logger& logger)
    : m_db(db), m_logger(logger)
{}

// ── findById ──────────────────────────────────────────────────────────────────

Result<Scene> SqliteSceneRepository::findById(SceneId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM scenes WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));

    if (!q.exec() || !q.next()) {
        return Result<Scene>::err(
            AppError::notFound("Scene not found: " + id.toString()));
    }
    return Result<Scene>::ok(hydrateScene(q));
}

// ── findAll ───────────────────────────────────────────────────────────────────

Result<std::vector<Scene>> SqliteSceneRepository::findAll() const
{
    auto q = m_db.makeQuery();
    q.exec("SELECT * FROM scenes ORDER BY era, year");

    std::vector<Scene> result;
    while (q.next()) result.push_back(hydrateScene(q));
    return Result<std::vector<Scene>>::ok(std::move(result));
}

// ── save ──────────────────────────────────────────────────────────────────────

Result<Scene> SqliteSceneRepository::save(const Scene& s)
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        INSERT INTO scenes (title, summary, era, year, chapter_id)
        VALUES (?, ?, ?, ?, ?)
    )");
    q.addBindValue(QString::fromStdString(s.title));
    q.addBindValue(QString::fromStdString(s.summary));
    q.addBindValue(s.when.era);
    q.addBindValue(s.when.year);
    q.addBindValue(s.chapterId
                   ? QVariant(static_cast<qlonglong>(s.chapterId->value()))
                   : QVariant());

    if (!q.exec()) {
        return Result<Scene>::err(AppError::dbError(
            "Insert scene failed: " + q.lastError().text().toStdString()));
    }

    Scene saved = s;
    saved.id = SceneId(q.lastInsertId().toLongLong());

    // Persist the many-to-many links that were already on the entity
    for (const auto& pid : saved.placeIds)
        linkPlace(saved.id, pid);
    for (const auto& fid : saved.factionIds)
        linkFaction(saved.id, fid);
    for (const auto& cid : saved.characterIds)
        linkCharacter(saved.id, cid);

    m_logger.debug("Scene saved: " + saved.title, "SceneRepo");
    return Result<Scene>::ok(std::move(saved));
}

// ── update ────────────────────────────────────────────────────────────────────

Result<Scene> SqliteSceneRepository::update(const Scene& s)
{
    CF_REQUIRE(s.id.isValid(), "Cannot update scene with invalid ID");

    auto q = m_db.makeQuery();
    q.prepare(R"(
        UPDATE scenes SET
            title=?, summary=?, era=?, year=?, chapter_id=?
        WHERE id=?
    )");
    q.addBindValue(QString::fromStdString(s.title));
    q.addBindValue(QString::fromStdString(s.summary));
    q.addBindValue(s.when.era);
    q.addBindValue(s.when.year);
    q.addBindValue(s.chapterId
                   ? QVariant(static_cast<qlonglong>(s.chapterId->value()))
                   : QVariant());
    q.addBindValue(static_cast<qlonglong>(s.id.value()));

    if (!q.exec()) {
        return Result<Scene>::err(AppError::dbError(
            "Update scene failed: " + q.lastError().text().toStdString()));
    }
    return Result<Scene>::ok(s);
}

// ── remove ────────────────────────────────────────────────────────────────────

Result<void> SqliteSceneRepository::remove(SceneId id)
{
    auto q = m_db.makeQuery();
    q.prepare("DELETE FROM scenes WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Delete scene failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── exists ────────────────────────────────────────────────────────────────────

bool SqliteSceneRepository::exists(SceneId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT 1 FROM scenes WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    return q.exec() && q.next();
}

// ── findByNameContaining ──────────────────────────────────────────────────────

Result<std::vector<Scene>>
SqliteSceneRepository::findByNameContaining(const std::string& fragment) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM scenes WHERE title LIKE ? ORDER BY era, year");
    q.addBindValue(QString::fromStdString("%" + fragment + "%"));

    std::vector<Scene> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateScene(q));
    return Result<std::vector<Scene>>::ok(std::move(result));
}

// ── findInTimeRange ───────────────────────────────────────────────────────────

Result<std::vector<Scene>>
SqliteSceneRepository::findInTimeRange(
    const TimePoint& from, const TimePoint& to) const
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        SELECT * FROM scenes
        WHERE (era * 100000 + year) >= ?
          AND (era * 100000 + year) <= ?
        ORDER BY era, year
    )");
    q.addBindValue(static_cast<qlonglong>(from.toSortKey()));
    q.addBindValue(static_cast<qlonglong>(to.toSortKey()));

    std::vector<Scene> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateScene(q));
    return Result<std::vector<Scene>>::ok(std::move(result));
}

// ── findByPlace ───────────────────────────────────────────────────────────────

Result<std::vector<Scene>>
SqliteSceneRepository::findByPlace(PlaceId placeId) const
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        SELECT s.* FROM scenes s
        JOIN scene_places sp ON sp.scene_id = s.id
        WHERE sp.place_id = ?
        ORDER BY s.era, s.year
    )");
    q.addBindValue(static_cast<qlonglong>(placeId.value()));

    std::vector<Scene> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateScene(q));
    return Result<std::vector<Scene>>::ok(std::move(result));
}

// ── findByFaction ─────────────────────────────────────────────────────────────

Result<std::vector<Scene>>
SqliteSceneRepository::findByFaction(FactionId factionId) const
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        SELECT s.* FROM scenes s
        JOIN scene_factions sf ON sf.scene_id = s.id
        WHERE sf.faction_id = ?
        ORDER BY s.era, s.year
    )");
    q.addBindValue(static_cast<qlonglong>(factionId.value()));

    std::vector<Scene> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateScene(q));
    return Result<std::vector<Scene>>::ok(std::move(result));
}

// ── findByCharacter ───────────────────────────────────────────────────────────

Result<std::vector<Scene>>
SqliteSceneRepository::findByCharacter(CharacterId characterId) const
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        SELECT s.* FROM scenes s
        JOIN scene_characters sc ON sc.scene_id = s.id
        WHERE sc.character_id = ?
        ORDER BY s.era, s.year
    )");
    q.addBindValue(static_cast<qlonglong>(characterId.value()));

    std::vector<Scene> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateScene(q));
    return Result<std::vector<Scene>>::ok(std::move(result));
}

// ── findByChapter ─────────────────────────────────────────────────────────────

Result<std::vector<Scene>>
SqliteSceneRepository::findByChapter(ChapterId chapterId) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM scenes WHERE chapter_id = ? ORDER BY era, year");
    q.addBindValue(static_cast<qlonglong>(chapterId.value()));

    std::vector<Scene> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateScene(q));
    return Result<std::vector<Scene>>::ok(std::move(result));
}

// ── Many-to-many link/unlink ──────────────────────────────────────────────────

Result<void> SqliteSceneRepository::linkPlace(SceneId sid, PlaceId pid)
{
    return insertJoin("scene_places", "scene_id", sid.value(), "place_id", pid.value());
}
Result<void> SqliteSceneRepository::unlinkPlace(SceneId sid, PlaceId pid)
{
    return deleteJoin("scene_places", "scene_id", sid.value(), "place_id", pid.value());
}
Result<void> SqliteSceneRepository::linkFaction(SceneId sid, FactionId fid)
{
    return insertJoin("scene_factions", "scene_id", sid.value(), "faction_id", fid.value());
}
Result<void> SqliteSceneRepository::unlinkFaction(SceneId sid, FactionId fid)
{
    return deleteJoin("scene_factions", "scene_id", sid.value(), "faction_id", fid.value());
}
Result<void> SqliteSceneRepository::linkCharacter(SceneId sid, CharacterId cid)
{
    return insertJoin("scene_characters", "scene_id", sid.value(), "character_id", cid.value());
}
Result<void> SqliteSceneRepository::unlinkCharacter(SceneId sid, CharacterId cid)
{
    return deleteJoin("scene_characters", "scene_id", sid.value(), "character_id", cid.value());
}

// ── Generic join helpers ──────────────────────────────────────────────────────

Result<void> SqliteSceneRepository::insertJoin(
    const QString& table,
    const QString& col1, qlonglong id1,
    const QString& col2, qlonglong id2)
{
    auto q = m_db.makeQuery();
    q.prepare(QString("INSERT OR IGNORE INTO %1 (%2, %3) VALUES (?, ?)")
              .arg(table, col1, col2));
    q.addBindValue(id1);
    q.addBindValue(id2);
    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Join insert failed on " + table.toStdString()
            + ": " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

Result<void> SqliteSceneRepository::deleteJoin(
    const QString& table,
    const QString& col1, qlonglong id1,
    const QString& col2, qlonglong id2)
{
    auto q = m_db.makeQuery();
    q.prepare(QString("DELETE FROM %1 WHERE %2=? AND %3=?")
              .arg(table, col1, col2));
    q.addBindValue(id1);
    q.addBindValue(id2);
    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Join delete failed on " + table.toStdString()
            + ": " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── Private helpers ───────────────────────────────────────────────────────────

Scene SqliteSceneRepository::hydrateScene(QSqlQuery& q) const
{
    Scene s;
    s.id      = SceneId(q.value("id").toLongLong());
    s.title   = q.value("title").toString().toStdString();
    s.summary = q.value("summary").toString().toStdString();
    s.when    = { q.value("era").toInt(), q.value("year").toInt() };

    const auto cid = q.value("chapter_id");
    if (!cid.isNull()) s.chapterId = ChapterId(cid.toLongLong());

    const int64_t id = s.id.value();
    s.placeIds     = loadPlaceIds(id);
    s.factionIds   = loadFactionIds(id);
    s.characterIds = loadCharacterIds(id);

    return s;
}

std::vector<PlaceId>
SqliteSceneRepository::loadPlaceIds(int64_t sceneId) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT place_id FROM scene_places WHERE scene_id=?");
    q.addBindValue(static_cast<qlonglong>(sceneId));
    q.exec();

    std::vector<PlaceId> result;
    while (q.next()) result.emplace_back(q.value(0).toLongLong());
    return result;
}

std::vector<FactionId>
SqliteSceneRepository::loadFactionIds(int64_t sceneId) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT faction_id FROM scene_factions WHERE scene_id=?");
    q.addBindValue(static_cast<qlonglong>(sceneId));
    q.exec();

    std::vector<FactionId> result;
    while (q.next()) result.emplace_back(q.value(0).toLongLong());
    return result;
}

std::vector<CharacterId>
SqliteSceneRepository::loadCharacterIds(int64_t sceneId) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT character_id FROM scene_characters WHERE scene_id=?");
    q.addBindValue(static_cast<qlonglong>(sceneId));
    q.exec();

    std::vector<CharacterId> result;
    while (q.next()) result.emplace_back(q.value(0).toLongLong());
    return result;
}

} // namespace CF::Infra