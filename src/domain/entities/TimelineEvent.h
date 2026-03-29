#pragma once

#include "domain/valueobjects/EntityId.h"
#include "domain/valueobjects/TimePoint.h"

#include <string>
#include <variant>
#include <optional>

namespace CF::Domain {

/**
 * TimelineEvent is the universal record of anything that happens
 * at a specific point in time. Every change in the world is a
 * TimelineEvent, regardless of which entity it concerns.
 *
 * The 'subject' variant identifies what kind of entity changed.
 * This allows the TimelineWidget to filter by entity type or
 * show all events in chronological order.
 *
 * Examples:
 *  - Character born:       subject=CharacterId, type=CharacterBorn
 *  - Faction dissolved:    subject=FactionId,   type=FactionDissolved
 *  - Character moved:      subject=CharacterId, type=CharacterMoved
 *  - Scene occurred:       subject=SceneId,     type=SceneOccurred
 */
struct TimelineEvent {

    enum class EventType {
        // Character events
        CharacterBorn,
        CharacterDied,
        CharacterEvolved,
        CharacterMoved,
        CharacterJoinedFaction,
        CharacterLeftFaction,

        // Faction events
        FactionFounded,
        FactionDissolved,
        FactionEvolved,

        // Place events
        PlaceEstablished,
        PlaceDestroyed,
        PlaceEvolved,

        // Narrative events
        SceneOccurred,

        // Generic user-defined event
        Custom
    };

    using Subject = std::variant<
        CharacterId,
        PlaceId,
        FactionId,
        SceneId
    >;

    TimelineId  id;
    TimePoint   when;
    EventType   type;
    Subject     subject;
    std::string title;       // Short label shown on the timeline bar
    std::string description; // Optional longer description

    // Helper: returns a human-readable type label
    [[nodiscard]] static std::string eventTypeLabel(EventType t) {
        switch (t) {
            case EventType::CharacterBorn:           return "Born";
            case EventType::CharacterDied:           return "Died";
            case EventType::CharacterEvolved:        return "Evolved";
            case EventType::CharacterMoved:          return "Moved";
            case EventType::CharacterJoinedFaction:  return "Joined Faction";
            case EventType::CharacterLeftFaction:    return "Left Faction";
            case EventType::FactionFounded:          return "Founded";
            case EventType::FactionDissolved:        return "Dissolved";
            case EventType::FactionEvolved:          return "Evolved";
            case EventType::PlaceEstablished:        return "Established";
            case EventType::PlaceDestroyed:          return "Destroyed";
            case EventType::PlaceEvolved:            return "Evolved";
            case EventType::SceneOccurred:           return "Scene";
            case EventType::Custom:                  return "Event";
        }
        return "?";
    }

    // Visitor helper to extract the entity id as int64
    [[nodiscard]] int64_t subjectId() const {
        return std::visit([](const auto& id) { return id.value(); }, subject);
    }
};

} // namespace CF::Domain