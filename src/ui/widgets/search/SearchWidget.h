#pragma once

#include <QWidget>
#include <QTreeView>
#include <QLineEdit>
#include <QLabel>
#include <QTimer>
#include <QStyledItemDelegate>
#include <memory>

#include "ui/viewmodels/SearchViewModel.h"
#include "repositories/ISearchRepository.h"

namespace CF::UI {

/**
 * SearchResultDelegate paints each result row with a two-line layout:
 *   Line 1 (bold):  entity name
 *   Line 2 (dim):   FTS5 snippet with [match] highlighted
 */
class SearchResultDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit SearchResultDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
};


/**
 * SearchWidget is the global search panel.
 *
 * Layout:
 * ┌──────────────────────────────────────┐
 * │  🔍 [ search box                  ] │
 * │  Showing N results for "query"       │
 * ├──────────────────────────────────────┤
 * │  ▶ Characters (3)                   │
 * │      Kael Doran                      │
 * │        ...found in [biography]...    │
 * │  ▶ Places (1)                       │
 * │      The Iron Fortress               │
 * │        ...located in [region]...     │
 * └──────────────────────────────────────┘
 *
 * Clicking a result emits a typed navigation signal.
 * The MainWindow receives it and switches to the right panel.
 */
class SearchWidget : public QWidget {
    Q_OBJECT

public:
    explicit SearchWidget(
        std::shared_ptr<SearchViewModel> viewModel,
        QWidget* parent = nullptr);

    void activate();
    void searchFor(const QString& query);  // Called by other panels

signals:
    // Emitted when user clicks a result — MainWindow navigates to it
    void navigateToCharacter(qint64 id);
    void navigateToPlace(qint64 id);
    void navigateToFaction(qint64 id);
    void navigateToChapter(qint64 id);

private slots:
    void onSearchTextChanged(const QString& text);
    void onSearchCompleted(int total);
    void onResultActivated(const QModelIndex& index);
    void onError(const QString& message);

private:
    void setupUi();
    void setupConnections();
    void updateStatusLabel(int total, const QString& query);

    std::shared_ptr<SearchViewModel> m_viewModel;

    QLineEdit*           m_searchBox    = nullptr;
    QTreeView*           m_resultsTree  = nullptr;
    QLabel*              m_statusLabel  = nullptr;
    QTimer*              m_searchTimer  = nullptr;

    static constexpr int SEARCH_DELAY_MS = 250;
};

} // namespace CF::UI