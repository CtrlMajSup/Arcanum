#pragma once

#include "repositories/ISceneRepository.h"
#include "infrastructure/database/DatabaseManager.h"
#include "core/Logger.h"

namespace CF::Infra {

class SqliteSceneRepository final : public Repositories::ISceneRepository {
public:
    explicit SqliteSceneRepository(DatabaseManager& db, Core::Logger& logger);

    // ── IRepository ──────────────────────────────────────────────────────────
    Core::Result<Domain::Scene>              findById(Domain::SceneId id) const override;
    Core::Result<std::vector<Domain::Scene>> findAll()                    const override;
    Core::Result<Domain::Scene>              save(const Domain::Scene& s)       override;
    Core::Result<Domain::Scene>              update(const Domain::Scene& s)     override;
    Core::Result<void>                       remove(Domain::SceneId id)         override;
    bool                                     exists(Domain::SceneId id)   const override;

    // ── ISceneRepository ─────────────────────────────────────────────────────
    Core::Result<std::vector<Domain::Scene>> findByNameContaining(const std::string& f)            const override;
    Core::Result<std::vector<Domain::Scene>> findInTimeRange(const Domain::TimePoint&,
                                                              const Domain::TimePoint&)             const override;
    Core::Result<std::vector<Domain::Scene>> findByPlace(Domain::PlaceId)                          const override;
    Core::Result<std::vector<Domain::Scene>> findByFaction(Domain::FactionId)                      const override;
    Core::Result<std::vector<Domain::Scene>> findByCharacter(Domain::CharacterId)                  const override;
    Core::Result<std::vector<Domain::Scene>> findByChapter(Domain::ChapterId)                      const override;

    Core::Result<void> linkPlace    (Domain::SceneId, Domain::PlaceId)     override;
    Core::Result<void> unlinkPlace  (Domain::SceneId, Domain::PlaceId)     override;
    Core::Result<void> linkFaction  (Domain::SceneId, Domain::FactionId)   override;
    Core::Result<void> unlinkFaction(Domain::SceneId, Domain::FactionId)   override;
    Core::Result<void> linkCharacter    (Domain::SceneId, Domain::CharacterId) override;
    Core::Result<void> unlinkCharacter  (Domain::SceneId, Domain::CharacterId) override;

private:
    [[nodiscard]] Domain::Scene                  hydrateScene(QSqlQuery& q)           const;
    [[nodiscard]] std::vector<Domain::PlaceId>   loadPlaceIds(int64_t sceneId)        const;
    [[nodiscard]] std::vector<Domain::FactionId> loadFactionIds(int64_t sceneId)      const;
    [[nodiscard]] std::vector<Domain::CharacterId> loadCharacterIds(int64_t sceneId)  const;

    // Generic join table helper — avoids repeating link/unlink 6 times
    [[nodiscard]] Core::Result<void> insertJoin(const QString& table,
                                                 const QString& col1, qlonglong id1,
                                                 const QString& col2, qlonglong id2);
    [[nodiscard]] Core::Result<void> deleteJoin(const QString& table,
                                                 const QString& col1, qlonglong id1,
                                                 const QString& col2, qlonglong id2);

    DatabaseManager& m_db;
    Core::Logger&    m_logger;
};

} // namespace CF::Infra