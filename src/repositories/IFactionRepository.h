#pragma once

#include "IRepository.h"
#include "domain/entities/Faction.h"
#include "domain/valueobjects/TimePoint.h"

#include <string>
#include <vector>

namespace CF::Repositories {

class IFactionRepository
    : public IRepository<Domain::Faction, Domain::FactionId>
{
public:
    ~IFactionRepository() override = default;

    [[nodiscard]] virtual Core::Result<std::vector<Domain::Faction>>
    findByNameContaining(const std::string& fragment) const = 0;

    // Factions active at a given in-world time
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Faction>>
    findActiveAt(const Domain::TimePoint& when) const = 0;

    // Factions of a given type ("Empire", "Crew", etc.)
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Faction>>
    findByType(const std::string& type) const = 0;

    [[nodiscard]] virtual Core::Result<void>
    saveEvolution(Domain::FactionId factionId,
                  const Domain::FactionEvolution& evolution) = 0;

    [[nodiscard]] virtual Core::Result<void>
    removeEvolution(Domain::FactionId factionId,
                    const Domain::TimePoint& at) = 0;
};

} // namespace CF::Repositories