#include "TimelineService.h"
#include "core/Assert.h"

namespace CF::Services {

using namespace CF::Domain;
using namespace CF::Core;

TimelineService::TimelineService(
    std::shared_ptr<Repositories::ITimelineRepository> timelineRepo,
    Core::Logger& logger)
    : m_timelineRepo(std::move(timelineRepo))
    , m_logger(logger)
{
    CF_REQUIRE(m_timelineRepo != nullptr, "TimelineRepository must not be null");
}

Result<std::vector<TimelineEvent>> TimelineService::getAll() const
{
    return m_timelineRepo->findAll();
}

Result<std::vector<TimelineEvent>>
TimelineService::getInRange(const TimePoint& from, const TimePoint& to) const
{
    if (to < from) {
        return Result<std::vector<TimelineEvent>>::err(
            AppError::validation("Range end must be >= range start"));
    }
    return m_timelineRepo->findInTimeRange(from, to);
}

Result<std::vector<TimelineEvent>>
TimelineService::getForCharacter(CharacterId id) const
{
    return m_timelineRepo->findByCharacter(id);
}

Result<std::vector<TimelineEvent>>
TimelineService::getForFaction(FactionId id) const
{
    return m_timelineRepo->findByFaction(id);
}

Result<std::vector<TimelineEvent>>
TimelineService::getForPlace(PlaceId id) const
{
    return m_timelineRepo->findByPlace(id);
}

Result<std::pair<TimePoint, TimePoint>>
TimelineService::getWorldTimeBounds() const
{
    auto earliest = m_timelineRepo->earliestEvent();
    auto latest   = m_timelineRepo->latestEvent();

    // If no events exist yet, return a sensible default range
    const TimePoint defaultStart{ 1, 1 };
    const TimePoint defaultEnd  { 1, 100 };

    const TimePoint from = earliest.isOk() ? earliest.value() : defaultStart;
    const TimePoint to   = latest.isOk()   ? latest.value()   : defaultEnd;

    return Result<std::pair<TimePoint, TimePoint>>::ok({ from, to });
}

Result<TimelineEvent> TimelineService::createCustomEvent(
    const std::string& title,
    const std::string& description,
    const TimePoint&   when,
    TimelineEvent::Subject subject)
{
    if (title.empty()) {
        return Result<TimelineEvent>::err(
            AppError::validation("Event title cannot be empty"));
    }

    TimelineEvent event;
    event.when        = when;
    event.type        = TimelineEvent::EventType::Custom;
    event.subject     = subject;
    event.title       = title;
    event.description = description;

    auto result = m_timelineRepo->save(event);
    if (result.isOk()) {
        m_logger.info("Custom timeline event created: " + title, "TimelineService");
    }
    return result;
}

Result<void> TimelineService::deleteEvent(TimelineId id)
{
    return m_timelineRepo->remove(id);
}

} // namespace CF::Services