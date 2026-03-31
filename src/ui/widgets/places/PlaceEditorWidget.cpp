#include "PlaceEditorWidget.h"
#include "core/Assert.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QGroupBox>
#include <QScrollArea>
#include <QMessageBox>

namespace CF::UI {

using namespace CF::Domain;
using namespace CF::Core;

PlaceEditorWidget::PlaceEditorWidget(
    std::shared_ptr<PlaceViewModel>     placeViewModel,
    std::shared_ptr<CharacterViewModel> characterViewModel,
    QWidget* parent)
    : QWidget(parent)
    , m_viewModel(std::move(placeViewModel))
    , m_characterViewModel(std::move(characterViewModel))
{
    CF_REQUIRE(m_viewModel           != nullptr, "PlaceViewModel must not be null");
    CF_REQUIRE(m_characterViewModel  != nullptr, "CharacterViewModel must not be null");
    setupUi();
    setupConnections();
    clearEditor();
}

void PlaceEditorWidget::loadPlace(PlaceId id)
{
    const auto place = m_viewModel->placeById(id);
    if (!place.has_value()) return;
    m_currentPlace = place;
    populateFromPlace(*m_currentPlace);
    populateCharactersTable(id);   // NEW
    setEnabled(true);
}

void PlaceEditorWidget::clearEditor()
{
    m_currentPlace.reset();
    if (m_nameEdit)   m_nameEdit->clear();
    if (m_typeEdit)   m_typeEdit->clear();
    if (m_regionEdit) m_regionEdit->clear();
    if (m_descriptionEdit) m_descriptionEdit->clear();
    if (m_evolutionsTable) m_evolutionsTable->setRowCount(0);
    setEnabled(false);
}

void PlaceEditorWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto* tabs = new QTabWidget(this);
    setupIdentityTab(tabs);
    setupEvolutionsTab(tabs);
    setupCharactersTab(tabs);   // NEW
    root->addWidget(tabs, 1);
}

void PlaceEditorWidget::setupIdentityTab(QTabWidget* tabs)
{
    auto* widget = new QWidget();
    auto* scroll = new QScrollArea();
    scroll->setWidget(widget);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    m_nameEdit = new QLineEdit(widget);
    m_nameEdit->setPlaceholderText("Place name");
    layout->addRow("Name:", m_nameEdit);

    m_typeEdit = new QLineEdit(widget);
    m_typeEdit->setPlaceholderText("e.g. City, Spaceship, Station, Region");
    layout->addRow("Type:", m_typeEdit);

    m_regionEdit = new QLineEdit(widget);
    m_regionEdit->setPlaceholderText("Region / zone name");
    layout->addRow("Region:", m_regionEdit);

    m_mobileCheck = new QCheckBox("Mobile (spaceship, vehicle…)", widget);
    layout->addRow("", m_mobileCheck);

    m_descriptionEdit = new QTextEdit(widget);
    m_descriptionEdit->setPlaceholderText("Description...");
    m_descriptionEdit->setMinimumHeight(100);
    layout->addRow("Description:", m_descriptionEdit);

    // Map position (stored as 0–100 integer %, converted to 0.0–1.0 on save)
    auto* posRow = new QHBoxLayout();
    m_mapXBox = new QSpinBox(widget);
    m_mapXBox->setRange(0, 100); m_mapXBox->setSuffix("%");
    m_mapYBox = new QSpinBox(widget);
    m_mapYBox->setRange(0, 100); m_mapYBox->setSuffix("%");
    posRow->addWidget(new QLabel("X:", widget));
    posRow->addWidget(m_mapXBox);
    posRow->addSpacing(12);
    posRow->addWidget(new QLabel("Y:", widget));
    posRow->addWidget(m_mapYBox);
    posRow->addStretch();
    layout->addRow("Map position:", posRow);

    m_saveButton = new QPushButton("Save changes", widget);
    m_saveButton->setObjectName("primaryButton");
    layout->addRow("", m_saveButton);

    tabs->addTab(scroll, "Identity");
}

void PlaceEditorWidget::setupEvolutionsTab(QTabWidget* tabs)
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(8, 8, 8, 8);

    m_evolutionsTable = new QTableWidget(0, 3, widget);
    m_evolutionsTable->setHorizontalHeaderLabels({"Era", "Year", "Description"});
    m_evolutionsTable->horizontalHeader()->setStretchLastSection(true);
    m_evolutionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_evolutionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_evolutionsTable, 1);

    auto* addGroup = new QGroupBox("Add evolution", widget);
    auto* addLayout = new QHBoxLayout(addGroup);
    m_evoEraBox  = new QSpinBox(addGroup);
    m_evoEraBox->setRange(1, 9999); m_evoEraBox->setPrefix("Era ");
    m_evoYearBox = new QSpinBox(addGroup);
    m_evoYearBox->setRange(1, 99999); m_evoYearBox->setPrefix("Year ");
    m_evoDescEdit = new QLineEdit(addGroup);
    m_evoDescEdit->setPlaceholderText("What changed?");
    m_addEvoButton    = new QPushButton("Add", addGroup);
    m_removeEvoButton = new QPushButton("Remove selected", addGroup);
    addLayout->addWidget(m_evoEraBox);
    addLayout->addWidget(m_evoYearBox);
    addLayout->addWidget(m_evoDescEdit, 1);
    addLayout->addWidget(m_addEvoButton);
    addLayout->addWidget(m_removeEvoButton);
    layout->addWidget(addGroup);

    tabs->addTab(widget, "Timeline");
}

void PlaceEditorWidget::setupCharactersTab(QTabWidget* tabs)
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto* label = new QLabel(
        "Characters currently located here:", widget);
    label->setObjectName("countLabel");
    layout->addWidget(label);

    // 3 columns: Name | Species | [Open]
    m_charactersTable = new QTableWidget(0, 3, widget);
    m_charactersTable->setHorizontalHeaderLabels({"Name", "Species", ""});
    m_charactersTable->horizontalHeader()->setStretchLastSection(false);
    m_charactersTable->horizontalHeader()
        ->setSectionResizeMode(0, QHeaderView::Stretch);
    m_charactersTable->horizontalHeader()
        ->setSectionResizeMode(1, QHeaderView::Stretch);
    m_charactersTable->horizontalHeader()
        ->setSectionResizeMode(2, QHeaderView::Fixed);
    m_charactersTable->setColumnWidth(2, 60);
    m_charactersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_charactersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_charactersTable, 1);

    auto* refreshBtn = new QPushButton("Refresh", widget);
    refreshBtn->setObjectName("timelineBtn");
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentPlace.has_value())
            populateCharactersTable(m_currentPlace->id);
    });
    layout->addWidget(refreshBtn);

    tabs->addTab(widget, "Characters");
}

void PlaceEditorWidget::setupConnections()
{
    connect(m_saveButton,     &QPushButton::clicked,
            this, &PlaceEditorWidget::onSaveClicked);
    connect(m_addEvoButton,   &QPushButton::clicked,
            this, &PlaceEditorWidget::onAddEvolutionClicked);
    connect(m_removeEvoButton,&QPushButton::clicked,
            this, &PlaceEditorWidget::onRemoveEvolutionClicked);

    connect(m_viewModel.get(), &PlaceViewModel::placeUpdated,
            this, [this](qint64 id) {
                if (m_currentPlace && m_currentPlace->id.value() == id)
                    loadPlace(Domain::PlaceId(id));
            });
}

void PlaceEditorWidget::populateFromPlace(const Place& p)
{
    m_nameEdit->setText(QString::fromStdString(p.name));
    m_typeEdit->setText(QString::fromStdString(p.type));
    m_regionEdit->setText(QString::fromStdString(p.region));
    m_mobileCheck->setChecked(p.isMobile);
    m_descriptionEdit->setPlainText(QString::fromStdString(p.description));
    m_mapXBox->setValue(static_cast<int>(p.mapX * 100.0));
    m_mapYBox->setValue(static_cast<int>(p.mapY * 100.0));
    populateEvolutionsTable(p);
}

void PlaceEditorWidget::populateEvolutionsTable(const Place& p)
{
    m_evolutionsTable->setRowCount(0);
    for (const auto& evo : p.evolutions) {
        const int row = m_evolutionsTable->rowCount();
        m_evolutionsTable->insertRow(row);
        m_evolutionsTable->setItem(row, 0,
            new QTableWidgetItem(QString::number(evo.at.era)));
        m_evolutionsTable->setItem(row, 1,
            new QTableWidgetItem(QString::number(evo.at.year)));
        m_evolutionsTable->setItem(row, 2,
            new QTableWidgetItem(QString::fromStdString(evo.description)));
    }
}

void PlaceEditorWidget::populateCharactersTable(Domain::PlaceId placeId)
{
    m_charactersTable->setRowCount(0);

    // Iterate all characters in the ViewModel cache
    // and find those whose current location is this place
    const int count = m_characterViewModel->rowCount();
    for (int i = 0; i < count; ++i) {
        const auto character = m_characterViewModel->characterAt(i);
        if (!character.has_value()) continue;

        // Check if character is currently at this place
        bool isHere = false;
        for (const auto& stint : character->locationHistory) {
            if (stint.placeId == placeId && stint.isCurrent()) {
                isHere = true;
                break;
            }
        }
        if (!isHere) continue;

        const int row = m_charactersTable->rowCount();
        m_charactersTable->insertRow(row);

        m_charactersTable->setItem(row, 0,
            new QTableWidgetItem(
                QString::fromStdString(character->name)));
        m_charactersTable->setItem(row, 1,
            new QTableWidgetItem(
                QString::fromStdString(character->species)));

        // "Open →" navigates to character editor
        auto* openBtn = new QPushButton("Open →", m_charactersTable);
        openBtn->setObjectName("timelineBtn");
        openBtn->setFixedHeight(24);
        const Domain::CharacterId charId = character->id;
        connect(openBtn, &QPushButton::clicked,
                this, [this, charId]() {
                    emit navigateToCharacter(charId);
                });
        m_charactersTable->setCellWidget(row, 2, openBtn);
    }

    // Update tab label with count
    // (parent tab widget accessed via parentWidget chain)
    const int found = m_charactersTable->rowCount();
    if (auto* tabs = qobject_cast<QTabWidget*>(
            m_charactersTable->parentWidget()
                             ->parentWidget()
                             ->parentWidget())) {
        tabs->setTabText(2,
            found > 0
            ? QString("Characters (%1)").arg(found)
            : "Characters");
    }
}


void PlaceEditorWidget::onSaveClicked()
{
    if (!m_currentPlace.has_value()) return;
    Place updated = buildPlaceFromForm();
    m_viewModel->updatePlace(updated);
    emit placeSaved(updated.id);
}

void PlaceEditorWidget::onAddEvolutionClicked()
{
    if (!m_currentPlace.has_value()) return;
    if (m_evoDescEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation",
                             "Evolution description is required.");
        return;
    }
    m_viewModel->addEvolution(
        m_currentPlace->id,
        m_evoEraBox->value(),
        m_evoYearBox->value(),
        m_evoDescEdit->text().trimmed());
    m_evoDescEdit->clear();
}

void PlaceEditorWidget::onRemoveEvolutionClicked()
{
    if (!m_currentPlace.has_value()) return;
    const int row = m_evolutionsTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_currentPlace->evolutions.size()))
        return;
    const auto& evo = m_currentPlace->evolutions[row];
    m_viewModel->removeEvolution(
        m_currentPlace->id, evo.at.era, evo.at.year);
}

Place PlaceEditorWidget::buildPlaceFromForm() const
{
    CF_ASSERT(m_currentPlace.has_value(), "No place loaded in editor");
    Place p       = *m_currentPlace;
    p.name        = m_nameEdit->text().trimmed().toStdString();
    p.type        = m_typeEdit->text().trimmed().toStdString();
    p.region      = m_regionEdit->text().trimmed().toStdString();
    p.isMobile    = m_mobileCheck->isChecked();
    p.description = m_descriptionEdit->toPlainText().toStdString();
    p.mapX        = m_mapXBox->value() / 100.0;
    p.mapY        = m_mapYBox->value() / 100.0;
    return p;
}

} // namespace CF::UI