#pragma once

#include "domain/valueobjects/EntityId.h"
#include "domain/valueobjects/TimePoint.h"

#include <string>
#include <vector>
#include <optional>

namespace CF::Domain {

/**
 * Place represents any physical location in the world:
 * a city, a spaceship, a moon base, a forest region.
 *
 * Places belong to a Region (the schematic zone grouping for the map).
 * A Place can also be mobile (e.g. a spaceship), tracked via
 * its own location history relative to other places.
 */
struct PlaceEvolution {
    TimePoint   at;
    std::string description;   // e.g. "City destroyed in the Siege"
};

struct Place {
    PlaceId     id;
    std::string name;
    std::string type;           // "City", "Spaceship", "Station", "Region", etc.
    std::string region;         // Zone grouping for the schematic map
    std::string description;
    bool        isMobile = false;  // True for spaceships, vehicles

    // Map coordinates within the schematic canvas (normalised 0.0–1.0)
    double mapX = 0.5;
    double mapY = 0.5;

    // Temporal changes (e.g. city renamed, destroyed, rebuilt)
    std::vector<PlaceEvolution> evolutions;

    // Parent place (e.g. a room inside a spaceship)
    std::optional<PlaceId> parentPlaceId;
};

} // namespace CF::Domain