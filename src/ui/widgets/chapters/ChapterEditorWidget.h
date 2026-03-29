#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QLabel>
#include <QToolButton>
#include <QTimer>
#include <QTextCursor>
#include <memory>
#include <optional>

#include "ui/viewmodels/ChapterViewModel.h"
#include "AutoCompletePopup.h"

namespace CF::UI {

/**
 * MarkdownEditor is a QPlainTextEdit that:
 *  1. Detects '@' and triggers autocomplete
 *  2. Applies minimal syntax highlighting to Markdown
 *  3. Reports content changes to the ViewModel
 *
 * It intercepts arrow keys and Enter/Tab when the popup is visible
 * to route them into the autocomplete list.
 */
class MarkdownEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit MarkdownEditor(QWidget* parent = nullptr);

    void setAutoCompletePopup(AutoCompletePopup* popup);
    void setViewModel(ChapterViewModel* vm);

signals:
    void contentChanged(const QString& text);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onTextChanged();
    void onCursorPositionChanged();

private:
    void triggerAutoComplete(const QString& fragment);
    void hideAutoComplete();
    void insertCompletion(const QString& insertionText);

    // Returns the '@fragment' being typed if cursor is in an @-word,
    // or empty string if not
    [[nodiscard]] QString currentAtFragment() const;

    // Returns the pixel position of the '@' character in the editor
    [[nodiscard]] QPoint atCharPosition() const;

    AutoCompletePopup* m_popup    = nullptr;
    ChapterViewModel*  m_vm       = nullptr;
    bool m_suppressTextChanged    = false;
};


/**
 * ChapterEditorWidget contains:
 *  - Left gutter:  word count, autosave indicator, chapter metadata
 *  - Centre:       MarkdownEditor (raw text input)
 *  - Right:        Live HTML preview (QTextEdit with rendered Markdown)
 *
 * The preview is updated 500ms after the last keystroke to avoid
 * blocking the editor on every character.
 */
class ChapterEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChapterEditorWidget(
        std::shared_ptr<ChapterViewModel> viewModel,
        QWidget* parent = nullptr);

    // Called when a chapter is selected in the list
    void loadChapter(const Domain::Chapter& chapter);

    // Called on panel hide/switch
    void flushContent();

signals:
    void chapterModified(Domain::ChapterId id);

private slots:
    void onContentChanged(const QString& text);
    void onPreviewTimer();
    void onSaveIndicatorTimer();
    void onContentSaved(Domain::ChapterId id);

private:
    void setupUi();
    void setupConnections();
    void updateWordCount(const QString& text);
    void updatePreview(const QString& markdown);

    // Converts @[Name|type:id] references to styled HTML spans
    [[nodiscard]] static QString renderMarkdownToHtml(const QString& markdown);
    [[nodiscard]] static QString highlightEntityRefs(const QString& html);

    std::shared_ptr<ChapterViewModel> m_viewModel;
    std::optional<Domain::Chapter>    m_currentChapter;

    // ── Widgets ───────────────────────────────────────────────────────────────
    MarkdownEditor*  m_editor       = nullptr;
    QTextEdit*       m_preview      = nullptr;
    QLabel*          m_wordCount    = nullptr;
    QLabel*          m_saveStatus   = nullptr;
    QLabel*          m_chapterTitle = nullptr;
    AutoCompletePopup* m_popup      = nullptr;

    // ── Timers ────────────────────────────────────────────────────────────────
    QTimer* m_previewTimer     = nullptr;  // Debounce preview refresh
    QTimer* m_saveIndicatorTimer = nullptr; // "Saved" label fade-out

    static constexpr int PREVIEW_DELAY_MS     = 500;
    static constexpr int SAVE_INDICATOR_MS    = 2000;
};

} // namespace CF::UI