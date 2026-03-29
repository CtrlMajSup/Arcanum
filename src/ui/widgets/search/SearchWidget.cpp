#include "SearchWidget.h"
#include "core/Assert.h"

#include <QPainter>
#include <QStyleOptionViewItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QApplication>
#include <QRegularExpression>

namespace CF::UI {

using namespace CF::Repositories;
using namespace CF::Core;

// ═══════════════════════════════════════════════════════════════════════════════
// SearchResultDelegate
// ═══════════════════════════════════════════════════════════════════════════════

SearchResultDelegate::SearchResultDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{}

void SearchResultDelegate::paint(
    QPainter* painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index) const
{
    // Group headers: use default style
    if (index.internalId() == 0) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Background
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered  = option.state & QStyle::State_MouseOver;
    if (selected) {
        painter->fillRect(option.rect, QColor("#313244"));
    } else if (hovered) {
        painter->fillRect(option.rect, QColor("#1e1e2e"));
    }

    const QRect r = option.rect.adjusted(8, 4, -8, -4);

    // Line 1: entity name (bold)
    const QString name = index.data(Qt::DisplayRole).toString();
    QFont nameFont = option.font;
    nameFont.setPixelSize(13);
    nameFont.setBold(false);
    painter->setFont(nameFont);
    painter->setPen(selected ? QColor("#cdd6f4") : QColor("#cdd6f4"));
    painter->drawText(r.adjusted(0, 0, 0, -r.height() / 2),
                      Qt::AlignVCenter | Qt::AlignLeft,
                      name);

    // Line 2: snippet (smaller, dim, highlight [match] brackets)
    QString snippet = index.data(SearchViewModel::SnippetRole).toString();

    // Convert FTS5 snippet markers [word] to styled text
    // We render it as plain text here; rich text would need QTextDocument
    snippet.replace(QRegularExpression(R"(\[([^\]]+)\])"), "«\\1»");

    QFont snippetFont = option.font;
    snippetFont.setPixelSize(11);
    painter->setFont(snippetFont);
    painter->setPen(QColor("#6c7086"));

    const QFontMetrics fm(snippetFont);
    const QString elided = fm.elidedText(snippet, Qt::ElideRight, r.width());
    painter->drawText(r.adjusted(0, r.height() / 2, 0, 0),
                      Qt::AlignVCenter | Qt::AlignLeft,
                      elided);

    painter->restore();
}

QSize SearchResultDelegate::sizeHint(
    const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.internalId() == 0)
        return QSize(option.rect.width(), 28); // Group header height
    return QSize(option.rect.width(), 52);     // Two-line result height
}

// ═══════════════════════════════════════════════════════════════════════════════
// SearchWidget
// ═══════════════════════════════════════════════════════════════════════════════

SearchWidget::SearchWidget(
    std::shared_ptr<SearchViewModel> viewModel, QWidget* parent)
    : QWidget(parent)
    , m_viewModel(std::move(viewModel))
{
    CF_REQUIRE(m_viewModel != nullptr, "SearchViewModel must not be null");
    setupUi();
    setupConnections();
}

void SearchWidget::activate()
{
    m_searchBox->setFocus();
    if (!m_viewModel->lastQuery().isEmpty()) {
        m_searchBox->selectAll();
    }
}

void SearchWidget::searchFor(const QString& query)
{
    m_searchBox->setText(query);
    m_viewModel->search(query);
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void SearchWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Search bar ────────────────────────────────────────────────────────────
    auto* searchBar = new QWidget(this);
    searchBar->setObjectName("searchBar");
    searchBar->setFixedHeight(56);
    auto* sl = new QHBoxLayout(searchBar);
    sl->setContentsMargins(16, 8, 16, 8);
    sl->setSpacing(8);

    auto* icon = new QLabel("🔍", searchBar);
    icon->setFixedWidth(20);

    m_searchBox = new QLineEdit(searchBar);
    m_searchBox->setObjectName("searchInput");
    m_searchBox->setPlaceholderText(
        "Search characters, places, factions, chapters…");
    m_searchBox->setClearButtonEnabled(true);

    QFont sf = m_searchBox->font();
    sf.setPixelSize(15);
    m_searchBox->setFont(sf);

    sl->addWidget(icon);
    sl->addWidget(m_searchBox, 1);
    root->addWidget(searchBar);

    // ── Status label ──────────────────────────────────────────────────────────
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("countLabel");
    m_statusLabel->setContentsMargins(16, 4, 16, 4);
    root->addWidget(m_statusLabel);

    // ── Results tree ──────────────────────────────────────────────────────────
    m_resultsTree = new QTreeView(this);
    m_resultsTree->setModel(m_viewModel.get());
    m_resultsTree->setItemDelegate(new SearchResultDelegate(m_resultsTree));
    m_resultsTree->setObjectName("searchResultsTree");
    m_resultsTree->setHeaderHidden(true);
    m_resultsTree->setRootIsDecorated(true);
    m_resultsTree->setAnimated(true);
    m_resultsTree->setIndentation(16);
    m_resultsTree->setExpandsOnDoubleClick(false);
    m_resultsTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultsTree->setUniformRowHeights(false);

    // Extra styling
    m_resultsTree->setStyleSheet(R"(
        QTreeView {
            background: #1e1e2e;
            border: none;
            color: #cdd6f4;
            outline: none;
        }
        QTreeView::item:selected {
            background: #313244;
        }
        QTreeView::item:hover {
            background: #24273a;
        }
        QTreeView::branch {
            background: #1e1e2e;
        }
        QTreeView::branch:has-children:!has-siblings:closed,
        QTreeView::branch:closed:has-children:has-siblings {
            image: url(:/icons/arrow-right.png);
        }
        QTreeView::branch:open:has-children:!has-siblings,
        QTreeView::branch:open:has-children:has-siblings {
            image: url(:/icons/arrow-down.png);
        }
    )");

    root->addWidget(m_resultsTree, 1);

    // Empty state label
    auto* emptyLabel = new QLabel(
        "Type to search across your entire world…", this);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setObjectName("placeholderLabel");
    root->addWidget(emptyLabel);

    // Show empty label when no results, hide when results exist
    connect(m_viewModel.get(), &SearchViewModel::searchCompleted,
            this, [emptyLabel, this](int total) {
                emptyLabel->setVisible(total == 0);
                m_resultsTree->setVisible(total > 0);
            });

    m_resultsTree->setVisible(false);

    // Debounce timer
    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(SEARCH_DELAY_MS);
}

void SearchWidget::setupConnections()
{
    // Debounce: restart timer on every keystroke
    connect(m_searchBox, &QLineEdit::textChanged,
            m_searchTimer, qOverload<>(&QTimer::start));

    connect(m_searchTimer, &QTimer::timeout, this, [this]() {
        onSearchTextChanged(m_searchBox->text());
    });

    // Clear search when box is cleared
    connect(m_searchBox, &QLineEdit::textChanged, this, [this](const QString& t) {
        if (t.isEmpty()) m_viewModel->clear();
    });

    connect(m_viewModel.get(), &SearchViewModel::searchCompleted,
            this, &SearchWidget::onSearchCompleted);

    connect(m_viewModel.get(), &SearchViewModel::errorOccurred,
            this, &SearchWidget::onError);

    // Single click on a result navigates immediately
    connect(m_resultsTree, &QTreeView::clicked,
            this, &SearchWidget::onResultActivated);

    // Auto-expand group nodes when search completes
    connect(m_viewModel.get(), &SearchViewModel::searchCompleted,
            this, [this](int total) {
                if (total > 0) m_resultsTree->expandAll();
            });
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void SearchWidget::onSearchTextChanged(const QString& text)
{
    if (text.trimmed().isEmpty()) {
        m_viewModel->clear();
        return;
    }
    m_viewModel->search(text.trimmed());
}

void SearchWidget::onSearchCompleted(int total)
{
    updateStatusLabel(total, m_viewModel->lastQuery());
}

void SearchWidget::onResultActivated(const QModelIndex& index)
{
    const auto result = m_viewModel->resultForIndex(index);
    if (!result.has_value()) {
        // Group node — toggle expand/collapse
        if (m_resultsTree->isExpanded(index))
            m_resultsTree->collapse(index);
        else
            m_resultsTree->expand(index);
        return;
    }

    const qint64 id = static_cast<qint64>(result->id);

    switch (result->type) {
        case SearchResult::EntityType::Character:
            emit navigateToCharacter(id);
            break;
        case SearchResult::EntityType::Place:
            emit navigateToPlace(id);
            break;
        case SearchResult::EntityType::Faction:
            emit navigateToFaction(id);
            break;
        case SearchResult::EntityType::Chapter:
            emit navigateToChapter(id);
            break;
    }
}

void SearchWidget::onError(const QString& message)
{
    m_statusLabel->setText("⚠ " + message);
}

void SearchWidget::updateStatusLabel(int total, const QString& query)
{
    if (query.isEmpty()) {
        m_statusLabel->clear();
        return;
    }
    if (total == 0) {
        m_statusLabel->setText(
            QString("No results for \"%1\"").arg(query));
    } else {
        m_statusLabel->setText(
            QString("%1 result%2 for \"%3\"")
            .arg(total)
            .arg(total == 1 ? "" : "s")
            .arg(query));
    }
}

} // namespace CF::UI