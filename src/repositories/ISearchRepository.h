#pragma once

#include "core/Result.h"

#include <string>
#include <vector>

namespace CF::Repositories {

/**
 * SearchResult is a lightweight descriptor returned by cross-entity search.
 * It avoids loading full domain objects just for a search results list.
 */
struct SearchResult {
    enum class EntityType {
        Character, Place, Faction, Scene, Chapter
    };

    int64_t    id;
    EntityType type;
    std::string name;       // Display name
    std::string snippet;    // Short context excerpt
    double      score;      // Relevance (0.0–1.0), used for sorting
};

/**
 * ISearchRepository performs full-text search across all entity tables.
 * The SQLite implementation uses FTS5 virtual tables for performance.
 */
class ISearchRepository {
public:
    virtual ~ISearchRepository() = default;

    // Searches name + description fields across all entity types
    [[nodiscard]] virtual Core::Result<std::vector<SearchResult>>
    searchAll(const std::string& query, int maxResults = 50) const = 0;

    // Scoped search — only returns results of specific types
    [[nodiscard]] virtual Core::Result<std::vector<SearchResult>>
    searchByType(const std::string& query,
                 SearchResult::EntityType type,
                 int maxResults = 20) const = 0;

    // Rebuilds the FTS index — call after bulk imports
    [[nodiscard]] virtual Core::Result<void>
    rebuildIndex() = 0;
};

} // namespace CF::Repositories