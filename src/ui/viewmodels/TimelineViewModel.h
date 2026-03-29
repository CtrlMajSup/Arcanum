#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <vector>

#include "services/TimelineService.h"
#include "services/CharacterService.h"
#include "services/FactionService.h"
#include "services/PlaceService.h"
#include "domain/entities/TimelineEvent.h"
#include "domain/valueobjects/TimePoint.h"
#include "core/Logger.h"

namespace CF::UI {

/**
 * TimelineViewModel prepares data for the TimelineWidget.
 *
 * It owns:
 *  - The flat list of all timeline events (sorted chronologically)
 *  - The current visible time window [windowStart, windowEnd]
 *  - Filter state (which entity types are visible)
 *
 * The widget queries this ViewModel for the events to paint —
 * it never calls services directly.
 */
class TimelineViewModel : public QObject {
    Q_OBJECT

public:
    // One display-ready event entry for the painter
    struct DisplayEvent {
        Domain::TimelineId   id;
        Domain::TimePoint    when;
        QString              title;
        QString              entityName;  // Resolved name of the subject entity
        Domain::TimelineEvent::EventType type;

        // opérateurs de comparaison publics
        bool operator==(const DisplayEvent& other) const {
            return id == other.id && title == other.title;
        }

        bool operator!=(const DisplayEvent& other) const {
            return !(*this == other);
        }

        // Which swimlane this event belongs to
        enum class Lane { Character, Place, Faction, Scene, Custom };
        Lane lane;

        // Normalised position in current window [0.0 – 1.0]
        // Computed by ViewModel when window changes
        double normalisedX = 0.0;
    };

    explicit TimelineViewModel(
        std::shared_ptr<Services::TimelineService>  timelineService,
        std::shared_ptr<Services::CharacterService> characterService,
        std::shared_ptr<Services::FactionService>   factionService,
        std::shared_ptr<Services::PlaceService>     placeService,
        Core::Logger& logger,
        QObject* parent = nullptr);

    // ── Data loading ──────────────────────────────────────────────────────────
    void loadAll();
    void loadRange(const Domain::TimePoint& from, const Domain::TimePoint& to);

    // ── Window management ─────────────────────────────────────────────────────

    // Sets the visible time window and recomputes normalisedX for all events
    void setWindow(const Domain::TimePoint& from, const Domain::TimePoint& to);

    // Pan the window by +/- years (positive = forward in time)
    void panBy(int years);

    // Zoom: factor > 1.0 = zoom in (smaller window), < 1.0 = zoom out
    void zoom(double factor);

    [[nodiscard]] Domain::TimePoint windowStart() const;
    [[nodiscard]] Domain::TimePoint windowEnd()   const;
    [[nodiscard]] Domain::TimePoint worldStart()  const;
    [[nodiscard]] Domain::TimePoint worldEnd()    const;

    // ── Filter ────────────────────────────────────────────────────────────────
    void setLaneVisible(DisplayEvent::Lane lane, bool visible);
    [[nodiscard]] bool isLaneVisible(DisplayEvent::Lane lane) const;

    // ── Queries ───────────────────────────────────────────────────────────────
    [[nodiscard]] const std::vector<DisplayEvent>& visibleEvents() const;

    // Returns the event at a normalised X position (for click detection)
    [[nodiscard]] std::optional<DisplayEvent>
    eventAtNormalisedX(double nx, DisplayEvent::Lane lane,
                       double toleranceNx = 0.005) const;

    // ── Custom event creation ─────────────────────────────────────────────────
    void createCustomEvent(const QString& title,
                           const QString& description,
                           const Domain::TimePoint& when);

signals:
    void eventsChanged();
    void windowChanged();
    void errorOccurred(const QString& message);

private:
    void recomputeNormalisedPositions();
    void recomputeVisibleEvents();
    [[nodiscard]] QString resolveEntityName(const Domain::TimelineEvent& e) const;
    [[nodiscard]] DisplayEvent::Lane laneForEvent(const Domain::TimelineEvent& e) const;

    // All loaded events (unfiltered, sorted by TimePoint)
    std::vector<Domain::TimelineEvent> m_allEvents;

    // Filtered + normalised events for the current window
    std::vector<DisplayEvent> m_visibleEvents;

    // Visible time window
    Domain::TimePoint m_windowStart{ 1, 1 };
    Domain::TimePoint m_windowEnd  { 1, 100 };

    // Full world time bounds (from DB extremes)
    Domain::TimePoint m_worldStart{ 1, 1 };
    Domain::TimePoint m_worldEnd  { 1, 100 };

    // Lane visibility flags
    bool m_laneVisible[5] = { true, true, true, true, true };

    std::shared_ptr<Services::TimelineService>  m_timelineService;
    std::shared_ptr<Services::CharacterService> m_characterService;
    std::shared_ptr<Services::FactionService>   m_factionService;
    std::shared_ptr<Services::PlaceService>     m_placeService;
    Core::Logger& m_logger;
};

} // namespace CF::UI