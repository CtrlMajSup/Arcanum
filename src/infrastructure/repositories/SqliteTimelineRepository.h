#pragma once

#include "repositories/ITimelineRepository.h"
#include "infrastructure/database/DatabaseManager.h"
#include "core/Logger.h"

namespace CF::Infra {

class SqliteTimelineRepository final : public Repositories::ITimelineRepository {
public:
    explicit SqliteTimelineRepository(DatabaseManager& db, Core::Logger& logger);

    // ── IRepository ──────────────────────────────────────────────────────────
    Core::Result<Domain::TimelineEvent>              findById(Domain::TimelineId id) const override;
    Core::Result<std::vector<Domain::TimelineEvent>> findAll()                       const override;
    Core::Result<Domain::TimelineEvent>              save(const Domain::TimelineEvent& e)   override;
    Core::Result<Domain::TimelineEvent>              update(const Domain::TimelineEvent& e) override;
    Core::Result<void>                               remove(Domain::TimelineId id)          override;
    bool                                             exists(Domain::TimelineId id)    const override;

    // ── ITimelineRepository ──────────────────────────────────────────────────
    Core::Result<std::vector<Domain::TimelineEvent>> findInTimeRange(
        const Domain::TimePoint&, const Domain::TimePoint&) const override;
    Core::Result<std::vector<Domain::TimelineEvent>> findBySubject(int64_t subjectId)               const override;
    Core::Result<std::vector<Domain::TimelineEvent>> findByEventType(Domain::TimelineEvent::EventType) const override;
    Core::Result<std::vector<Domain::TimelineEvent>> findByCharacter(Domain::CharacterId)            const override;
    Core::Result<std::vector<Domain::TimelineEvent>> findByFaction(Domain::FactionId)                const override;
    Core::Result<std::vector<Domain::TimelineEvent>> findByPlace(Domain::PlaceId)                    const override;
    Core::Result<Domain::TimePoint> earliestEvent() const override;
    Core::Result<Domain::TimePoint> latestEvent()   const override;

private:
    [[nodiscard]] Domain::TimelineEvent hydrateEvent(QSqlQuery& q) const;

    // Bidirectional string/enum conversion for EventType and subject type
    static std::string          eventTypeToString(Domain::TimelineEvent::EventType t);
    static Domain::TimelineEvent::EventType eventTypeFromString(const std::string& s);
    static std::string          subjectTypeString(const Domain::TimelineEvent::Subject& s);

    DatabaseManager& m_db;
    Core::Logger&    m_logger;
};

} // namespace CF::Infra