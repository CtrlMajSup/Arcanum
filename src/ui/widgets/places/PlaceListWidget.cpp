#include "PlaceListWidget.h"

#include <QMenu>
#include <QMessageBox>
#include <QTimer>

namespace CF::UI {

PlaceListWidget::PlaceListWidget(
    std::shared_ptr<PlaceViewModel> viewModel, QWidget* parent)
    : QWidget(parent)
    , m_viewModel(std::move(viewModel))
{
    setupUi();
    setupConnections();
    m_viewModel->loadAll();
}

void PlaceListWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    auto* headerRow = new QHBoxLayout();
    auto* title     = new QLabel("Places", this);
    title->setObjectName("sectionTitle");
    m_createButton  = new QPushButton("+", this);
    m_createButton->setObjectName("createButton");
    m_createButton->setFixedWidth(28);
    m_createButton->setToolTip("Create new place");
    headerRow->addWidget(title);
    headerRow->addStretch();
    headerRow->addWidget(m_createButton);

    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText("Search places...");
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

void PlaceListWidget::setupConnections()
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
            this, &PlaceListWidget::onSelectionChanged);

    connect(m_createButton, &QPushButton::clicked,
            this, &PlaceListWidget::onCreateClicked);

    connect(m_listView, &QListView::customContextMenuRequested,
            this, &PlaceListWidget::onContextMenuRequested);

    connect(m_viewModel.get(), &PlaceViewModel::errorOccurred,
            this, &PlaceListWidget::onError);

    connect(m_viewModel.get(), &QAbstractListModel::modelReset, this, [this]() {
        m_countLabel->setText(
            QString("%1 place(s)").arg(m_viewModel->rowCount()));
    });
}

void PlaceListWidget::refresh()
{
    m_viewModel->loadAll();
}

void PlaceListWidget::onSearchTextChanged(const QString& text)
{
    m_viewModel->search(text);
}

void PlaceListWidget::onSelectionChanged(
    const QModelIndex& current, const QModelIndex&)
{
    if (!current.isValid()) return;
    const auto place = m_viewModel->placeAt(current.row());
    if (place.has_value()) emit placeSelected(place->id);
}

void PlaceListWidget::onCreateClicked()
{
    emit createRequested();
}

void PlaceListWidget::onContextMenuRequested(const QPoint& pos)
{
    if (!m_listView->indexAt(pos).isValid()) return;
    QMenu menu(this);
    auto* del = menu.addAction("Delete place");
    connect(del, &QAction::triggered, this, &PlaceListWidget::onDeleteSelected);
    menu.exec(m_listView->viewport()->mapToGlobal(pos));
}

void PlaceListWidget::onDeleteSelected()
{
    const auto idx = m_listView->currentIndex();
    if (!idx.isValid()) return;

    const auto place = m_viewModel->placeAt(idx.row());
    if (!place.has_value()) return;

    const auto reply = QMessageBox::question(
        this, "Delete place",
        QString("Delete \"%1\"? This cannot be undone.")
            .arg(QString::fromStdString(place->name)),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
        m_viewModel->deletePlace(place->id);
}

void PlaceListWidget::onError(const QString& message)
{
    QMessageBox::warning(this, "Error", message);
}

} // namespace CF::UI