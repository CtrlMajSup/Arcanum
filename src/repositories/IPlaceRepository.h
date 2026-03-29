#pragma once

#include "IRepository.h"
#include "domain/entities/Place.h"

#include <string>
#include <vector>

namespace CF::Repositories {

class IPlaceRepository
    : public IRepository<Domain::Place, Domain::PlaceId>
{
public:
    ~IPlaceRepository() override = default;

    // Case-insensitive partial match on name
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Place>>
    findByNameContaining(const std::string& fragment) const = 0;

    // All places belonging to a named region (for the schematic map)
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Place>>
    findByRegion(const std::string& region) const = 0;

    // All distinct region names (used to populate map zone panels)
    [[nodiscard]] virtual Core::Result<std::vector<std::string>>
    allRegions() const = 0;

    // Mobile places only (spaceships, vehicles)
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Place>>
    findMobilePlaces() const = 0;

    // Direct children of a parent place (e.g. rooms in a spaceship)
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Place>>
    findChildren(Domain::PlaceId parentId) const = 0;

    // Persist a place evolution entry
    [[nodiscard]] virtual Core::Result<void>
    saveEvolution(Domain::PlaceId placeId,
                  const Domain::PlaceEvolution& evolution) = 0;

    [[nodiscard]] virtual Core::Result<void>
    removeEvolution(Domain::PlaceId placeId,
                    const Domain::TimePoint& at) = 0;
};

} // namespace CF::Repositories