#include "SqliteTimelineRepository.h"
#include "core/Assert.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace CF::Infra {

using namespace CF::Domain;
using namespace CF::Core;

SqliteTimelineRepository::SqliteTimelineRepository(
    DatabaseManager& db, Core::Logger& logger)
    : m_db(db), m_logger(logger)
{}

// ── findById ──────────────────────────────────────────────────────────────────

Result<TimelineEvent> SqliteTimelineRepository::findById(TimelineId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM timeline_events WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    if (!q.exec() || !q.next()) {
        return Result<TimelineEvent>::err(
            AppError::notFound("Timeline event not found: " + id.toString()));
    }
    return Result<TimelineEvent>::ok(hydrateEvent(q));
}

// ── findAll ───────────────────────────────────────────────────────────────────

Result<std::vector<TimelineEvent>> SqliteTimelineRepository::findAll() const
{
    auto q = m_db.makeQuery();
    q.exec("SELECT * FROM timeline_events ORDER BY era, year");
    std::vector<TimelineEvent> result;
    while (q.next()) result.push_back(hydrateEvent(q));
    return Result<std::vector<TimelineEvent>>::ok(std::move(result));
}

// ── save ──────────────────────────────────────────────────────────────────────

Result<TimelineEvent> SqliteTimelineRepository::save(const TimelineEvent& e)
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        INSERT INTO timeline_events
            (era, year, event_type, subject_id, subject_type, title, description)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    q.addBindValue(e.when.era);
    q.addBindValue(e.when.year);
    q.addBindValue(QString::fromStdString(eventTypeToString(e.type)));
    q.addBindValue(static_cast<qlonglong>(e.subjectId()));
    q.addBindValue(QString::fromStdString(subjectTypeString(e.subject)));
    q.addBindValue(QString::fromStdString(e.title));
    q.addBindValue(QString::fromStdString(e.description));

    if (!q.exec()) {
        return Result<TimelineEvent>::err(AppError::dbError(
            "Insert timeline event failed: " + q.lastError().text().toStdString()));
    }
    TimelineEvent saved = e;
    saved.id = TimelineId(q.lastInsertId().toLongLong());
    return Result<TimelineEvent>::ok(std::move(saved));
}

// ── update ────────────────────────────────────────────────────────────────────

Result<TimelineEvent> SqliteTimelineRepository::update(const TimelineEvent& e)
{
    CF_REQUIRE(e.id.isValid(), "Cannot update timeline event with invalid ID");

    auto q = m_db.makeQuery();
    q.prepare(R"(
        UPDATE timeline_events SET
            era=?, year=?, event_type=?,
            subject_id=?, subject_type=?,
            title=?, description=?
        WHERE id=?
    )");
    q.addBindValue(e.when.era);
    q.addBindValue(e.when.year);
    q.addBindValue(QString::fromStdString(eventTypeToString(e.type)));
    q.addBindValue(static_cast<qlonglong>(e.subjectId()));
    q.addBindValue(QString::fromStdString(subjectTypeString(e.subject)));
    q.addBindValue(QString::fromStdString(e.title));
    q.addBindValue(QString::fromStdString(e.description));
    q.addBindValue(static_cast<qlonglong>(e.id.value()));

    if (!q.exec()) {
        return Result<TimelineEvent>::err(AppError::dbError(
            "Update timeline event failed: " + q.lastError().text().toStdString()));
    }
    return Result<TimelineEvent>::ok(e);
}

// ── remove ────────────────────────────────────────────────────────────────────

Result<void> SqliteTimelineRepository::remove(TimelineId id)
{
    auto q = m_db.makeQuery();
    q.prepare("DELETE FROM timeline_events WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Delete timeline event failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// ── exists ────────────────────────────────────────────────────────────────────

bool SqliteTimelineRepository::exists(TimelineId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT 1 FROM timeline_events WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    return q.exec() && q.next();
}

// ── findInTimeRange ───────────────────────────────────────────────────────────

Result<std::vector<TimelineEvent>>
SqliteTimelineRepository::findInTimeRange(
    const TimePoint& from, const TimePoint& to) const
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        SELECT * FROM timeline_events
        WHERE (era * 100000 + year) >= ?
          AND (era * 100000 + year) <= ?
        ORDER BY era, year
    )");
    q.addBindValue(static_cast<qlonglong>(from.toSortKey()));
    q.addBindValue(static_cast<qlonglong>(to.toSortKey()));

    std::vector<TimelineEvent> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateEvent(q));
    return Result<std::vector<TimelineEvent>>::ok(std::move(result));
}

// ── findBySubject ─────────────────────────────────────────────────────────────

Result<std::vector<TimelineEvent>>
SqliteTimelineRepository::findBySubject(int64_t subjectId) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM timeline_events "
              "WHERE subject_id=? ORDER BY era, year");
    q.addBindValue(static_cast<qlonglong>(subjectId));

    std::vector<TimelineEvent> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateEvent(q));
    return Result<std::vector<TimelineEvent>>::ok(std::move(result));
}

// ── findByEventType ───────────────────────────────────────────────────────────

Result<std::vector<TimelineEvent>>
SqliteTimelineRepository::findByEventType(TimelineEvent::EventType type) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM timeline_events "
              "WHERE event_type=? ORDER BY era, year");
    q.addBindValue(QString::fromStdString(eventTypeToString(type)));

    std::vector<TimelineEvent> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateEvent(q));
    return Result<std::vector<TimelineEvent>>::ok(std::move(result));
}

// ── findByCharacter / Faction / Place ─────────────────────────────────────────

Result<std::vector<TimelineEvent>>
SqliteTimelineRepository::findByCharacter(CharacterId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM timeline_events "
              "WHERE subject_id=? AND subject_type='character' ORDER BY era, year");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    std::vector<TimelineEvent> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateEvent(q));
    return Result<std::vector<TimelineEvent>>::ok(std::move(result));
}

Result<std::vector<TimelineEvent>>
SqliteTimelineRepository::findByFaction(FactionId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM timeline_events "
              "WHERE subject_id=? AND subject_type='faction' ORDER BY era, year");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    std::vector<TimelineEvent> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateEvent(q));
    return Result<std::vector<TimelineEvent>>::ok(std::move(result));
}

Result<std::vector<TimelineEvent>>
SqliteTimelineRepository::findByPlace(PlaceId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM timeline_events "
              "WHERE subject_id=? AND subject_type='place' ORDER BY era, year");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    std::vector<TimelineEvent> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateEvent(q));
    return Result<std::vector<TimelineEvent>>::ok(std::move(result));
}

// ── earliestEvent / latestEvent ───────────────────────────────────────────────

Result<TimePoint> SqliteTimelineRepository::earliestEvent() const
{
    auto q = m_db.makeQuery();
    q.exec("SELECT era, year FROM timeline_events ORDER BY era, year LIMIT 1");
    if (!q.next())
        return Result<TimePoint>::err(AppError::notFound("No timeline events"));
    return Result<TimePoint>::ok({ q.value("era").toInt(), q.value("year").toInt() });
}

Result<TimePoint> SqliteTimelineRepository::latestEvent() const
{
    auto q = m_db.makeQuery();
    q.exec("SELECT era, year FROM timeline_events "
           "ORDER BY era DESC, year DESC LIMIT 1");
    if (!q.next())
        return Result<TimePoint>::err(AppError::notFound("No timeline events"));
    return Result<TimePoint>::ok({ q.value("era").toInt(), q.value("year").toInt() });
}

// ── Private helpers ───────────────────────────────────────────────────────────

TimelineEvent SqliteTimelineRepository::hydrateEvent(QSqlQuery& q) const
{
    TimelineEvent e;
    e.id          = TimelineId(q.value("id").toLongLong());
    e.when        = { q.value("era").toInt(), q.value("year").toInt() };
    e.title       = q.value("title").toString().toStdString();
    e.description = q.value("description").toString().toStdString();
    e.type        = eventTypeFromString(q.value("event_type").toString().toStdString());

    const int64_t subjectId   = q.value("subject_id").toLongLong();
    const auto    subjectType = q.value("subject_type").toString();

    if      (subjectType == "character") e.subject = CharacterId(subjectId);
    else if (subjectType == "place")     e.subject = PlaceId(subjectId);
    else if (subjectType == "faction")   e.subject = FactionId(subjectId);
    else                                 e.subject = SceneId(subjectId);

    return e;
}

std::string SqliteTimelineRepository::eventTypeToString(TimelineEvent::EventType t)
{
    using ET = TimelineEvent::EventType;
    switch (t) {
        case ET::CharacterBorn:          return "character_born";
        case ET::CharacterDied:          return "character_died";
        case ET::CharacterEvolved:       return "character_evolved";
        case ET::CharacterMoved:         return "character_moved";
        case ET::CharacterJoinedFaction: return "character_joined_faction";
        case ET::CharacterLeftFaction:   return "character_left_faction";
        case ET::FactionFounded:         return "faction_founded";
        case ET::FactionDissolved:       return "faction_dissolved";
        case ET::FactionEvolved:         return "faction_evolved";
        case ET::PlaceEstablished:       return "place_established";
        case ET::PlaceDestroyed:         return "place_destroyed";
        case ET::PlaceEvolved:           return "place_evolved";
        case ET::SceneOccurred:          return "scene_occurred";
        case ET::Custom:                 return "custom";
    }
    return "custom";
}

TimelineEvent::EventType
SqliteTimelineRepository::eventTypeFromString(const std::string& s)
{
    using ET = TimelineEvent::EventType;
    if (s == "character_born")           return ET::CharacterBorn;
    if (s == "character_died")           return ET::CharacterDied;
    if (s == "character_evolved")        return ET::CharacterEvolved;
    if (s == "character_moved")          return ET::CharacterMoved;
    if (s == "character_joined_faction") return ET::CharacterJoinedFaction;
    if (s == "character_left_faction")   return ET::CharacterLeftFaction;
    if (s == "faction_founded")          return ET::FactionFounded;
    if (s == "faction_dissolved")        return ET::FactionDissolved;
    if (s == "faction_evolved")          return ET::FactionEvolved;
    if (s == "place_established")        return ET::PlaceEstablished;
    if (s == "place_destroyed")          return ET::PlaceDestroyed;
    if (s == "place_evolved")            return ET::PlaceEvolved;
    if (s == "scene_occurred")           return ET::SceneOccurred;
    return ET::Custom;
}

std::string SqliteTimelineRepository::subjectTypeString(
    const TimelineEvent::Subject& s)
{
    return std::visit([](const auto& id) -> std::string {
        using T = std::decay_t<decltype(id)>;
        if constexpr (std::is_same_v<T, CharacterId>) return "character";
        if constexpr (std::is_same_v<T, PlaceId>)     return "place";
        if constexpr (std::is_same_v<T, FactionId>)   return "faction";
        if constexpr (std::is_same_v<T, SceneId>)     return "scene";
        return "unknown";
    }, s);
}

} // namespace CF::Infra