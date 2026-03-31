#include "PlacePickerDialog.h"

#include <QTimer>
#include <QHBoxLayout>

namespace CF::UI {


PlacePickerDialog::PlacePickerDialog(
    std::shared_ptr<PlaceViewModel> placeViewModel,
    QWidget* parent)
    : QDialog(parent)
    , m_placeViewModel(std::move(placeViewModel))
{
    setWindowTitle("Choisir un lieu");
    setFixedSize(400, 480);
    setModal(true);
    setupUi();
    setupConnections();
    loadPlaces();
}

std::optional<Domain::PlaceId> PlacePickerDialog::selectedPlaceId() const
{
    return m_selectedId;
}

void PlacePickerDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    root->addWidget(new QLabel("Select the destination place:", this));

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Filter places…");
    m_searchEdit->setClearButtonEnabled(true);
    root->addWidget(m_searchEdit);

    m_list = new QListWidget(this);
    m_list->setObjectName("characterList");
    root->addWidget(m_list, 1);

    auto* btnRow = new QHBoxLayout();
    m_cancelBtn  = new QPushButton("Cancel", this);
    m_okBtn      = new QPushButton("Select", this);
    m_okBtn->setObjectName("primaryButton");
    m_okBtn->setEnabled(false);
    m_okBtn->setDefault(true);
    btnRow->addStretch();
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_okBtn);
    root->addLayout(btnRow);
}

void PlacePickerDialog::setupConnections()
{
    // Debounce search
    auto* timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(250);
    connect(m_searchEdit, &QLineEdit::textChanged,
            timer, qOverload<>(&QTimer::start));
    connect(timer, &QTimer::timeout, this, [this]() {
        onSearchChanged(m_searchEdit->text());
    });

    connect(m_list, &QListWidget::itemSelectionChanged,
            this, [this]() {
                m_okBtn->setEnabled(m_list->currentItem() != nullptr);
            });

    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &PlacePickerDialog::onItemDoubleClicked);

    connect(m_okBtn,     &QPushButton::clicked,
            this, &PlacePickerDialog::onAcceptClicked);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &QDialog::reject);
}

void PlacePickerDialog::loadPlaces(const QString& filter)
{
    m_list->clear();

    // Itère directement le cache du ViewModel — aucun appel DB
    const int count = m_placeViewModel->rowCount();
    for (int i = 0; i < count; ++i) {
        const auto place = m_placeViewModel->placeAt(i);
        if (!place.has_value()) continue;

        const QString name = QString::fromStdString(place->name);

        // Filtre côté client sur le cache
        if (!filter.isEmpty() &&
            !name.contains(filter, Qt::CaseInsensitive)) continue;

        auto* item = new QListWidgetItem(m_list);

        QString display = name;
        if (!place->region.empty()) {
            display += "  ·  " + QString::fromStdString(place->region);
        }
        if (place->isMobile) {
            display += "  [Mobile]";
        }

        item->setText(display);
        item->setData(Qt::UserRole,
                      static_cast<qlonglong>(place->id.value()));
        item->setToolTip(
            QString("Type: %1\nRégion: %2")
            .arg(QString::fromStdString(place->type))
            .arg(QString::fromStdString(place->region)));

        m_list->addItem(item);
    }
}

void PlacePickerDialog::onSearchChanged(const QString& text)
{
    loadPlaces(text);
}

void PlacePickerDialog::onItemDoubleClicked(QListWidgetItem*)
{
    onAcceptClicked();
}

void PlacePickerDialog::onAcceptClicked()
{
    const auto* item = m_list->currentItem();
    if (!item) return;

    m_selectedId = Domain::PlaceId(
        item->data(Qt::UserRole).toLongLong());
    accept();
}

} // namespace CF::UI