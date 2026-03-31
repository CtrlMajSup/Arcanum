#include "TimelineViewModel.h"
#include "core/Assert.h"

#include <algorithm>

namespace CF::UI {

using namespace CF::Domain;
using namespace CF::Core;

TimelineViewModel::TimelineViewModel(
    std::shared_ptr<Services::TimelineService>  timelineService,
    std::shared_ptr<Services::CharacterService> characterService,
    std::shared_ptr<Services::FactionService>   factionService,
    std::shared_ptr<Services::PlaceService>     placeService,
    Core::Logger& logger,
    QObject* parent)
    : QObject(parent)
    , m_timelineService(std::move(timelineService))
    , m_characterService(std::move(characterService))
    , m_factionService(std::move(factionService))
    , m_placeService(std::move(placeService))
    , m_logger(logger)
{
    CF_REQUIRE(m_timelineService != nullptr, "TimelineService must not be null");
}

// ── Data loading ──────────────────────────────────────────────────────────────

void TimelineViewModel::loadAll()
{
    auto result = m_timelineService->getAll();
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    m_allEvents = std::move(result.value());

    // Sort chronologically
    std::sort(m_allEvents.begin(), m_allEvents.end(),
        [](const TimelineEvent& a, const TimelineEvent& b) {
            return a.when < b.when;
        });

    // Set world bounds from the full event set
    auto boundsResult = m_timelineService->getWorldTimeBounds();
    if (boundsResult.isOk()) {
        m_worldStart = boundsResult.value().first;
        m_worldEnd   = boundsResult.value().second;
        // Initialise window to full world span
        m_windowStart = m_worldStart;
        m_windowEnd   = m_worldEnd;
    }else {
        m_logger.debug(
            "World bounds ERROR: "
            + boundsResult.error().message,
            "TimelineViewModel"
        );
    }

    recomputeVisibleEvents();
    emit eventsChanged();
    emit windowChanged();

    m_logger.debug("Timeline loaded: "
                   + std::to_string(m_allEvents.size()) + " events",
                   "TimelineViewModel");
 
    m_logger.debug(
        QString("World bounds: %1 -> %2")
            .arg(QString::fromStdString(boundsResult.value().first.display()))
            .arg(QString::fromStdString(boundsResult.value().second.display()))
            .toStdString(),
        "TimelineViewModel"
    );

}

void TimelineViewModel::loadRange(const TimePoint& from, const TimePoint& to)
{
    auto result = m_timelineService->getInRange(from, to);
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    m_allEvents = std::move(result.value());
    recomputeVisibleEvents();
    emit eventsChanged();
}

// ── Window management ─────────────────────────────────────────────────────────

void TimelineViewModel::setWindow(const TimePoint& from, const TimePoint& to)
{
    if (to < from) return;
    m_windowStart = from;
    m_windowEnd   = to;
    recomputeVisibleEvents();
    emit windowChanged();

    m_logger.debug(
        "Window set: "
        + from.display() + " -> " + to.display(),
        "TimelineViewModel"
    );

}

void TimelineViewModel::panBy(int years)
{
    // Convert window to sort-key arithmetic
    const int64_t span   = m_windowEnd.toSortKey() - m_windowStart.toSortKey();
    int64_t newStart     = m_windowStart.toSortKey() + static_cast<int64_t>(years);
    int64_t newEnd       = newStart + span;

    // Clamp to world bounds
    const int64_t worldS = m_worldStart.toSortKey();
    const int64_t worldE = m_worldEnd.toSortKey();
    if (newStart < worldS) { newStart = worldS; newEnd = worldS + span; }
    if (newEnd   > worldE) { newEnd   = worldE; newStart = worldE - span; }

    m_windowStart = TimePoint::fromSortKey(newStart);
    m_windowEnd   = TimePoint::fromSortKey(newEnd);
    recomputeVisibleEvents();
    emit windowChanged();
}

void TimelineViewModel::zoom(double factor)
{
    CF_ASSERT(factor > 0.0, "Zoom factor must be positive");

    const int64_t centre = (m_windowStart.toSortKey()
                           + m_windowEnd.toSortKey()) / 2;
    const int64_t halfSpan = static_cast<int64_t>(
        static_cast<double>(m_windowEnd.toSortKey()
                            - m_windowStart.toSortKey()) / 2.0 / factor);

    // Enforce minimum visible span of 10 years
    const int64_t minHalfSpan = 5;
    const int64_t clampedHalf = std::max(halfSpan, minHalfSpan);

    m_windowStart = TimePoint::fromSortKey(
        std::max(m_worldStart.toSortKey(), centre - clampedHalf));
    m_windowEnd   = TimePoint::fromSortKey(
        std::min(m_worldEnd.toSortKey(),   centre + clampedHalf));

    recomputeVisibleEvents();
    emit windowChanged();
}

TimePoint TimelineViewModel::windowStart() const { return m_windowStart; }
TimePoint TimelineViewModel::windowEnd()   const { return m_windowEnd;   }
TimePoint TimelineViewModel::worldStart()  const { return m_worldStart;  }
TimePoint TimelineViewModel::worldEnd()    const { return m_worldEnd;    }

// ── Filter ────────────────────────────────────────────────────────────────────

void TimelineViewModel::setLaneVisible(DisplayEvent::Lane lane, bool visible)
{
    m_laneVisible[static_cast<int>(lane)] = visible;
    recomputeVisibleEvents();
    emit eventsChanged();
}

bool TimelineViewModel::isLaneVisible(DisplayEvent::Lane lane) const
{
    return m_laneVisible[static_cast<int>(lane)];
}

// ── Queries ───────────────────────────────────────────────────────────────────

const std::vector<TimelineViewModel::DisplayEvent>&
TimelineViewModel::visibleEvents() const
{
    return m_visibleEvents;
}

std::optional<TimelineViewModel::DisplayEvent>
TimelineViewModel::eventAtNormalisedX(
    double nx, DisplayEvent::Lane lane, double toleranceNx) const
{
    for (const auto& e : m_visibleEvents) {
        if (e.lane != lane) continue;
        if (std::abs(e.normalisedX - nx) <= toleranceNx) return e;
    }
    return std::nullopt;
}

// ── Custom event creation ─────────────────────────────────────────────────────

void TimelineViewModel::createCustomEvent(
    const QString& title, const QString& description, const TimePoint& when)
{
    // Custom events are not tied to a specific entity — use a placeholder
    // CharacterId(0) signals "no subject" — the service accepts this
    auto result = m_timelineService->createCustomEvent(
        title.toStdString(),
        description.toStdString(),
        when,
        CharacterId(0)  // No subject for custom events
    );

    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    // Append and re-sort
    m_allEvents.push_back(result.value());
    std::sort(m_allEvents.begin(), m_allEvents.end(),
        [](const TimelineEvent& a, const TimelineEvent& b) {
            return a.when < b.when;
        });

    recomputeVisibleEvents();
    emit eventsChanged();
}

// ── Private helpers ───────────────────────────────────────────────────────────

void TimelineViewModel::recomputeVisibleEvents()
{
    m_visibleEvents.clear();

    const int64_t winStart = m_windowStart.toSortKey();
    const int64_t winEnd   = m_windowEnd.toSortKey();
    const double  span     = static_cast<double>(winEnd - winStart);
    if (span <= 0.0) return;

    for (const auto& event : m_allEvents) {
        const int64_t key = event.when.toSortKey();
        if (key < winStart || key > winEnd) continue;

        const DisplayEvent::Lane lane = laneForEvent(event);
        if (!m_laneVisible[static_cast<int>(lane)]) continue;

        DisplayEvent de;
        de.id          = event.id;
        de.when        = event.when;
        de.title       = QString::fromStdString(event.title);
        de.entityName  = resolveEntityName(event);
        de.type        = event.type;
        de.lane        = lane;
        de.normalisedX = static_cast<double>(key - winStart) / span;

        m_visibleEvents.push_back(std::move(de));
    }
}

void TimelineViewModel::recomputeNormalisedPositions()
{
    const int64_t winStart = m_windowStart.toSortKey();
    const double  span     = static_cast<double>(
        m_windowEnd.toSortKey() - winStart);
    if (span <= 0.0) return;

    for (auto& e : m_visibleEvents) {
        e.normalisedX = static_cast<double>(
            e.when.toSortKey() - winStart) / span;
    }
}

QString TimelineViewModel::resolveEntityName(const TimelineEvent& e) const
{
    // Best-effort name resolution — falls back to ID if service call fails
    return std::visit([&](const auto& id) -> QString {
        using T = std::decay_t<decltype(id)>;

        if constexpr (std::is_same_v<T, CharacterId>) {
            if (auto r = m_characterService->getById(id); r.isOk())
                return QString::fromStdString(r.value().name);
        }
        else if constexpr (std::is_same_v<T, FactionId>) {
            if (auto r = m_factionService->getById(id); r.isOk())
                return QString::fromStdString(r.value().name);
        }
        else if constexpr (std::is_same_v<T, PlaceId>) {
            if (auto r = m_placeService->getById(id); r.isOk())
                return QString::fromStdString(r.value().name);
        }
        return QString("#%1").arg(id.value());
    }, e.subject);
}

TimelineViewModel::DisplayEvent::Lane
TimelineViewModel::laneForEvent(const TimelineEvent& e) const
{
    using ET = TimelineEvent::EventType;
    switch (e.type) {
        case ET::CharacterBorn:
        case ET::CharacterDied:
        case ET::CharacterEvolved:
        case ET::CharacterMoved:
        case ET::CharacterJoinedFaction:
        case ET::CharacterLeftFaction:
            return DisplayEvent::Lane::Character;

        case ET::FactionFounded:
        case ET::FactionDissolved:
        case ET::FactionEvolved:
            return DisplayEvent::Lane::Faction;

        case ET::PlaceEstablished:
        case ET::PlaceDestroyed:
        case ET::PlaceEvolved:
            return DisplayEvent::Lane::Place;

        case ET::SceneOccurred:
            return DisplayEvent::Lane::Scene;

        case ET::Custom:
        default:
            return DisplayEvent::Lane::Custom;
    }
}

} // namespace CF::UI