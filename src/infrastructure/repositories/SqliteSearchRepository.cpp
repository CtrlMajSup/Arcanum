#include "SqliteSearchRepository.h"
#include "core/Assert.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace CF::Infra {

using namespace CF::Repositories;
using namespace CF::Core;

SqliteSearchRepository::SqliteSearchRepository(
    DatabaseManager& db, Core::Logger& logger)
    : m_db(db), m_logger(logger)
{}

Result<std::vector<SearchResult>>
SqliteSearchRepository::searchAll(const std::string& query, int maxResults) const
{
    // FTS5 UNION across all indexed tables, ranked by BM25 score
    // bm25() returns negative values — negate for ascending relevance sort
    auto q = m_db.makeQuery();
    q.prepare(R"(
        SELECT 'character' AS type, rowid AS id, name,
               snippet(fts_characters, 1, '[', ']', '...', 10) AS snippet,
               -bm25(fts_characters) AS score
        FROM fts_characters WHERE fts_characters MATCH ?

        UNION ALL

        SELECT 'place', rowid, name,
               snippet(fts_places, 1, '[', ']', '...', 10),
               -bm25(fts_places)
        FROM fts_places WHERE fts_places MATCH ?

        UNION ALL

        SELECT 'faction', rowid, name,
               snippet(fts_factions, 0, '[', ']', '...', 10),
               -bm25(fts_factions)
        FROM fts_factions WHERE fts_factions MATCH ?

        UNION ALL

        SELECT 'chapter', rowid, title,
               snippet(fts_chapters, 1, '[', ']', '...', 10),
               -bm25(fts_chapters)
        FROM fts_chapters WHERE fts_chapters MATCH ?

        ORDER BY score DESC LIMIT ?
    )");

    const QString qstr = QString::fromStdString(query);
    q.addBindValue(qstr);
    q.addBindValue(qstr);
    q.addBindValue(qstr);
    q.addBindValue(qstr);
    q.addBindValue(maxResults);

    std::vector<SearchResult> results;
    if (!q.exec()) {
        m_logger.warning("Search query failed: " + q.lastError().text().toStdString(),
                         "SearchRepo");
        return Result<std::vector<SearchResult>>::ok({});
    }

    while (q.next()) {
        SearchResult r;
        r.id      = q.value("id").toLongLong();
        r.name    = q.value("name").toString().toStdString();
        r.snippet = q.value("snippet").toString().toStdString();
        r.score   = q.value("score").toDouble();

        const auto type = q.value("type").toString();
        if      (type == "character") r.type = SearchResult::EntityType::Character;
        else if (type == "place")     r.type = SearchResult::EntityType::Place;
        else if (type == "faction")   r.type = SearchResult::EntityType::Faction;
        else                          r.type = SearchResult::EntityType::Chapter;

        results.push_back(std::move(r));
    }
    return Result<std::vector<SearchResult>>::ok(std::move(results));
}

Result<std::vector<SearchResult>>
SqliteSearchRepository::searchByType(const std::string& query,
                                      SearchResult::EntityType type,
                                      int maxResults) const
{
    // Select only the FTS table matching the requested type
    QString ftsTable, typeLabel, nameCol;
    switch (type) {
        case SearchResult::EntityType::Character:
            ftsTable = "fts_characters"; typeLabel = "character"; nameCol = "name"; break;
        case SearchResult::EntityType::Place:
            ftsTable = "fts_places";     typeLabel = "place";     nameCol = "name"; break;
        case SearchResult::EntityType::Faction:
            ftsTable = "fts_factions";   typeLabel = "faction";   nameCol = "name"; break;
        case SearchResult::EntityType::Chapter:
            ftsTable = "fts_chapters";   typeLabel = "chapter";   nameCol = "title"; break;
        default:
            return Result<std::vector<SearchResult>>::ok({});
    }

    auto q = m_db.makeQuery();
    const QString sql = QString(R"(
        SELECT '%1' AS type, rowid AS id, %2 AS name,
               snippet(%3, 0, '[', ']', '...', 10) AS snippet,
               -bm25(%4) AS score
        FROM %5 WHERE %6 MATCH ?
        ORDER BY score DESC LIMIT ?
    )").arg(typeLabel, nameCol, ftsTable, ftsTable, ftsTable, ftsTable);

    q.prepare(sql);
    q.addBindValue(QString::fromStdString(query));
    q.addBindValue(maxResults);

    std::vector<SearchResult> results;
    if (q.exec()) {
        while (q.next()) {
            SearchResult r;
            r.id      = q.value("id").toLongLong();
            r.name    = q.value("name").toString().toStdString();
            r.snippet = q.value("snippet").toString().toStdString();
            r.score   = q.value("score").toDouble();
            r.type    = type;
            results.push_back(std::move(r));
        }
    }
    return Result<std::vector<SearchResult>>::ok(std::move(results));
}

Result<void> SqliteSearchRepository::rebuildIndex()
{
    // Rebuild FTS indexes — needed after bulk data imports
    for (const auto& table : {"fts_characters", "fts_places",
                               "fts_factions",  "fts_chapters"}) {
        const auto result = m_db.execute(
            QString("INSERT INTO %1(%2) VALUES('rebuild')")
            .arg(table, table));
        if (result.isErr()) return result;
    }
    m_logger.info("FTS indexes rebuilt", "SearchRepo");
    return Result<void>::ok();
}

} // namespace CF::Infra