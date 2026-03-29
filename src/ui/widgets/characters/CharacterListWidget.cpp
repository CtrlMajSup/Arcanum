#include "CharacterListWidget.h"

#include <QMessageBox>
#include <QTimer>

namespace CF::UI {

CharacterListWidget::CharacterListWidget(
    std::shared_ptr<CharacterViewModel> viewModel, QWidget* parent)
    : QWidget(parent)
    , m_viewModel(std::move(viewModel))
{
    setupUi();
    setupConnections();
    m_viewModel->loadAll();
}

void CharacterListWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // ── Header row ────────────────────────────────────────────────────────────
    auto* headerRow = new QHBoxLayout();
    auto* title = new QLabel("Characters", this);
    title->setObjectName("sectionTitle");

    m_createButton = new QPushButton("+", this);
    m_createButton->setObjectName("createButton");
    m_createButton->setFixedWidth(28);
    m_createButton->setToolTip("Create new character");

    headerRow->addWidget(title);
    headerRow->addStretch();
    headerRow->addWidget(m_createButton);

    // ── Search ────────────────────────────────────────────────────────────────
    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText("Search characters...");
    m_searchBox->setClearButtonEnabled(true);
    m_searchBox->setObjectName("searchBox");

    // ── List ──────────────────────────────────────────────────────────────────
    m_listView = new QListView(this);
    m_listView->setModel(m_viewModel.get());
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setObjectName("characterList");

    // ── Footer count ──────────────────────────────────────────────────────────
    m_countLabel = new QLabel(this);
    m_countLabel->setObjectName("countLabel");

    root->addLayout(headerRow);
    root->addWidget(m_searchBox);
    root->addWidget(m_listView, 1);
    root->addWidget(m_countLabel);
}

void CharacterListWidget::setupConnections()
{
    // Debounce search input — wait 300ms after last keystroke
    auto* searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    searchTimer->setInterval(300);

    connect(m_searchBox, &QLineEdit::textChanged,
            searchTimer, qOverload<>(&QTimer::start));

    connect(searchTimer, &QTimer::timeout, this, [this]() {
        onSearchTextChanged(m_searchBox->text());
    });

    connect(m_listView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this, &CharacterListWidget::onSelectionChanged);

    connect(m_createButton, &QPushButton::clicked,
            this, &CharacterListWidget::onCreateClicked);

    connect(m_listView, &QListView::customContextMenuRequested,
            this, &CharacterListWidget::onContextMenuRequested);

    connect(m_viewModel.get(), &CharacterViewModel::errorOccurred,
            this, &CharacterListWidget::onError);

    // Update count label whenever the model resets or rows change
    connect(m_viewModel.get(), &QAbstractListModel::modelReset, this, [this]() {
        m_countLabel->setText(
            QString("%1 character(s)").arg(m_viewModel->rowCount()));
    });
}

void CharacterListWidget::refresh()
{
    m_viewModel->loadAll();
}

void CharacterListWidget::onSearchTextChanged(const QString& text)
{
    m_viewModel->search(text);
}

void CharacterListWidget::onSelectionChanged(
    const QModelIndex& current, const QModelIndex&)
{
    if (!current.isValid()) return;

    const auto character = m_viewModel->characterAt(current.row());
    if (character.has_value()) {
        emit characterSelected(character->id);
    }
}

void CharacterListWidget::onCreateClicked()
{
    emit createRequested();
}

void CharacterListWidget::onContextMenuRequested(const QPoint& pos)
{
    const auto index = m_listView->indexAt(pos);
    if (!index.isValid()) return;

    QMenu menu(this);
    auto* deleteAction = menu.addAction("Delete character");
    deleteAction->setIcon(QIcon::fromTheme("edit-delete"));

    connect(deleteAction, &QAction::triggered,
            this, &CharacterListWidget::onDeleteSelected);

    menu.exec(m_listView->viewport()->mapToGlobal(pos));
}

void CharacterListWidget::onDeleteSelected()
{
    const auto index = m_listView->currentIndex();
    if (!index.isValid()) return;

    const auto character = m_viewModel->characterAt(index.row());
    if (!character.has_value()) return;

    const auto reply = QMessageBox::question(
        this,
        "Delete character",
        QString("Delete \"%1\"? This cannot be undone.")
            .arg(QString::fromStdString(character->name)),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_viewModel->deleteCharacter(character->id);
    }
}

void CharacterListWidget::onError(const QString& message)
{
    QMessageBox::warning(this, "Error", message);
}

} // namespace CF::UI