#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <memory>
#include <vector>

#include "services/SearchService.h"
#include "repositories/ISearchRepository.h"
#include "core/Logger.h"

namespace CF::UI {

/**
 * SearchViewModel wraps SearchService results into a grouped
 * QAbstractItemModel suitable for a QTreeView.
 *
 * Tree structure:
 *   Root
 *   ├── Characters (N)
 *   │     ├── ResultItem
 *   │     └── ResultItem
 *   ├── Places (N)
 *   ├── Factions (N)
 *   └── Chapters (N)
 *
 * Group nodes are non-selectable headers.
 * Leaf nodes carry the full SearchResult payload.
 */
class SearchViewModel : public QAbstractItemModel {
    Q_OBJECT

public:
    // Extra roles beyond Qt::DisplayRole
    enum Roles {
        EntityIdRole     = Qt::UserRole + 1,
        EntityTypeRole,
        SnippetRole,
        ScoreRole
    };

    explicit SearchViewModel(
        std::shared_ptr<Services::SearchService> searchService,
        Core::Logger& logger,
        QObject* parent = nullptr);

    // ── QAbstractItemModel ────────────────────────────────────────────────────
    QModelIndex   index(int row, int col,
                        const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex   parent(const QModelIndex& index)                  const override;
    int           rowCount(const QModelIndex& parent = QModelIndex())const override;
    int           columnCount(const QModelIndex& parent = QModelIndex())const override;
    QVariant      data(const QModelIndex& index,
                       int role = Qt::DisplayRole)                  const override;
    Qt::ItemFlags flags(const QModelIndex& index)                   const override;
    QHash<int, QByteArray> roleNames()                              const override;

    // ── Commands ──────────────────────────────────────────────────────────────

    // Runs a search and rebuilds the model
    void search(const QString& query);

    // Clears all results
    void clear();

    [[nodiscard]] bool hasResults()    const;
    [[nodiscard]] int  totalResults()  const;
    [[nodiscard]] QString lastQuery()  const;

    // Returns the SearchResult for a leaf index, or nullopt for group nodes
    [[nodiscard]] std::optional<Repositories::SearchResult>
    resultForIndex(const QModelIndex& index) const;

signals:
    void searchCompleted(int totalResults);
    void errorOccurred(const QString& message);

private:
    // Internal tree nodes
    struct GroupNode {
        QString                               label;
        Repositories::SearchResult::EntityType type;
        std::vector<Repositories::SearchResult> results;
    };

    void rebuildModel(
        const std::vector<Repositories::SearchResult>& results);

    std::shared_ptr<Services::SearchService> m_searchService;
    Core::Logger& m_logger;

    std::vector<GroupNode> m_groups;
    QString                m_lastQuery;

    // Stable group order: Characters → Places → Factions → Chapters
    static constexpr int GROUP_COUNT = 4;
    static const Repositories::SearchResult::EntityType GROUP_TYPES[GROUP_COUNT];
    static const char* GROUP_LABELS[GROUP_COUNT];
};

} // namespace CF::UI