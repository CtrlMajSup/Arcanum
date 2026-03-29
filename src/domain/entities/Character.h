#pragma once

#include "domain/valueobjects/EntityId.h"
#include "domain/valueobjects/TimePoint.h"
#include "domain/valueobjects/Relationship.h"

#include <string>
#include <vector>
#include <optional>

namespace CF::Domain {

/**
 * Character is the central aggregate of the encyclopedia.
 *
 * A character has:
 *  - A fixed identity (id, name, species)
 *  - A mutable biography that evolves across the timeline
 *  - A list of location stints (where they were, and when)
 *  - A list of relationships to other characters
 *  - Faction memberships with temporal bounds
 */
struct CharacterEvolution {
    TimePoint   at;
    std::string description;   // What changed at this point in time
};

struct LocationStint {
    PlaceId                  placeId;
    TimePoint                from;
    std::optional<TimePoint> to;       // Empty = currently here
    std::string              note;

    [[nodiscard]] bool isCurrent() const { return !to.has_value(); }
};

struct FactionMembership {
    FactionId                factionId;
    TimePoint                from;
    std::optional<TimePoint> to;
    std::string              role;     // e.g. "Commander", "Initiate"

    [[nodiscard]] bool isCurrent() const { return !to.has_value(); }
};

struct Character {
    CharacterId  id;
    std::string  name;
    std::string  species;
    std::string  biography;            // Full backstory / static description
    std::string  imageRef;             // Path to portrait, may be empty

    // Temporal data — sorted ascending by TimePoint
    std::vector<CharacterEvolution> evolutions;
    std::vector<LocationStint>      locationHistory;
    std::vector<FactionMembership>  factionMemberships;
    std::vector<Relationship>       relationships;

    TimePoint born;
    std::optional<TimePoint> died;     // Empty = alive

    [[nodiscard]] bool isAlive() const { return !died.has_value(); }

    // Returns the place the character is at or before the given time.
    // Returns nullopt if no location is recorded yet.
    [[nodiscard]] std::optional<PlaceId> locationAt(const TimePoint& when) const {
        std::optional<PlaceId> result;
        for (const auto& stint : locationHistory) {
            if (stint.from <= when) {
                if (!stint.to.has_value() || *stint.to >= when) {
                    result = stint.placeId;
                }
            }
        }
        return result;
    }

    // Returns active faction memberships at a given time
    [[nodiscard]] std::vector<FactionId> factionsAt(const TimePoint& when) const {
        std::vector<FactionId> result;
        for (const auto& m : factionMemberships) {
            if (m.from <= when && (!m.to.has_value() || *m.to >= when)) {
                result.push_back(m.factionId);
            }
        }
        return result;
    }
};

} // namespace CF::Domain