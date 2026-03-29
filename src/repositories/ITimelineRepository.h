#pragma once

#include "IRepository.h"
#include "domain/entities/TimelineEvent.h"
#include "domain/valueobjects/TimePoint.h"

#include <vector>

namespace CF::Repositories {

/**
 * ITimelineRepository manages TimelineEvents — the universal
 * chronological record of everything that happens in the world.
 *
 * The timeline is append-heavy and read-mostly once written.
 * Bulk time-range queries are the primary access pattern.
 */
class ITimelineRepository
    : public IRepository<Domain::TimelineEvent, Domain::TimelineId>
{
public:
    ~ITimelineRepository() override = default;

    // All events in chronological order within a time window
    [[nodiscard]] virtual Core::Result<std::vector<Domain::TimelineEvent>>
    findInTimeRange(const Domain::TimePoint& from,
                    const Domain::TimePoint& to) const = 0;

    // All events concerning a specific entity (any type)
    [[nodiscard]] virtual Core::Result<std::vector<Domain::TimelineEvent>>
    findBySubject(int64_t subjectId) const = 0;

    // Filtered by event type
    [[nodiscard]] virtual Core::Result<std::vector<Domain::TimelineEvent>>
    findByEventType(Domain::TimelineEvent::EventType type) const = 0;

    // All events for a character (convenience over findBySubject)
    [[nodiscard]] virtual Core::Result<std::vector<Domain::TimelineEvent>>
    findByCharacter(Domain::CharacterId characterId) const = 0;

    [[nodiscard]] virtual Core::Result<std::vector<Domain::TimelineEvent>>
    findByFaction(Domain::FactionId factionId) const = 0;

    [[nodiscard]] virtual Core::Result<std::vector<Domain::TimelineEvent>>
    findByPlace(Domain::PlaceId placeId) const = 0;

    // The earliest and latest TimePoint recorded — used to set timeline bounds
    [[nodiscard]] virtual Core::Result<Domain::TimePoint>
    earliestEvent() const = 0;

    [[nodiscard]] virtual Core::Result<Domain::TimePoint>
    latestEvent() const = 0;
};

} // namespace CF::Repositories