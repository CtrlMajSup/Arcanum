#include "SearchService.h"
#include "core/Assert.h"

namespace CF::Services {

using namespace CF::Core;
using namespace CF::Repositories;

SearchService::SearchService(
    std::shared_ptr<ISearchRepository> searchRepo,
    Core::Logger& logger)
    : m_searchRepo(std::move(searchRepo))
    , m_logger(logger)
{
    CF_REQUIRE(m_searchRepo != nullptr, "SearchRepository must not be null");
}

Result<std::vector<SearchResult>>
SearchService::searchAll(const std::string& query, int maxResults) const
{
    if (query.empty()) return Result<std::vector<SearchResult>>::ok({});

    // Sanitise: FTS5 treats '*' as a prefix operator — allow it intentionally
    m_logger.debug("Search query: " + query, "SearchService");
    return m_searchRepo->searchAll(query, maxResults);
}

Result<std::vector<SearchResult>>
SearchService::searchCharacters(const std::string& query, int maxResults) const
{
    if (query.empty()) return Result<std::vector<SearchResult>>::ok({});
    return m_searchRepo->searchByType(query, SearchResult::EntityType::Character, maxResults);
}

Result<std::vector<SearchResult>>
SearchService::searchPlaces(const std::string& query, int maxResults) const
{
    if (query.empty()) return Result<std::vector<SearchResult>>::ok({});
    return m_searchRepo->searchByType(query, SearchResult::EntityType::Place, maxResults);
}

Result<std::vector<SearchResult>>
SearchService::searchFactions(const std::string& query, int maxResults) const
{
    if (query.empty()) return Result<std::vector<SearchResult>>::ok({});
    return m_searchRepo->searchByType(query, SearchResult::EntityType::Faction, maxResults);
}

Result<std::vector<SearchResult>>
SearchService::searchChapters(const std::string& query, int maxResults) const
{
    if (query.empty()) return Result<std::vector<SearchResult>>::ok({});
    return m_searchRepo->searchByType(query, SearchResult::EntityType::Chapter, maxResults);
}

} // namespace CF::Services