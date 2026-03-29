#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>
#include <vector>
#include <optional>

#include "services/ChapterService.h"
#include "services/AutoCompleteService.h"
#include "domain/entities/Chapter.h"
#include "core/Logger.h"

namespace CF::UI {

/**
 * ChapterViewModel exposes the chapter list as a QAbstractListModel
 * and manages the currently active chapter for the editor.
 *
 * It also owns the autosave timer logic — the editor reports content
 * changes here, and this ViewModel decides when to flush to the DB.
 */
class ChapterViewModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        IdRole       = Qt::UserRole + 1,
        TitleRole,
        SortOrderRole
    };

    explicit ChapterViewModel(
        std::shared_ptr<Services::ChapterService>      chapterService,
        std::shared_ptr<Services::AutoCompleteService> autoCompleteService,
        Core::Logger& logger,
        QObject* parent = nullptr);

    // ── QAbstractListModel ────────────────────────────────────────────────────
    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Drag-reorder support
    Qt::ItemFlags         flags(const QModelIndex& index) const override;
    Qt::DropActions       supportedDropActions() const override;
    bool                  moveRows(const QModelIndex& sourceParent,
                                   int sourceRow, int count,
                                   const QModelIndex& destinationParent,
                                   int destinationChild) override;

    // ── Commands ──────────────────────────────────────────────────────────────
    void loadAll();

    void createChapter(const QString& title);
    void deleteChapter(Domain::ChapterId id);
    void renameChapter(Domain::ChapterId id, const QString& newTitle);

    // Called by the editor widget on every keystroke — starts autosave timer
    void onContentChanged(const QString& markdown);

    // Force-flush content immediately (e.g. on tab switch)
    void flushContent();

    // ── Active chapter ────────────────────────────────────────────────────────
    void setActiveChapter(int row);
    [[nodiscard]] std::optional<Domain::Chapter> activeChapter() const;
    [[nodiscard]] std::optional<int>             activeRow()     const;

    // ── Autocomplete ──────────────────────────────────────────────────────────
    // Returns suggestions for the fragment typed after '@'
    [[nodiscard]] Core::Result<std::vector<Services::AutoCompleteSuggestion>>
    getSuggestions(const QString& fragment) const;

signals:
    void activeChapterChanged(const Domain::Chapter& chapter);
    void contentSaved(Domain::ChapterId id);
    void errorOccurred(const QString& message);

private slots:
    void onAutosaveTimer();

private:
    void onServiceChange(Domain::DomainEvent event);
    [[nodiscard]] int indexOfId(Domain::ChapterId id) const;

    std::shared_ptr<Services::ChapterService>      m_chapterService;
    std::shared_ptr<Services::AutoCompleteService> m_autoCompleteService;
    Core::Logger& m_logger;

    std::vector<Domain::Chapter> m_chapters;
    std::optional<int>           m_activeRow;

    // Autosave state
    QTimer*  m_autosaveTimer  = nullptr;
    QString  m_pendingContent;
    bool     m_contentDirty   = false;

    static constexpr int AUTOSAVE_DELAY_MS = 1500;
};

} // namespace CF::UI