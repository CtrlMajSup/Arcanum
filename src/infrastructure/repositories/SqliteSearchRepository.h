#pragma once

#include "repositories/ISearchRepository.h"
#include "infrastructure/database/DatabaseManager.h"
#include "core/Logger.h"

namespace CF::Infra {

class SqliteSearchRepository final : public Repositories::ISearchRepository {
public:
    explicit SqliteSearchRepository(DatabaseManager& db, Core::Logger& logger);

    Core::Result<std::vector<Repositories::SearchResult>>
    searchAll(const std::string& query, int maxResults = 50) const override;

    Core::Result<std::vector<Repositories::SearchResult>>
    searchByType(const std::string& query,
                 Repositories::SearchResult::EntityType type,
                 int maxResults = 20) const override;

    Core::Result<void> rebuildIndex() override;

private:
    DatabaseManager& m_db;
    Core::Logger&    m_logger;
};

} // namespace CF::Infra