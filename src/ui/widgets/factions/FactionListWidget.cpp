#include "FactionListWidget.h"

#include <QMenu>
#include <QMessageBox>
#include <QTimer>

namespace CF::UI {

FactionListWidget::FactionListWidget(
    std::shared_ptr<FactionViewModel> viewModel, QWidget* parent)
    : QWidget(parent)
    , m_viewModel(std::move(viewModel))
{
    setupUi();
    setupConnections();
    m_viewModel->loadAll();
}

void FactionListWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    auto* headerRow = new QHBoxLayout();
    auto* title     = new QLabel("Factions", this);
    title->setObjectName("sectionTitle");
    m_createButton  = new QPushButton("+", this);
    m_createButton->setObjectName("createButton");
    m_createButton->setFixedWidth(28);
    m_createButton->setToolTip("Create new faction");
    headerRow->addWidget(title);
    headerRow->addStretch();
    headerRow->addWidget(m_createButton);

    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText("Search factions...");
    m_searchBox->setClearButtonEnabled(true);
    m_searchBox->setObjectName("searchBox");

    m_listView = new QListView(this);
    m_listView->setModel(m_viewModel.get());
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setObjectName("characterList");

    m_countLabel = new QLabel(this);
    m_countLabel->setObjectName("countLabel");

    root->addLayout(headerRow);
    root->addWidget(m_searchBox);
    root->addWidget(m_listView, 1);
    root->addWidget(m_countLabel);
}

void FactionListWidget::setupConnections()
{
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
            this, &FactionListWidget::onSelectionChanged);

    connect(m_createButton, &QPushButton::clicked,
            this, &FactionListWidget::onCreateClicked);

    connect(m_listView, &QListView::customContextMenuRequested,
            this, &FactionListWidget::onContextMenuRequested);

    connect(m_viewModel.get(), &FactionViewModel::errorOccurred,
            this, &FactionListWidget::onError);

    connect(m_viewModel.get(), &QAbstractListModel::modelReset, this, [this]() {
        m_countLabel->setText(
            QString("%1 faction(s)").arg(m_viewModel->rowCount()));
    });
}

void FactionListWidget::refresh()
{
    m_viewModel->loadAll();
}

void FactionListWidget::onSearchTextChanged(const QString& text)
{
    m_viewModel->search(text);
}

void FactionListWidget::onSelectionChanged(
    const QModelIndex& current, const QModelIndex&)
{
    if (!current.isValid()) return;
    const auto faction = m_viewModel->factionAt(current.row());
    if (faction.has_value()) emit factionSelected(faction->id);
}

void FactionListWidget::onCreateClicked()
{
    emit createRequested();
}

void FactionListWidget::onContextMenuRequested(const QPoint& pos)
{
    if (!m_listView->indexAt(pos).isValid()) return;
    QMenu menu(this);
    auto* del = menu.addAction("Delete faction");
    connect(del, &QAction::triggered, this, &FactionListWidget::onDeleteSelected);
    menu.exec(m_listView->viewport()->mapToGlobal(pos));
}

void FactionListWidget::onDeleteSelected()
{
    const auto idx = m_listView->currentIndex();
    if (!idx.isValid()) return;
    const auto faction = m_viewModel->factionAt(idx.row());
    if (!faction.has_value()) return;

    const auto reply = QMessageBox::question(
        this, "Delete faction",
        QString("Delete \"%1\"? This cannot be undone.")
            .arg(QString::fromStdString(faction->name)),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
        m_viewModel->deleteFaction(faction->id);
}

void FactionListWidget::onError(const QString& message)
{
    QMessageBox::warning(this, "Error", message);
}

} // namespace CF::UI