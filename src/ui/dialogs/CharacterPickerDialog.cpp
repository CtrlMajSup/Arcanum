#include "CharacterPickerDialog.h"

#include <QTimer>
#include <QHBoxLayout>
#include <QLabel>

namespace CF::UI {

CharacterPickerDialog::CharacterPickerDialog(
    std::shared_ptr<Services::CharacterService> characterService,
    Domain::CharacterId excludeId,
    QWidget* parent)
    : QDialog(parent)
    , m_characterService(std::move(characterService))
    , m_excludeId(excludeId)
{
    setWindowTitle("Choose a character");
    setFixedSize(400, 440);
    setModal(true);
    setupUi();
    setupConnections();
    loadCharacters();
}

std::optional<Domain::CharacterId>
CharacterPickerDialog::selectedCharacterId() const
{
    return m_selectedId;
}

void CharacterPickerDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    root->addWidget(new QLabel("Select the relationship target:", this));

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Filter characters…");
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

void CharacterPickerDialog::setupConnections()
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
            this, &CharacterPickerDialog::onItemDoubleClicked);

    connect(m_okBtn,     &QPushButton::clicked,
            this, &CharacterPickerDialog::onAcceptClicked);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &QDialog::reject);
}

void CharacterPickerDialog::loadCharacters(const QString& filter)
{
    auto result = filter.isEmpty()
        ? m_characterService->getAll()
        : m_characterService->search(filter.toStdString());

    if (result.isErr()) return;

    m_list->clear();
    for (const auto& character : result.value()) {
        // Exclude the owner character
        if (character.id == m_excludeId) continue;

        auto* item = new QListWidgetItem(m_list);

        QString display = QString::fromStdString(character.name);
        if (!character.species.empty()) {
            display += QString("  ·  %1")
                .arg(QString::fromStdString(character.species));
        }
        if (!character.isAlive()) {
            display += "  [Deceased]";
        }

        item->setText(display);
        item->setData(Qt::UserRole,
                      static_cast<qlonglong>(character.id.value()));
        item->setToolTip(
            QString("Born: %1%2")
            .arg(QString::fromStdString(character.born.display()))
            .arg(!character.isAlive()
                 ? "\nDeceased: "
                   + QString::fromStdString(character.died->display())
                 : ""));

        if (!character.isAlive()) {
            item->setForeground(QColor("#6c7086"));
        }
        m_list->addItem(item);
    }
}

void CharacterPickerDialog::onSearchChanged(const QString& text)
{
    loadCharacters(text);
}

void CharacterPickerDialog::onItemDoubleClicked(QListWidgetItem*)
{
    onAcceptClicked();
}

void CharacterPickerDialog::onAcceptClicked()
{
    const auto* item = m_list->currentItem();
    if (!item) return;

    m_selectedId = Domain::CharacterId(
        item->data(Qt::UserRole).toLongLong());
    accept();
}

} // namespace CF::UI