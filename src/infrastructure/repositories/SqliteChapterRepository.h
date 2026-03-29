#pragma once

#include "repositories/IChapterRepository.h"
#include "infrastructure/database/DatabaseManager.h"
#include "core/Logger.h"

namespace CF::Infra {

class SqliteChapterRepository final : public Repositories::IChapterRepository {
public:
    explicit SqliteChapterRepository(DatabaseManager& db, Core::Logger& logger);

    // ── IRepository ──────────────────────────────────────────────────────────
    Core::Result<Domain::Chapter>              findById(Domain::ChapterId id) const override;
    Core::Result<std::vector<Domain::Chapter>> findAll()                      const override;
    Core::Result<Domain::Chapter>              save(const Domain::Chapter& c)       override;
    Core::Result<Domain::Chapter>              update(const Domain::Chapter& c)     override;
    Core::Result<void>                         remove(Domain::ChapterId id)         override;
    bool                                       exists(Domain::ChapterId id)   const override;

    // ── IChapterRepository ───────────────────────────────────────────────────
    Core::Result<std::vector<Domain::Chapter>> findByNameContaining(const std::string& f) const override;
    Core::Result<std::vector<Domain::Chapter>> findAllOrdered()                           const override;
    Core::Result<void> updateContent(Domain::ChapterId, const std::string& markdown)            override;
    Core::Result<void> updateSortOrder(Domain::ChapterId, int32_t sortOrder)                    override;

private:
    [[nodiscard]] Domain::Chapter hydrateChapter(QSqlQuery& q) const;

    DatabaseManager& m_db;
    Core::Logger&    m_logger;
};

} // namespace CF::Infra