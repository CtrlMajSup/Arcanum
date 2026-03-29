#include "SearchViewModel.h"
#include "core/Assert.h"

#include <QColor>
#include <QFont>

namespace CF::UI {

using namespace CF::Repositories;
using namespace CF::Core;

// Static tables
const SearchResult::EntityType
SearchViewModel::GROUP_TYPES[GROUP_COUNT] = {
    SearchResult::EntityType::Character,
    SearchResult::EntityType::Place,
    SearchResult::EntityType::Faction,
    SearchResult::EntityType::Chapter
};

const char* SearchViewModel::GROUP_LABELS[GROUP_COUNT] = {
    "Characters", "Places", "Factions", "Chapters"
};

SearchViewModel::SearchViewModel(
    std::shared_ptr<Services::SearchService> searchService,
    Core::Logger& logger,
    QObject* parent)
    : QAbstractItemModel(parent)
    , m_searchService(std::move(searchService))
    , m_logger(logger)
{
    CF_REQUIRE(m_searchService != nullptr, "SearchService must not be null");

    // Pre-populate empty groups so the tree always has the right structure
    for (int i = 0; i < GROUP_COUNT; ++i) {
        m_groups.push_back({ GROUP_LABELS[i], GROUP_TYPES[i], {} });
    }
}

// ── QAbstractItemModel ────────────────────────────────────────────────────────

QModelIndex SearchViewModel::index(
    int row, int col, const QModelIndex& parent) const
{
    if (!hasIndex(row, col, parent)) return {};

    if (!parent.isValid()) {
        // Top-level: group nodes
        return createIndex(row, col,
                           reinterpret_cast<quintptr>(nullptr));
    }

    // Leaf: result item — encode group index in internalId
    const int groupRow = parent.row();
    return createIndex(row, col,
                       static_cast<quintptr>(groupRow + 1)); // +1: 0 = root
}

QModelIndex SearchViewModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) return {};

    const quintptr id = index.internalId();
    if (id == 0) return {};  // Group node — parent is root

    // Leaf node — parent is the group at (id - 1)
    const int groupRow = static_cast<int>(id) - 1;
    return createIndex(groupRow, 0, static_cast<quintptr>(0));
}

int SearchViewModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        // Root: number of groups that have results
        return static_cast<int>(m_groups.size());
    }

    // Only count children for group nodes (internalId == 0)
    if (parent.internalId() != 0) return 0;

    if (parent.row() < 0 || parent.row() >= GROUP_COUNT) return 0;
    return static_cast<int>(m_groups[parent.row()].results.size());
}

int SearchViewModel::columnCount(const QModelIndex&) const
{
    return 1;
}

QVariant SearchViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};

    const bool isGroup = (index.internalId() == 0);

    if (isGroup) {
        const auto& group = m_groups[index.row()];
        switch (role) {
            case Qt::DisplayRole:
                return QString("%1  (%2)")
                    .arg(group.label)
                    .arg(group.results.size());
            case Qt::ForegroundRole:
                return QColor("#a6adc8");
            case Qt::FontRole: {
                QFont f;
                f.setBold(true);
                f.setPixelSize(12);
                return f;
            }
            default: return {};
        }
    }

    // Leaf node
    const int groupIdx  = static_cast<int>(index.internalId()) - 1;
    if (groupIdx < 0 || groupIdx >= GROUP_COUNT) return {};

    const auto& results = m_groups[groupIdx].results;
    if (index.row() < 0 || index.row() >= static_cast<int>(results.size()))
        return {};

    const auto& result = results[index.row()];
    switch (role) {
        case Qt::DisplayRole:
            return QString::fromStdString(result.name);
        case SnippetRole:
            return QString::fromStdString(result.snippet);
        case EntityIdRole:
            return static_cast<qint64>(result.id);
        case EntityTypeRole:
            return static_cast<int>(result.type);
        case ScoreRole:
            return result.score;
        case Qt::ToolTipRole:
            return QString::fromStdString(result.snippet);
        case Qt::ForegroundRole:
            return QColor("#cdd6f4");
        default: return {};
    }
}

Qt::ItemFlags SearchViewModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;

    // Group nodes: not selectable
    if (index.internalId() == 0)
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> SearchViewModel::roleNames() const
{
    return {
        { EntityIdRole,   "entityId"   },
        { EntityTypeRole, "entityType" },
        { SnippetRole,    "snippet"    },
        { ScoreRole,      "score"      }
    };
}

// ── Commands ──────────────────────────────────────────────────────────────────

void SearchViewModel::search(const QString& query)
{
    if (query.trimmed().isEmpty()) {
        clear();
        return;
    }

    m_lastQuery = query;

    auto result = m_searchService->searchAll(query.toStdString(), 100);
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    beginResetModel();
    rebuildModel(result.value());
    endResetModel();

    const int total = totalResults();
    m_logger.debug("Search '" + query.toStdString() + "': "
                   + std::to_string(total) + " results", "SearchViewModel");
    emit searchCompleted(total);
}

void SearchViewModel::clear()
{
    beginResetModel();
    for (auto& group : m_groups) group.results.clear();
    m_lastQuery.clear();
    endResetModel();
    emit searchCompleted(0);
}

bool SearchViewModel::hasResults() const
{
    return totalResults() > 0;
}

int SearchViewModel::totalResults() const
{
    int total = 0;
    for (const auto& g : m_groups) total += static_cast<int>(g.results.size());
    return total;
}

QString SearchViewModel::lastQuery() const
{
    return m_lastQuery;
}

std::optional<SearchResult>
SearchViewModel::resultForIndex(const QModelIndex& index) const
{
    if (!index.isValid() || index.internalId() == 0) return std::nullopt;

    const int groupIdx = static_cast<int>(index.internalId()) - 1;
    if (groupIdx < 0 || groupIdx >= GROUP_COUNT) return std::nullopt;

    const auto& results = m_groups[groupIdx].results;
    if (index.row() < 0 || index.row() >= static_cast<int>(results.size()))
        return std::nullopt;

    return results[index.row()];
}

// ── Private ───────────────────────────────────────────────────────────────────

void SearchViewModel::rebuildModel(const std::vector<SearchResult>& results)
{
    // Clear all group results
    for (auto& group : m_groups) group.results.clear();

    // Distribute results into groups
    for (const auto& r : results) {
        for (auto& group : m_groups) {
            if (group.type == r.type) {
                group.results.push_back(r);
                break;
            }
        }
    }
}

} // namespace CF::UI