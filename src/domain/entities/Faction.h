#pragma once

#include "domain/valueobjects/EntityId.h"
#include "domain/valueobjects/TimePoint.h"

#include <string>
#include <vector>
#include <optional>

namespace CF::Domain {

/**
 * Faction: any organised group — empire, guild, crew, religion, etc.
 * Factions have a temporal existence (they can emerge and dissolve).
 */
struct FactionEvolution {
    TimePoint   at;
    std::string description;   // e.g. "Renamed after the coup"
};

struct Faction {
    FactionId   id;
    std::string name;
    std::string type;           // "Empire", "Crew", "Religion", "Guild", etc.
    std::string description;
    std::string iconRef;        // Path to faction emblem, may be empty

    TimePoint              founded;
    std::optional<TimePoint> dissolved;   // Empty = still active

    std::vector<FactionEvolution> evolutions;

    [[nodiscard]] bool isActiveAt(const TimePoint& when) const {
        return founded <= when &&
               (!dissolved.has_value() || *dissolved >= when);
    }
};

} // namespace CF::Domain