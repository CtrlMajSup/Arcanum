#include "ChapterEditorWidget.h"
#include "core/Assert.h"

#include <QKeyEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QRegularExpression>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QFontMetrics>

namespace CF::UI {

using namespace CF::Domain;
using namespace CF::Core;

// ═══════════════════════════════════════════════════════════════════════════════
// MarkdownEditor
// ═══════════════════════════════════════════════════════════════════════════════

MarkdownEditor::MarkdownEditor(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setObjectName("markdownEditor");

    // Monospace font for the raw editor
    QFont f("JetBrains Mono,Fira Code,Courier New,monospace");
    f.setPixelSize(14);
    f.setStyleHint(QFont::Monospace);
    setFont(f);

    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    setTabStopDistance(QFontMetrics(font()).horizontalAdvance(' ') * 4);
    setPlaceholderText("Start writing... Type @ to reference a character, place, or faction.");

    connect(this, &QPlainTextEdit::textChanged,
            this, &MarkdownEditor::onTextChanged);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &MarkdownEditor::onCursorPositionChanged);
}

void MarkdownEditor::setAutoCompletePopup(AutoCompletePopup* popup)
{
    m_popup = popup;
    if (m_popup) {
        m_popup->setAcceptCallback([this](const QString& text) {
            insertCompletion(text);
        });
    }
}

void MarkdownEditor::setViewModel(ChapterViewModel* vm)
{
    m_vm = vm;
}

void MarkdownEditor::keyPressEvent(QKeyEvent* event)
{
    // Route navigation keys to popup when it's visible
    if (m_popup && m_popup->isVisible()) {
        switch (event->key()) {
            case Qt::Key_Up:
                m_popup->navigateUp();
                event->accept();
                return;
            case Qt::Key_Down:
                m_popup->navigateDown();
                event->accept();
                return;
            case Qt::Key_Return:
            case Qt::Key_Tab:
                m_popup->acceptCurrent();
                event->accept();
                return;
            case Qt::Key_Escape:
                hideAutoComplete();
                event->accept();
                return;
            default:
                break;
        }
    }

    // Normal editing
    QPlainTextEdit::keyPressEvent(event);
}

void MarkdownEditor::onTextChanged()
{
    if (m_suppressTextChanged) return;
    emit contentChanged(toPlainText());
}

void MarkdownEditor::onCursorPositionChanged()
{
    if (!m_popup) return;

    const QString fragment = currentAtFragment();
    if (fragment.isEmpty()) {
        hideAutoComplete();
    } else {
        triggerAutoComplete(fragment);
    }
}

void MarkdownEditor::triggerAutoComplete(const QString& fragment)
{
    if (!m_vm || !m_popup) return;

    auto result = m_vm->getSuggestions(fragment);
    if (result.isErr() || result.value().empty()) {
        hideAutoComplete();
        return;
    }

    m_popup->setSuggestions(result.value());
    m_popup->move(viewport()->mapToGlobal(atCharPosition()));
    m_popup->show();
    m_popup->raise();
}

void MarkdownEditor::hideAutoComplete()
{
    if (m_popup) m_popup->hide();
}

void MarkdownEditor::insertCompletion(const QString& insertionText)
{
    // Replace the '@fragment' with the full insertion text
    QTextCursor cursor = textCursor();

    // Select back to the '@' character
    const QString text = toPlainText();
    int pos = cursor.position();
    while (pos > 0 && text[pos - 1] != '@') --pos;
    if (pos > 0) --pos;  // include the '@'

    cursor.setPosition(pos);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    // Extend selection to current cursor position
    cursor.setPosition(textCursor().position(), QTextCursor::KeepAnchor);

    m_suppressTextChanged = true;
    cursor.insertText(insertionText);
    m_suppressTextChanged = false;

    setTextCursor(cursor);
    emit contentChanged(toPlainText());
}

QString MarkdownEditor::currentAtFragment() const
{
    const QTextCursor cursor = textCursor();
    const QString     text   = toPlainText();
    int               pos    = cursor.position();

    // Walk back from cursor to find '@'
    int start = pos;
    while (start > 0) {
        const QChar c = text[start - 1];
        // Stop if we hit whitespace or a '[' (already completed ref)
        if (c.isSpace() || c == '[' || c == ']') break;
        if (c == '@') {
            // Found '@' — fragment is text between '@' and cursor
            return text.mid(start, pos - start);
        }
        --start;
    }
    return {};
}

QPoint MarkdownEditor::atCharPosition() const
{
    QTextCursor cursor = textCursor();

    // Find position of the '@' character
    const QString text = toPlainText();
    int pos = cursor.position();
    while (pos > 0 && text[pos - 1] != '@') --pos;
    if (pos > 0) --pos;

    QTextCursor atCursor = textCursor();
    atCursor.setPosition(pos);

    const QRect rect = cursorRect(atCursor);
    // Position popup below the '@' character
    return QPoint(rect.left(), rect.bottom() + 2);
}


// ═══════════════════════════════════════════════════════════════════════════════
// ChapterEditorWidget
// ═══════════════════════════════════════════════════════════════════════════════

ChapterEditorWidget::ChapterEditorWidget(
    std::shared_ptr<ChapterViewModel> viewModel, QWidget* parent)
    : QWidget(parent)
    , m_viewModel(std::move(viewModel))
{
    CF_REQUIRE(m_viewModel != nullptr, "ChapterViewModel must not be null");
    setupUi();
    setupConnections();
}

void ChapterEditorWidget::loadChapter(const Chapter& chapter)
{
    m_currentChapter = chapter;

    m_chapterTitle->setText(QString::fromStdString(chapter.title));

    // Block contentChanged signal while loading to avoid a spurious autosave
    m_editor->blockSignals(true);
    m_editor->setPlainText(QString::fromStdString(chapter.markdownContent));
    m_editor->blockSignals(false);

    updateWordCount(QString::fromStdString(chapter.markdownContent));
    updatePreview(QString::fromStdString(chapter.markdownContent));

    m_saveStatus->setText("Loaded");
}

void ChapterEditorWidget::flushContent()
{
    m_viewModel->flushContent();
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void ChapterEditorWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Header bar ────────────────────────────────────────────────────────────
    auto* header = new QWidget(this);
    header->setObjectName("editorHeader");
    header->setFixedHeight(40);
    auto* hl = new QHBoxLayout(header);
    hl->setContentsMargins(12, 0, 12, 0);

    m_chapterTitle = new QLabel("No chapter selected", header);
    m_chapterTitle->setObjectName("sectionTitle");

    m_saveStatus = new QLabel("", header);
    m_saveStatus->setObjectName("countLabel");

    m_wordCount = new QLabel("", header);
    m_wordCount->setObjectName("countLabel");

    // Preview toggle button
    auto* previewBtn = new QToolButton(header);
    previewBtn->setText("⊟ Preview");
    previewBtn->setCheckable(true);
    previewBtn->setChecked(true);
    previewBtn->setObjectName("timelineBtn");

    hl->addWidget(m_chapterTitle);
    hl->addStretch();
    hl->addWidget(m_wordCount);
    hl->addSpacing(12);
    hl->addWidget(m_saveStatus);
    hl->addSpacing(8);
    hl->addWidget(previewBtn);
    root->addWidget(header);

    // ── Editor + Preview splitter ─────────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName("editorSplitter");
    splitter->setHandleWidth(1);

    // Markdown editor
    m_editor = new MarkdownEditor(splitter);

    // Autocomplete popup — parented to editor, not to this widget
    m_popup  = new AutoCompletePopup(m_editor->viewport());
    m_popup->hide();
    m_editor->setAutoCompletePopup(m_popup);
    m_editor->setViewModel(m_viewModel.get());

    // Preview pane
    m_preview = new QTextEdit(splitter);
    m_preview->setObjectName("markdownPreview");
    m_preview->setReadOnly(true);
    m_preview->setAcceptRichText(true);

    // Custom preview stylesheet
    m_preview->document()->setDefaultStyleSheet(R"(
        body   { color: #cdd6f4; background: #1e1e2e;
                 font-family: Georgia, serif; font-size: 15px;
                 line-height: 1.7; margin: 0 16px; }
        h1,h2,h3 { color: #89b4fa; }
        h1     { font-size: 24px; border-bottom: 1px solid #313244; }
        h2     { font-size: 20px; }
        h3     { font-size: 16px; }
        strong { color: #f9e2af; }
        em     { color: #94e2d5; }
        code   { background: #313244; color: #a6e3a1;
                 padding: 1px 4px; border-radius: 3px;
                 font-family: monospace; }
        pre    { background: #181825; padding: 12px;
                 border-radius: 6px; overflow-x: auto; }
        blockquote { border-left: 3px solid #45475a;
                     margin-left: 0; padding-left: 12px;
                     color: #a6adc8; }
        a      { color: #89dceb; }
        .entity-character { color: #89b4fa;
                            background: rgba(137,180,250,0.12);
                            padding: 1px 4px; border-radius: 3px; }
        .entity-place     { color: #a6e3a1;
                            background: rgba(166,227,161,0.12);
                            padding: 1px 4px; border-radius: 3px; }
        .entity-faction   { color: #f38ba8;
                            background: rgba(243,139,168,0.12);
                            padding: 1px 4px; border-radius: 3px; }
        .entity-scene     { color: #fab387;
                            background: rgba(250,179,135,0.12);
                            padding: 1px 4px; border-radius: 3px; }
    )");

    splitter->addWidget(m_editor);
    splitter->addWidget(m_preview);
    splitter->setSizes({ 550, 450 });

    root->addWidget(splitter, 1);

    // Wire preview toggle
    connect(previewBtn, &QToolButton::toggled, m_preview, &QWidget::setVisible);

    // Timers
    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    m_previewTimer->setInterval(PREVIEW_DELAY_MS);

    m_saveIndicatorTimer = new QTimer(this);
    m_saveIndicatorTimer->setSingleShot(true);
    m_saveIndicatorTimer->setInterval(SAVE_INDICATOR_MS);
}

void ChapterEditorWidget::setupConnections()
{
    connect(m_editor, &MarkdownEditor::contentChanged,
            this, &ChapterEditorWidget::onContentChanged);

    connect(m_previewTimer, &QTimer::timeout,
            this, &ChapterEditorWidget::onPreviewTimer);

    connect(m_saveIndicatorTimer, &QTimer::timeout,
            this, [this]() { m_saveStatus->setText(""); });

    connect(m_viewModel.get(), &ChapterViewModel::contentSaved,
            this, &ChapterEditorWidget::onContentSaved);

    connect(m_saveIndicatorTimer, &QTimer::timeout,
            this, &ChapterEditorWidget::onSaveIndicatorTimer);

    connect(m_viewModel.get(), &ChapterViewModel::errorOccurred,
            this, [this](const QString& msg) {
                m_saveStatus->setText("⚠ " + msg);
            });

    connect(m_viewModel.get(), &ChapterViewModel::activeChapterChanged,
            this, &ChapterEditorWidget::loadChapter);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void ChapterEditorWidget::onContentChanged(const QString& text)
{
    if (!m_currentChapter.has_value()) return;

    m_viewModel->onContentChanged(text);
    updateWordCount(text);

    m_saveStatus->setText("Editing…");
    m_previewTimer->start();  // Debounce preview refresh
}

void ChapterEditorWidget::onPreviewTimer()
{
    if (!m_currentChapter.has_value()) return;
    updatePreview(m_editor->toPlainText());
}

void ChapterEditorWidget::onContentSaved(ChapterId id)
{
    if (m_currentChapter && m_currentChapter->id == id) {
        m_saveStatus->setText("✓ Saved");
        m_saveIndicatorTimer->start();
        emit chapterModified(id);
    }
}

void ChapterEditorWidget::onSaveIndicatorTimer()
{
    // Timer fired — the "✓ Saved" message has been visible long enough
    m_saveStatus->setText("");
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void ChapterEditorWidget::updateWordCount(const QString& text)
{
    // Count words: split on whitespace, filter empties
    const auto words = text.split(QRegularExpression("\\s+"),
                                  Qt::SkipEmptyParts);
    m_wordCount->setText(QString("%1 words").arg(words.size()));
}

void ChapterEditorWidget::updatePreview(const QString& markdown)
{
    const QString html = renderMarkdownToHtml(markdown);
    const int scrollPos = m_preview->verticalScrollBar()->value();
    m_preview->setHtml(html);
    m_preview->verticalScrollBar()->setValue(scrollPos);
}

QString ChapterEditorWidget::renderMarkdownToHtml(const QString& markdown)
{
    // Qt 6.x has QTextDocument::setMarkdown() — use it for real rendering
    QTextDocument doc;
    doc.setMarkdown(markdown, QTextDocument::MarkdownDialectCommonMark);
    QString html = doc.toHtml();

    // Post-process: convert @[Name|type:id] refs to styled spans
    return highlightEntityRefs(html);
}

QString ChapterEditorWidget::highlightEntityRefs(const QString& html)
{
    // Pattern: @[Name|type:id]
    // e.g. @[Kael|character:1] → <span class="entity-character">@Kael</span>
    static const QRegularExpression refPattern(
        R"(@\[([^\|]+)\|(\w+):(\d+)\])");

    QString result = html;
    result.replace(refPattern,
        R"(<span class="entity-\2">@\1</span>)");
    return result;
}

} // namespace CF::UI