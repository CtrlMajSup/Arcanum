#include "ChapterListWidget.h"

#include <QInputDialog>
#include <QMessageBox>

namespace CF::UI {

ChapterListWidget::ChapterListWidget(
    std::shared_ptr<ChapterViewModel> viewModel, QWidget* parent)
    : QWidget(parent)
    , m_viewModel(std::move(viewModel))
{
    setupUi();
    setupConnections();
    m_viewModel->loadAll();
}

void ChapterListWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // Header
    auto* headerRow = new QHBoxLayout();
    auto* title = new QLabel("Chapters", this);
    title->setObjectName("sectionTitle");
    m_createButton = new QPushButton("+", this);
    m_createButton->setObjectName("createButton");
    m_createButton->setFixedWidth(28);
    m_createButton->setToolTip("New chapter");
    headerRow->addWidget(title);
    headerRow->addStretch();
    headerRow->addWidget(m_createButton);

    // List — drag-drop enabled
    m_listView = new QListView(this);
    m_listView->setModel(m_viewModel.get());
    m_listView->setDragDropMode(QAbstractItemView::InternalMove);
    m_listView->setDefaultDropAction(Qt::MoveAction);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setObjectName("characterList");  // Reuse style

    m_deleteButton = new QPushButton("Delete selected", this);
    m_deleteButton->setEnabled(false);

    m_countLabel = new QLabel(this);
    m_countLabel->setObjectName("countLabel");

    root->addLayout(headerRow);
    root->addWidget(m_listView, 1);
    root->addWidget(m_deleteButton);
    root->addWidget(m_countLabel);
}

void ChapterListWidget::setupConnections()
{
    connect(m_listView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this, &ChapterListWidget::onSelectionChanged);

    connect(m_createButton, &QPushButton::clicked,
            this, &ChapterListWidget::onCreateClicked);

    connect(m_deleteButton, &QPushButton::clicked,
            this, &ChapterListWidget::onDeleteClicked);

    connect(m_viewModel.get(), &ChapterViewModel::errorOccurred,
            this, &ChapterListWidget::onError);

    connect(m_viewModel.get(), &QAbstractListModel::modelReset, this, [this]() {
        m_countLabel->setText(
            QString("%1 chapter(s)").arg(m_viewModel->rowCount()));
    });
}

void ChapterListWidget::refresh()
{
    m_viewModel->loadAll();
}

void ChapterListWidget::onSelectionChanged(
    const QModelIndex& current, const QModelIndex&)
{
    m_deleteButton->setEnabled(current.isValid());
    if (current.isValid()) {
        m_viewModel->setActiveChapter(current.row());
        emit chapterSelected(current.row());
    }
}

void ChapterListWidget::onCreateClicked()
{
    bool ok = false;
    const QString title = QInputDialog::getText(
        this, "New chapter", "Chapter title:",
        QLineEdit::Normal, "", &ok);

    if (!ok || title.trimmed().isEmpty()) return;
    m_viewModel->createChapter(title.trimmed());
}

void ChapterListWidget::onDeleteClicked()
{
    const auto index = m_listView->currentIndex();
    if (!index.isValid()) return;

    const QString title = index.data(ChapterViewModel::TitleRole).toString();
    const auto reply = QMessageBox::question(
        this, "Delete chapter",
        QString("Delete \"%1\"? This cannot be undone.").arg(title),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    const qint64 id = index.data(ChapterViewModel::IdRole).toLongLong();
    m_viewModel->deleteChapter(Domain::ChapterId(id));
}

void ChapterListWidget::onError(const QString& message)
{
    QMessageBox::warning(this, "Error", message);
}

} // namespace CF::UI