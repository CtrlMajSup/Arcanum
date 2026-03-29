#pragma once

#include "repositories/IFactionRepository.h"
#include "infrastructure/database/DatabaseManager.h"
#include "core/Logger.h"

namespace CF::Infra {

class SqliteFactionRepository final : public Repositories::IFactionRepository {
public:
    explicit SqliteFactionRepository(DatabaseManager& db, Core::Logger& logger);

    // ── IRepository ──────────────────────────────────────────────────────────
    Core::Result<Domain::Faction>              findById(Domain::FactionId id) const override;
    Core::Result<std::vector<Domain::Faction>> findAll()                      const override;
    Core::Result<Domain::Faction>              save(const Domain::Faction& f)       override;
    Core::Result<Domain::Faction>              update(const Domain::Faction& f)     override;
    Core::Result<void>                         remove(Domain::FactionId id)         override;
    bool                                       exists(Domain::FactionId id)   const override;

    // ── IFactionRepository ───────────────────────────────────────────────────
    Core::Result<std::vector<Domain::Faction>> findByNameContaining(const std::string& fragment) const override;
    Core::Result<std::vector<Domain::Faction>> findActiveAt(const Domain::TimePoint& when)       const override;
    Core::Result<std::vector<Domain::Faction>> findByType(const std::string& type)               const override;
    Core::Result<void> saveEvolution(Domain::FactionId, const Domain::FactionEvolution&)               override;
    Core::Result<void> removeEvolution(Domain::FactionId, const Domain::TimePoint&)                    override;

private:
    [[nodiscard]] Domain::Faction                        hydrateFaction(QSqlQuery& q)        const;
    [[nodiscard]] std::vector<Domain::FactionEvolution>  loadEvolutions(int64_t factionId)   const;

    DatabaseManager& m_db;
    Core::Logger&    m_logger;
};

} // namespace CF::Infra