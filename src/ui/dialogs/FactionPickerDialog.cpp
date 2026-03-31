#include "FactionPickerDialog.h"

#include <QTimer>
#include <QHBoxLayout>
#include <QLabel>

namespace CF::UI {

FactionPickerDialog::FactionPickerDialog(
    std::shared_ptr<FactionViewModel> factionViewModel,
    QWidget* parent)
    : QDialog(parent)
    , m_factionViewModel(std::move(factionViewModel))
{
    setWindowTitle("Choisir une faction");
    setFixedSize(400, 440);
    setModal(true);
    setupUi();
    setupConnections();
    loadFactions();
}

std::optional<Domain::FactionId> FactionPickerDialog::selectedFactionId() const
{
    return m_selectedId;
}

void FactionPickerDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    root->addWidget(new QLabel("Select a faction to join:", this));

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Filter factions…");
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

void FactionPickerDialog::setupConnections()
{
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
            this, &FactionPickerDialog::onItemDoubleClicked);

    connect(m_okBtn,     &QPushButton::clicked,
            this, &FactionPickerDialog::onAcceptClicked);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &QDialog::reject);
}

void FactionPickerDialog::loadFactions(const QString& filter)
{
    m_list->clear();

    const int count = m_factionViewModel->rowCount();
    for (int i = 0; i < count; ++i) {
        const auto faction = m_factionViewModel->factionAt(i);
        if (!faction.has_value()) continue;

        const QString name = QString::fromStdString(faction->name);

        if (!filter.isEmpty() &&
            !name.contains(filter, Qt::CaseInsensitive)) continue;

        auto* item = new QListWidgetItem(m_list);

        QString display = name;
        if (!faction->type.empty()) {
            display += "  ·  " + QString::fromStdString(faction->type);
        }
        if (faction->dissolved.has_value()) {
            display += "  [Dissoute]";
        }

        item->setText(display);
        item->setData(Qt::UserRole,
                      static_cast<qlonglong>(faction->id.value()));
        item->setToolTip(QString::fromStdString(faction->description));

        // Factions dissoutes affichées en gris
        if (faction->dissolved.has_value()) {
            item->setForeground(QColor("#6c7086"));
        }

        m_list->addItem(item);
    }
}

void FactionPickerDialog::onSearchChanged(const QString& text)
{
    loadFactions(text);
}

void FactionPickerDialog::onItemDoubleClicked(QListWidgetItem*)
{
    onAcceptClicked();
}

void FactionPickerDialog::onAcceptClicked()
{
    const auto* item = m_list->currentItem();
    if (!item) return;

    m_selectedId = Domain::FactionId(
        item->data(Qt::UserRole).toLongLong());
    accept();
}

} // namespace CF::UI