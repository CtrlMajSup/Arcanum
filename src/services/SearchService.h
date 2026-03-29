#pragma once

#include "core/Result.h"
#include "core/Logger.h"
#include "repositories/ISearchRepository.h"

#include <memory>
#include <string>
#include <vector>

namespace CF::Services {

class SearchService {
public:
    explicit SearchService(
        std::shared_ptr<Repositories::ISearchRepository> searchRepo,
        Core::Logger& logger);

    // Global search across all entity types
    [[nodiscard]] Core::Result<std::vector<Repositories::SearchResult>>
    searchAll(const std::string& query, int maxResults = 50) const;

    // Scoped search returning only one entity type
    [[nodiscard]] Core::Result<std::vector<Repositories::SearchResult>>
    searchCharacters(const std::string& query, int maxResults = 20) const;

    [[nodiscard]] Core::Result<std::vector<Repositories::SearchResult>>
    searchPlaces(const std::string& query, int maxResults = 20) const;

    [[nodiscard]] Core::Result<std::vector<Repositories::SearchResult>>
    searchFactions(const std::string& query, int maxResults = 20) const;

    [[nodiscard]] Core::Result<std::vector<Repositories::SearchResult>>
    searchChapters(const std::string& query, int maxResults = 20) const;

private:
    std::shared_ptr<Repositories::ISearchRepository> m_searchRepo;
    Core::Logger& m_logger;
};

} // namespace CF::Services