#include "ChapterViewModel.h"
#include "core/Assert.h"

#include <QTimer>
#include <QMimeData>

namespace CF::UI {

using namespace CF::Domain;
using namespace CF::Services;
using namespace CF::Core;

ChapterViewModel::ChapterViewModel(
    std::shared_ptr<ChapterService>      chapterService,
    std::shared_ptr<AutoCompleteService> autoCompleteService,
    Core::Logger& logger,
    QObject* parent)
    : QAbstractListModel(parent)
    , m_chapterService(std::move(chapterService))
    , m_autoCompleteService(std::move(autoCompleteService))
    , m_logger(logger)
{
    CF_REQUIRE(m_chapterService      != nullptr, "ChapterService must not be null");
    CF_REQUIRE(m_autoCompleteService != nullptr, "AutoCompleteService must not be null");

    // Autosave timer — single-shot, restarted on every content change
    m_autosaveTimer = new QTimer(this);
    m_autosaveTimer->setSingleShot(true);
    m_autosaveTimer->setInterval(AUTOSAVE_DELAY_MS);
    connect(m_autosaveTimer, &QTimer::timeout,
            this, &ChapterViewModel::onAutosaveTimer);

    m_chapterService->setOnChangeCallback(
        [this](DomainEvent event) {
            QMetaObject::invokeMethod(this, [this, event]() {
                onServiceChange(event);
            }, Qt::QueuedConnection);
        });
}

// ── QAbstractListModel ────────────────────────────────────────────────────────

int ChapterViewModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_chapters.size());
}

QVariant ChapterViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_chapters.size()))
        return {};

    const auto& c = m_chapters[index.row()];
    switch (role) {
        case Qt::DisplayRole:
        case TitleRole:     return QString::fromStdString(c.title);
        case IdRole:        return static_cast<qint64>(c.id.value());
        case SortOrderRole: return c.sortOrder;
        default:            return {};
    }
}

QHash<int, QByteArray> ChapterViewModel::roleNames() const
{
    return {
        { IdRole,        "chapterId"  },
        { TitleRole,     "title"      },
        { SortOrderRole, "sortOrder"  }
    };
}

Qt::ItemFlags ChapterViewModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    if (index.isValid())
        return defaultFlags | Qt::ItemIsDragEnabled;
    return defaultFlags | Qt::ItemIsDropEnabled;
}

Qt::DropActions ChapterViewModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool ChapterViewModel::moveRows(
    const QModelIndex&, int sourceRow, int count,
    const QModelIndex&, int destinationChild)
{
    if (sourceRow < 0 || sourceRow + count > static_cast<int>(m_chapters.size()))
        return false;

    beginMoveRows(QModelIndex(), sourceRow, sourceRow + count - 1,
                  QModelIndex(), destinationChild);

    // Move in local cache
    auto it = m_chapters.begin() + sourceRow;
    std::vector<Chapter> moved(it, it + count);
    m_chapters.erase(it, it + count);
    int insertAt = destinationChild > sourceRow
                 ? destinationChild - count : destinationChild;
    m_chapters.insert(m_chapters.begin() + insertAt,
                      moved.begin(), moved.end());

    endMoveRows();

    // Build new ID order and persist
    std::vector<ChapterId> orderedIds;
    orderedIds.reserve(m_chapters.size());
    for (const auto& c : m_chapters) orderedIds.push_back(c.id);

    auto result = m_chapterService->reorder(orderedIds);
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
    }
    return true;
}

// ── Commands ──────────────────────────────────────────────────────────────────

void ChapterViewModel::loadAll()
{
    auto result = m_chapterService->getAllOrdered();
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    beginResetModel();
    m_chapters = std::move(result.value());
    endResetModel();
}

void ChapterViewModel::createChapter(const QString& title)
{
    auto result = m_chapterService->createChapter(title.toStdString());
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    const int newRow = static_cast<int>(m_chapters.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_chapters.push_back(result.value());
    endInsertRows();
}

void ChapterViewModel::deleteChapter(ChapterId id)
{
    auto result = m_chapterService->deleteChapter(id);
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    const int row = indexOfId(id);
    if (row >= 0) {
        beginRemoveRows(QModelIndex(), row, row);
        m_chapters.erase(m_chapters.begin() + row);
        endRemoveRows();

        // Clear active if it was this chapter
        if (m_activeRow == row) {
            m_activeRow.reset();
        }
    }
}

void ChapterViewModel::renameChapter(ChapterId id, const QString& newTitle)
{
    const int row = indexOfId(id);
    if (row < 0) return;

    Chapter updated = m_chapters[row];
    updated.title   = newTitle.toStdString();

    auto result = m_chapterService->updateChapter(updated);
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    m_chapters[row] = result.value();
    const auto idx  = index(row);
    emit dataChanged(idx, idx, { TitleRole });
}

void ChapterViewModel::onContentChanged(const QString& markdown)
{
    m_pendingContent = markdown;
    m_contentDirty   = true;
    m_autosaveTimer->start();  // Restart — debounce
}

void ChapterViewModel::flushContent()
{
    if (!m_contentDirty || !m_activeRow.has_value()) return;
    m_autosaveTimer->stop();
    onAutosaveTimer();
}

// ── Active chapter ────────────────────────────────────────────────────────────

void ChapterViewModel::setActiveChapter(int row)
{
    if (row < 0 || row >= static_cast<int>(m_chapters.size())) return;

    // Flush pending content for the previous chapter before switching
    flushContent();

    m_activeRow      = row;
    m_contentDirty   = false;
    m_pendingContent.clear();

    emit activeChapterChanged(m_chapters[row]);
}

std::optional<Domain::Chapter> ChapterViewModel::activeChapter() const
{
    if (!m_activeRow.has_value()) return std::nullopt;
    return m_chapters[*m_activeRow];
}

std::optional<int> ChapterViewModel::activeRow() const
{
    return m_activeRow;
}

// ── Autocomplete ──────────────────────────────────────────────────────────────

Core::Result<std::vector<AutoCompleteSuggestion>>
ChapterViewModel::getSuggestions(const QString& fragment) const
{
    return m_autoCompleteService->suggest(fragment.toStdString());
}

// ── Private ───────────────────────────────────────────────────────────────────

void ChapterViewModel::onAutosaveTimer()
{
    if (!m_contentDirty || !m_activeRow.has_value()) return;

    const auto& chapter = m_chapters[*m_activeRow];
    auto result = m_chapterService->saveContent(
        chapter.id, m_pendingContent.toStdString());

    if (result.isErr()) {
        emit errorOccurred("Autosave failed: "
            + QString::fromStdString(result.error().message));
        return;
    }

    // Update local cache
    m_chapters[*m_activeRow].markdownContent = m_pendingContent.toStdString();
    m_contentDirty = false;

    emit contentSaved(chapter.id);
    m_logger.debug("Chapter autosaved: " + chapter.title, "ChapterViewModel");
}

void ChapterViewModel::onServiceChange(DomainEvent event)
{
    using Kind = DomainEvent::Kind;
    if (event.kind == Kind::EntityDeleted) return;  // Handled explicitly

    // Reload if a chapter we don't have was created externally
    if (event.kind == Kind::EntityCreated) {
        const int row = indexOfId(ChapterId(event.entityId));
        if (row < 0) loadAll();
    }
}

int ChapterViewModel::indexOfId(ChapterId id) const
{
    for (int i = 0; i < static_cast<int>(m_chapters.size()); ++i) {
        if (m_chapters[i].id == id) return i;
    }
    return -1;
}

} // namespace CF::UI