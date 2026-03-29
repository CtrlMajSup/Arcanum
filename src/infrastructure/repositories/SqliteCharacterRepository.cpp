#include "SqliteCharacterRepository.h"
#include "core/Assert.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace CF::Infra {

using namespace CF::Domain;
using namespace CF::Core;

// ── Helpers ───────────────────────────────────────────────────────────────────

static TimePoint timePointFromCols(const QVariant& era, const QVariant& year)
{
    return { era.toInt(), year.toInt() };
}

static std::optional<TimePoint> optTimePoint(const QVariant& era, const QVariant& year)
{
    if (era.isNull()) return std::nullopt;
    return TimePoint{ era.toInt(), year.toInt() };
}

// ── Constructor ───────────────────────────────────────────────────────────────

SqliteCharacterRepository::SqliteCharacterRepository(DatabaseManager& db,
                                                     Core::Logger&    logger)
    : m_db(db)
    , m_logger(logger)
{}

// ── findById ──────────────────────────────────────────────────────────────────

Result<Character> SqliteCharacterRepository::findById(CharacterId id) const
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        SELECT id, name, species, biography, image_ref,
               born_era, born_year, died_era, died_year
        FROM characters WHERE id = ?
    )");
    q.addBindValue(static_cast<qlonglong>(id.value()));

    if (!q.exec() || !q.next()) {
        return Result<Character>::err(
            AppError::notFound("Character not found: " + id.toString()));
    }
    return Result<Character>::ok(hydrateCharacter(q));
}

// ── findAll ───────────────────────────────────────────────────────────────────

Result<std::vector<Character>> SqliteCharacterRepository::findAll() const
{
    auto q = m_db.makeQuery();
    q.exec(R"(
        SELECT id, name, species, biography, image_ref,
               born_era, born_year, died_era, died_year
        FROM characters ORDER BY name
    )");

    std::vector<Character> result;
    while (q.next()) {
        result.push_back(hydrateCharacter(q));
    }
    return Result<std::vector<Character>>::ok(std::move(result));
}

// ── save ──────────────────────────────────────────────────────────────────────

Result<Character> SqliteCharacterRepository::save(const Character& c)
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        INSERT INTO characters
            (name, species, biography, image_ref,
             born_era, born_year, died_era, died_year)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    q.addBindValue(QString::fromStdString(c.name));
    q.addBindValue(QString::fromStdString(c.species));
    q.addBindValue(QString::fromStdString(c.biography));
    q.addBindValue(QString::fromStdString(c.imageRef));
    q.addBindValue(c.born.era);
    q.addBindValue(c.born.year);
    q.addBindValue(c.died ? QVariant(c.died->era)  : QVariant());
    q.addBindValue(c.died ? QVariant(c.died->year) : QVariant());

    if (!q.exec()) {
        return Result<Character>::err(
            AppError::dbError("Insert character failed: "
                              + q.lastError().text().toStdString()));
    }

    Character saved = c;
    saved.id = CharacterId(q.lastInsertId().toLongLong());

    m_logger.debug("Character saved: " + saved.name, "CharacterRepo");
    return Result<Character>::ok(std::move(saved));
}

// ── update ────────────────────────────────────────────────────────────────────

Result<Character> SqliteCharacterRepository::update(const Character& c)
{
    CF_REQUIRE(c.id.isValid(), "Cannot update character with invalid ID");

    auto q = m_db.makeQuery();
    q.prepare(R"(
        UPDATE characters SET
            name = ?, species = ?, biography = ?, image_ref = ?,
            born_era = ?, born_year = ?, died_era = ?, died_year = ?
        WHERE id = ?
    )");
    q.addBindValue(QString::fromStdString(c.name));
    q.addBindValue(QString::fromStdString(c.species));
    q.addBindValue(QString::fromStdString(c.biography));
    q.addBindValue(QString::fromStdString(c.imageRef));
    q.addBindValue(c.born.era);
    q.addBindValue(c.born.year);
    q.addBindValue(c.died ? QVariant(c.died->era)  : QVariant());
    q.addBindValue(c.died ? QVariant(c.died->year) : QVariant());
    q.addBindValue(static_cast<qlonglong>(c.id.value()));

    if (!q.exec()) {
        return Result<Character>::err(
            AppError::dbError("Update character failed: "
                              + q.lastError().text().toStdString()));
    }
    return Result<Character>::ok(c);
}

// ── remove ────────────────────────────────────────────────────────────────────

Result<void> SqliteCharacterRepository::remove(CharacterId id)
{
    auto q = m_db.makeQuery();
    q.prepare("DELETE FROM characters WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));

    if (!q.exec()) {
        return Result<void>::err(
            AppError::dbError("Delete character failed: "
                              + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── exists ────────────────────────────────────────────────────────────────────

bool SqliteCharacterRepository::exists(CharacterId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT 1 FROM characters WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    return q.exec() && q.next();
}

// ── findByNameContaining ──────────────────────────────────────────────────────

Result<std::vector<Character>>
SqliteCharacterRepository::findByNameContaining(const std::string& fragment) const
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        SELECT id, name, species, biography, image_ref,
               born_era, born_year, died_era, died_year
        FROM characters WHERE name LIKE ? ORDER BY name
    )");
    q.addBindValue(QString::fromStdString("%" + fragment + "%"));

    std::vector<Character> result;
    if (q.exec()) {
        while (q.next()) result.push_back(hydrateCharacter(q));
    }
    return Result<std::vector<Character>>::ok(std::move(result));
}

// ── findByPlace ───────────────────────────────────────────────────────────────

Result<std::vector<Character>>
SqliteCharacterRepository::findByPlace(PlaceId placeId) const
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        SELECT DISTINCT c.id, c.name, c.species, c.biography, c.image_ref,
               c.born_era, c.born_year, c.died_era, c.died_year
        FROM characters c
        JOIN character_locations cl ON cl.character_id = c.id
        WHERE cl.place_id = ?
        ORDER BY c.name
    )");
    q.addBindValue(static_cast<qlonglong>(placeId.value()));

    std::vector<Character> result;
    if (q.exec()) {
        while (q.next()) result.push_back(hydrateCharacter(q));
    }
    return Result<std::vector<Character>>::ok(std::move(result));
}

// ── findCurrentlyAtPlace ──────────────────────────────────────────────────────

Result<std::vector<Character>>
SqliteCharacterRepository::findCurrentlyAtPlace(PlaceId placeId) const
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        SELECT DISTINCT c.id, c.name, c.species, c.biography, c.image_ref,
               c.born_era, c.born_year, c.died_era, c.died_year
        FROM characters c
        JOIN character_locations cl ON cl.character_id = c.id
        WHERE cl.place_id = ? AND cl.to_era IS NULL
        ORDER BY c.name
    )");
    q.addBindValue(static_cast<qlonglong>(placeId.value()));

    std::vector<Character> result;
    if (q.exec()) {
        while (q.next()) result.push_back(hydrateCharacter(q));
    }
    return Result<std::vector<Character>>::ok(std::move(result));
}

// ── findByFaction ─────────────────────────────────────────────────────────────

Result<std::vector<Character>>
SqliteCharacterRepository::findByFaction(FactionId factionId) const
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        SELECT DISTINCT c.id, c.name, c.species, c.biography, c.image_ref,
               c.born_era, c.born_year, c.died_era, c.died_year
        FROM characters c
        JOIN character_faction_memberships m ON m.character_id = c.id
        WHERE m.faction_id = ?
        ORDER BY c.name
    )");
    q.addBindValue(static_cast<qlonglong>(factionId.value()));

    std::vector<Character> result;
    if (q.exec()) {
        while (q.next()) result.push_back(hydrateCharacter(q));
    }
    return Result<std::vector<Character>>::ok(std::move(result));
}

// ── saveRelationship ──────────────────────────────────────────────────────────

Result<void> SqliteCharacterRepository::saveRelationship(
    CharacterId ownerId, const Relationship& rel)
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        INSERT OR REPLACE INTO character_relationships
            (owner_id, target_id, type, note,
             since_era, since_year, until_era, until_year)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    q.addBindValue(static_cast<qlonglong>(ownerId.value()));
    q.addBindValue(static_cast<qlonglong>(rel.targetId.value()));
    q.addBindValue(QString::fromStdString(Relationship::typeToString(rel.type)));
    q.addBindValue(QString::fromStdString(rel.note));
    q.addBindValue(rel.since.era);
    q.addBindValue(rel.since.year);
    q.addBindValue(rel.until ? QVariant(rel.until->era)  : QVariant());
    q.addBindValue(rel.until ? QVariant(rel.until->year) : QVariant());

    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Save relationship failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── removeRelationship ────────────────────────────────────────────────────────

Result<void> SqliteCharacterRepository::removeRelationship(
    CharacterId ownerId, CharacterId targetId)
{
    auto q = m_db.makeQuery();
    q.prepare("DELETE FROM character_relationships WHERE owner_id=? AND target_id=?");
    q.addBindValue(static_cast<qlonglong>(ownerId.value()));
    q.addBindValue(static_cast<qlonglong>(targetId.value()));
    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Remove relationship failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── saveEvolution ─────────────────────────────────────────────────────────────

Result<void> SqliteCharacterRepository::saveEvolution(
    CharacterId characterId, const CharacterEvolution& evolution)
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        INSERT OR REPLACE INTO character_evolutions
            (character_id, era, year, description)
        VALUES (?, ?, ?, ?)
    )");
    q.addBindValue(static_cast<qlonglong>(characterId.value()));
    q.addBindValue(evolution.at.era);
    q.addBindValue(evolution.at.year);
    q.addBindValue(QString::fromStdString(evolution.description));

    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Save evolution failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── removeEvolution ───────────────────────────────────────────────────────────

Result<void> SqliteCharacterRepository::removeEvolution(
    CharacterId characterId, const TimePoint& at)
{
    auto q = m_db.makeQuery();
    q.prepare("DELETE FROM character_evolutions "
              "WHERE character_id=? AND era=? AND year=?");
    q.addBindValue(static_cast<qlonglong>(characterId.value()));
    q.addBindValue(at.era);
    q.addBindValue(at.year);
    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Remove evolution failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── saveLocationStint ─────────────────────────────────────────────────────────

Result<void> SqliteCharacterRepository::saveLocationStint(
    CharacterId characterId, const LocationStint& stint)
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        INSERT INTO character_locations
            (character_id, place_id,
             from_era, from_year, to_era, to_year, note)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    q.addBindValue(static_cast<qlonglong>(characterId.value()));
    q.addBindValue(static_cast<qlonglong>(stint.placeId.value()));
    q.addBindValue(stint.from.era);
    q.addBindValue(stint.from.year);
    q.addBindValue(stint.to ? QVariant(stint.to->era)  : QVariant());
    q.addBindValue(stint.to ? QVariant(stint.to->year) : QVariant());
    q.addBindValue(QString::fromStdString(stint.note));

    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Save location stint failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── removeLocationStint ───────────────────────────────────────────────────────

Result<void> SqliteCharacterRepository::removeLocationStint(
    CharacterId characterId, const TimePoint& from)
{
    auto q = m_db.makeQuery();
    q.prepare("DELETE FROM character_locations "
              "WHERE character_id=? AND from_era=? AND from_year=?");
    q.addBindValue(static_cast<qlonglong>(characterId.value()));
    q.addBindValue(from.era);
    q.addBindValue(from.year);
    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Remove location stint failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── saveFactionMembership ─────────────────────────────────────────────────────

Result<void> SqliteCharacterRepository::saveFactionMembership(
    CharacterId characterId, const FactionMembership& m)
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        INSERT INTO character_faction_memberships
            (character_id, faction_id, from_era, from_year,
             to_era, to_year, role)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    q.addBindValue(static_cast<qlonglong>(characterId.value()));
    q.addBindValue(static_cast<qlonglong>(m.factionId.value()));
    q.addBindValue(m.from.era);
    q.addBindValue(m.from.year);
    q.addBindValue(m.to ? QVariant(m.to->era)  : QVariant());
    q.addBindValue(m.to ? QVariant(m.to->year) : QVariant());
    q.addBindValue(QString::fromStdString(m.role));

    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Save faction membership failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── removeFactionMembership ───────────────────────────────────────────────────

Result<void> SqliteCharacterRepository::removeFactionMembership(
    CharacterId characterId, FactionId factionId)
{
    auto q = m_db.makeQuery();
    q.prepare("DELETE FROM character_faction_memberships "
              "WHERE character_id=? AND faction_id=?");
    q.addBindValue(static_cast<qlonglong>(characterId.value()));
    q.addBindValue(static_cast<qlonglong>(factionId.value()));
    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Remove faction membership failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── Private hydration helpers ─────────────────────────────────────────────────

Character SqliteCharacterRepository::hydrateCharacter(QSqlQuery& q) const
{
    Character c;
    c.id        = CharacterId(q.value("id").toLongLong());
    c.name      = q.value("name").toString().toStdString();
    c.species   = q.value("species").toString().toStdString();
    c.biography = q.value("biography").toString().toStdString();
    c.imageRef  = q.value("image_ref").toString().toStdString();
    c.born      = timePointFromCols(q.value("born_era"), q.value("born_year"));
    c.died      = optTimePoint(q.value("died_era"), q.value("died_year"));

    const int64_t id = c.id.value();
    c.evolutions          = loadEvolutions(id);
    c.locationHistory     = loadLocationStints(id);
    c.factionMemberships  = loadMemberships(id);
    c.relationships       = loadRelationships(id);

    return c;
}

std::vector<CharacterEvolution>
SqliteCharacterRepository::loadEvolutions(int64_t charId) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT era, year, description FROM character_evolutions "
              "WHERE character_id=? ORDER BY era, year");
    q.addBindValue(static_cast<qlonglong>(charId));
    q.exec();

    std::vector<CharacterEvolution> result;
    while (q.next()) {
        result.push_back({
            timePointFromCols(q.value("era"), q.value("year")),
            q.value("description").toString().toStdString()
        });
    }
    return result;
}

std::vector<LocationStint>
SqliteCharacterRepository::loadLocationStints(int64_t charId) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT place_id, from_era, from_year, to_era, to_year, note "
              "FROM character_locations WHERE character_id=? ORDER BY from_era, from_year");
    q.addBindValue(static_cast<qlonglong>(charId));
    q.exec();

    std::vector<LocationStint> result;
    while (q.next()) {
        LocationStint s;
        s.placeId = PlaceId(q.value("place_id").toLongLong());
        s.from    = timePointFromCols(q.value("from_era"), q.value("from_year"));
        s.to      = optTimePoint(q.value("to_era"), q.value("to_year"));
        s.note    = q.value("note").toString().toStdString();
        result.push_back(std::move(s));
    }
    return result;
}

std::vector<FactionMembership>
SqliteCharacterRepository::loadMemberships(int64_t charId) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT faction_id, from_era, from_year, to_era, to_year, role "
              "FROM character_faction_memberships "
              "WHERE character_id=? ORDER BY from_era, from_year");
    q.addBindValue(static_cast<qlonglong>(charId));
    q.exec();

    std::vector<FactionMembership> result;
    while (q.next()) {
        FactionMembership m;
        m.factionId = FactionId(q.value("faction_id").toLongLong());
        m.from      = timePointFromCols(q.value("from_era"), q.value("from_year"));
        m.to        = optTimePoint(q.value("to_era"), q.value("to_year"));
        m.role      = q.value("role").toString().toStdString();
        result.push_back(std::move(m));
    }
    return result;
}

std::vector<Relationship>
SqliteCharacterRepository::loadRelationships(int64_t charId) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT target_id, type, note, "
              "       since_era, since_year, until_era, until_year "
              "FROM character_relationships WHERE owner_id=?");
    q.addBindValue(static_cast<qlonglong>(charId));
    q.exec();

    std::vector<Relationship> result;
    while (q.next()) {
        Relationship r;
        r.targetId = CharacterId(q.value("target_id").toLongLong());
        r.type     = Relationship::typeFromString(
                         q.value("type").toString().toStdString());
        r.note     = q.value("note").toString().toStdString();
        r.since    = timePointFromCols(q.value("since_era"), q.value("since_year"));
        r.until    = optTimePoint(q.value("until_era"), q.value("until_year"));
        result.push_back(std::move(r));
    }
    return result;
}

} // namespace CF::Infra