#pragma once

#include "core/Result.h"
#include "core/Logger.h"
#include "domain/entities/TimelineEvent.h"
#include "repositories/ITimelineRepository.h"

#include <memory>
#include <vector>

namespace CF::Services {

/**
 * TimelineService is a read-focused service.
 * Writing timeline events is done by other services (Character, Place, Faction)
 * as a side-effect of their mutations.
 *
 * TimelineService is responsible for:
 *  - Querying events for the timeline widget
 *  - Providing world time bounds
 *  - Manual event creation (for custom user-defined events)
 */
class TimelineService {
public:
    explicit TimelineService(
        std::shared_ptr<Repositories::ITimelineRepository> timelineRepo,
        Core::Logger& logger);

    [[nodiscard]] Core::Result<std::vector<Domain::TimelineEvent>>
    getAll() const;

    [[nodiscard]] Core::Result<std::vector<Domain::TimelineEvent>>
    getInRange(const Domain::TimePoint& from,
               const Domain::TimePoint& to) const;

    [[nodiscard]] Core::Result<std::vector<Domain::TimelineEvent>>
    getForCharacter(Domain::CharacterId id) const;

    [[nodiscard]] Core::Result<std::vector<Domain::TimelineEvent>>
    getForFaction(Domain::FactionId id) const;

    [[nodiscard]] Core::Result<std::vector<Domain::TimelineEvent>>
    getForPlace(Domain::PlaceId id) const;

    // Returns {earliest, latest} TimePoint recorded in the world
    // Used to initialise the timeline widget's visible range
    [[nodiscard]] Core::Result<std::pair<Domain::TimePoint, Domain::TimePoint>>
    getWorldTimeBounds() const;

    // Creates a custom, manually authored event
    [[nodiscard]] Core::Result<Domain::TimelineEvent>
    createCustomEvent(const std::string&       title,
                      const std::string&       description,
                      const Domain::TimePoint& when,
                      Domain::TimelineEvent::Subject subject);

    [[nodiscard]] Core::Result<void>
    deleteEvent(Domain::TimelineId id);

private:
    std::shared_ptr<Repositories::ITimelineRepository> m_timelineRepo;
    Core::Logger& m_logger;
};

} // namespace CF::Services