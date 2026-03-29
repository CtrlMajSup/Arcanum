#pragma once

#include "repositories/IPlaceRepository.h"
#include "infrastructure/database/DatabaseManager.h"
#include "core/Logger.h"

namespace CF::Infra {

class SqlitePlaceRepository final : public Repositories::IPlaceRepository {
public:
    explicit SqlitePlaceRepository(DatabaseManager& db, Core::Logger& logger);

    // ── IRepository ──────────────────────────────────────────────────────────
    Core::Result<Domain::Place>              findById(Domain::PlaceId id) const override;
    Core::Result<std::vector<Domain::Place>> findAll()                    const override;
    Core::Result<Domain::Place>              save(const Domain::Place& p)       override;
    Core::Result<Domain::Place>              update(const Domain::Place& p)     override;
    Core::Result<void>                       remove(Domain::PlaceId id)         override;
    bool                                     exists(Domain::PlaceId id)   const override;

    // ── IPlaceRepository ─────────────────────────────────────────────────────
    Core::Result<std::vector<Domain::Place>>   findByNameContaining(const std::string& fragment) const override;
    Core::Result<std::vector<Domain::Place>>   findByRegion(const std::string& region)           const override;
    Core::Result<std::vector<std::string>>     allRegions()                                      const override;
    Core::Result<std::vector<Domain::Place>>   findMobilePlaces()                                const override;
    Core::Result<std::vector<Domain::Place>>   findChildren(Domain::PlaceId parentId)            const override;
    Core::Result<void> saveEvolution(Domain::PlaceId, const Domain::PlaceEvolution&)                   override;
    Core::Result<void> removeEvolution(Domain::PlaceId, const Domain::TimePoint&)                      override;

private:
    [[nodiscard]] Domain::Place                        hydratePlace(QSqlQuery& q)          const;
    [[nodiscard]] std::vector<Domain::PlaceEvolution>  loadEvolutions(int64_t placeId)     const;

    DatabaseManager& m_db;
    Core::Logger&    m_logger;
};

} // namespace CF::Infra