#pragma once

#include "repositories/ICharacterRepository.h"
#include "infrastructure/database/DatabaseManager.h"
#include "core/Logger.h"

namespace CF::Infra {

class SqliteCharacterRepository final : public Repositories::ICharacterRepository {
public:
    explicit SqliteCharacterRepository(DatabaseManager& db,
                                       Core::Logger&    logger);

    // ── IRepository ──────────────────────────────────────────────────────────
    Core::Result<Domain::Character>              findById(Domain::CharacterId id) const override;
    Core::Result<std::vector<Domain::Character>> findAll()                        const override;
    Core::Result<Domain::Character>              save(const Domain::Character& c)       override;
    Core::Result<Domain::Character>              update(const Domain::Character& c)     override;
    Core::Result<void>                           remove(Domain::CharacterId id)         override;
    bool                                         exists(Domain::CharacterId id)   const override;

    // ── ICharacterRepository ─────────────────────────────────────────────────
    Core::Result<std::vector<Domain::Character>> findByNameContaining(const std::string& f)      const override;
    Core::Result<std::vector<Domain::Character>> findByPlace(Domain::PlaceId id)                  const override;
    Core::Result<std::vector<Domain::Character>> findCurrentlyAtPlace(Domain::PlaceId id)         const override;
    Core::Result<std::vector<Domain::Character>> findByFaction(Domain::FactionId id)              const override;
    Core::Result<void> saveRelationship(Domain::CharacterId, const Domain::Relationship&)               override;
    Core::Result<void> removeRelationship(Domain::CharacterId, Domain::CharacterId)                     override;
    Core::Result<void> saveEvolution(Domain::CharacterId, const Domain::CharacterEvolution&)            override;
    Core::Result<void> removeEvolution(Domain::CharacterId, const Domain::TimePoint&)                   override;
    Core::Result<void> saveLocationStint(Domain::CharacterId, const Domain::LocationStint&)             override;
    Core::Result<void> removeLocationStint(Domain::CharacterId, const Domain::TimePoint&)               override;
    Core::Result<void> saveFactionMembership(Domain::CharacterId, const Domain::FactionMembership&)     override;
    Core::Result<void> removeFactionMembership(Domain::CharacterId, Domain::FactionId)                  override;

private:
    // Builds a full Character (with all sub-tables) from a row
    [[nodiscard]] Domain::Character           hydrateCharacter(QSqlQuery& q)                const;
    [[nodiscard]] std::vector<Domain::CharacterEvolution> loadEvolutions(int64_t charId)    const;
    [[nodiscard]] std::vector<Domain::LocationStint>      loadLocationStints(int64_t charId)const;
    [[nodiscard]] std::vector<Domain::FactionMembership>  loadMemberships(int64_t charId)   const;
    [[nodiscard]] std::vector<Domain::Relationship>       loadRelationships(int64_t charId) const;

    DatabaseManager& m_db;
    Core::Logger&    m_logger;
};

} // namespace CF::Infra