#pragma once

#include "IRepository.h"
#include "domain/entities/Character.h"
#include "domain/valueobjects/TimePoint.h"

#include <string>
#include <vector>

namespace CF::Repositories {

/**
 * ICharacterRepository extends the generic CRUD interface with
 * character-specific queries needed by CharacterService and
 * SearchService.
 *
 * All temporal query methods return the state of the data model
 * at or before the given TimePoint — they do NOT filter live entities
 * from the DB; that logic lives in the service layer.
 */
class ICharacterRepository
    : public IRepository<Domain::Character, Domain::CharacterId>
{
public:
    ~ICharacterRepository() override = default;

    // ── Name search ──────────────────────────────────────────────────────────

    // Case-insensitive partial match on name
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Character>>
    findByNameContaining(const std::string& fragment) const = 0;

    // ── Location queries ─────────────────────────────────────────────────────

    // All characters whose location history includes this place at any time
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Character>>
    findByPlace(Domain::PlaceId placeId) const = 0;

    // Characters currently (latest stint) located at a place
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Character>>
    findCurrentlyAtPlace(Domain::PlaceId placeId) const = 0;

    // ── Faction queries ───────────────────────────────────────────────────────

    [[nodiscard]] virtual Core::Result<std::vector<Domain::Character>>
    findByFaction(Domain::FactionId factionId) const = 0;

    // ── Relationship persistence ──────────────────────────────────────────────
    // Relationships are stored in a join table but loaded eagerly with Character.
    // These methods allow updating relationships independently.

    [[nodiscard]] virtual Core::Result<void>
    saveRelationship(Domain::CharacterId ownerId,
                     const Domain::Relationship& rel) = 0;

    [[nodiscard]] virtual Core::Result<void>
    removeRelationship(Domain::CharacterId ownerId,
                       Domain::CharacterId targetId) = 0;

    // ── Evolution persistence ─────────────────────────────────────────────────

    [[nodiscard]] virtual Core::Result<void>
    saveEvolution(Domain::CharacterId characterId,
                  const Domain::CharacterEvolution& evolution) = 0;

    [[nodiscard]] virtual Core::Result<void>
    removeEvolution(Domain::CharacterId characterId,
                    const Domain::TimePoint& at) = 0;

    // ── Location stint persistence ────────────────────────────────────────────

    [[nodiscard]] virtual Core::Result<void>
    saveLocationStint(Domain::CharacterId characterId,
                      const Domain::LocationStint& stint) = 0;

    [[nodiscard]] virtual Core::Result<void>
    removeLocationStint(Domain::CharacterId characterId,
                        const Domain::TimePoint& from) = 0;

    // ── Faction membership persistence ────────────────────────────────────────

    [[nodiscard]] virtual Core::Result<void>
    saveFactionMembership(Domain::CharacterId characterId,
                          const Domain::FactionMembership& membership) = 0;

    [[nodiscard]] virtual Core::Result<void>
    removeFactionMembership(Domain::CharacterId characterId,
                            Domain::FactionId factionId) = 0;
};

} // namespace CF::Repositories