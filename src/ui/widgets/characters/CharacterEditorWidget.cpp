#include "CharacterEditorWidget.h"
#include "core/Assert.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollArea>

#include "ui/dialogs/PlacePickerDialog.h"
#include "ui/dialogs/FactionPickerDialog.h"
#include "ui/dialogs/CharacterPickerDialog.h"

namespace CF::UI {

using namespace CF::Domain;
using namespace CF::Core;


CharacterEditorWidget::CharacterEditorWidget(
    std::shared_ptr<CharacterViewModel> viewModel,
    std::shared_ptr<PlaceViewModel>     placeViewModel,
    std::shared_ptr<FactionViewModel>   factionViewModel,
    QWidget* parent)
    : QWidget(parent)
    , m_viewModel(std::move(viewModel))
    , m_placeViewModel(std::move(placeViewModel))
    , m_factionViewModel(std::move(factionViewModel))
{
    CF_REQUIRE(m_viewModel      != nullptr, "CharacterViewModel must not be null");
    CF_REQUIRE(m_placeViewModel != nullptr, "PlaceViewModel must not be null");
    CF_REQUIRE(m_factionViewModel != nullptr, "FactionViewModel must not be null");

    setupUi();
    setupConnections();
    clearEditor();
}

// ── Public API ────────────────────────────────────────────────────────────────

void CharacterEditorWidget::loadCharacter(CharacterId id)
{
    const auto character = m_viewModel->characterById(id);
    if (!character.has_value()) return;

    m_currentCharacter = character;
    populateFromCharacter(*m_currentCharacter);
    setEnabled(true);
}

void CharacterEditorWidget::clearEditor()
{
    m_currentCharacter.reset();
    m_nameEdit->clear();
    m_speciesEdit->clear();
    m_biographyEdit->clear();
    m_bornEraBox->setValue(1);
    m_bornYearBox->setValue(1);
    m_isDeadCheck->setChecked(false);
    if (m_evolutionsTable) m_evolutionsTable->setRowCount(0);
    if (m_locationsTable)  m_locationsTable->setRowCount(0);
    if (m_factionsTable)   m_factionsTable->setRowCount(0);
    if (m_relationsTable)  m_relationsTable->setRowCount(0);
    setEnabled(false);
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void CharacterEditorWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto* tabs = new QTabWidget(this);
    setupIdentityTab(tabs);
    setupEvolutionsTab(tabs);
    setupLocationsTab(tabs);
    setupFactionsTab(tabs);
    setupRelationsTab(tabs);

    root->addWidget(tabs, 1);
}

void CharacterEditorWidget::setupIdentityTab(QTabWidget* tabs)
{
    auto* widget = new QWidget();
    auto* scroll = new QScrollArea();
    scroll->setWidget(widget);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    // Name
    m_nameEdit = new QLineEdit(widget);
    m_nameEdit->setPlaceholderText("Character name");
    layout->addRow("Name:", m_nameEdit);

    // Species
    m_speciesEdit = new QLineEdit(widget);
    m_speciesEdit->setPlaceholderText("e.g. Human, Elf, Android");
    layout->addRow("Species:", m_speciesEdit);

    // Born
    auto* bornRow = new QHBoxLayout();
    m_bornEraBox  = new QSpinBox(widget);
    m_bornEraBox->setRange(1, 9999);
    m_bornEraBox->setPrefix("Era ");
    m_bornYearBox = new QSpinBox(widget);
    m_bornYearBox->setRange(1, 99999);
    m_bornYearBox->setPrefix("Year ");
    bornRow->addWidget(m_bornEraBox);
    bornRow->addWidget(m_bornYearBox);
    layout->addRow("Born:", bornRow);

    // Died
    m_isDeadCheck = new QCheckBox("Deceased", widget);
    auto* diedRow = new QHBoxLayout();
    m_diedEraBox  = new QSpinBox(widget);
    m_diedEraBox->setRange(1, 9999);
    m_diedEraBox->setPrefix("Era ");
    m_diedYearBox = new QSpinBox(widget);
    m_diedYearBox->setRange(1, 99999);
    m_diedYearBox->setPrefix("Year ");
    diedRow->addWidget(m_isDeadCheck);
    diedRow->addWidget(m_diedEraBox);
    diedRow->addWidget(m_diedYearBox);
    layout->addRow("Death:", diedRow);

    // Enable/disable death fields based on checkbox
    auto updateDiedFields = [this](bool checked) {
        m_diedEraBox->setEnabled(checked);
        m_diedYearBox->setEnabled(checked);
    };
    connect(m_isDeadCheck, &QCheckBox::toggled, widget, updateDiedFields);
    updateDiedFields(false);

    // Biography
    m_biographyEdit = new QTextEdit(widget);
    m_biographyEdit->setPlaceholderText("Character biography and backstory...");
    m_biographyEdit->setMinimumHeight(120);
    layout->addRow("Biography:", m_biographyEdit);

    // Save button
    m_saveButton = new QPushButton("Save changes", widget);
    m_saveButton->setObjectName("primaryButton");
    layout->addRow("", m_saveButton);

    tabs->addTab(scroll, "Identity");
}

void CharacterEditorWidget::setupEvolutionsTab(QTabWidget* tabs)
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(8, 8, 8, 8);

    // Evolutions table
    m_evolutionsTable = new QTableWidget(0, 3, widget);
    m_evolutionsTable->setHorizontalHeaderLabels({"Era", "Year", "Description"});
    m_evolutionsTable->horizontalHeader()->setStretchLastSection(true);
    m_evolutionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_evolutionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_evolutionsTable, 1);

    // Add row
    auto* addGroup = new QGroupBox("Add evolution", widget);
    auto* addLayout = new QHBoxLayout(addGroup);
    m_evoEraBox   = new QSpinBox(addGroup); m_evoEraBox->setRange(1,9999); m_evoEraBox->setPrefix("Era ");
    m_evoYearBox  = new QSpinBox(addGroup); m_evoYearBox->setRange(1,99999); m_evoYearBox->setPrefix("Year ");
    m_evoDescEdit = new QLineEdit(addGroup); m_evoDescEdit->setPlaceholderText("What changed?");
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

void CharacterEditorWidget::setupLocationsTab(QTabWidget* tabs)
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(8, 8, 8, 8);

    m_locationsTable = new QTableWidget(0, 5, widget);
    m_locationsTable->setHorizontalHeaderLabels(
        {"Place ID", "From Era", "From Year", "To", "Note"});
    m_locationsTable->horizontalHeader()->setStretchLastSection(true);
    m_locationsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_locationsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_locationsTable, 1);

    auto* addGroup = new QGroupBox("Move to new location", widget);
    auto* addLayout = new QHBoxLayout(addGroup);
    m_locEraBox   = new QSpinBox(addGroup); m_locEraBox->setRange(1,9999); m_locEraBox->setPrefix("Era ");
    m_locYearBox  = new QSpinBox(addGroup); m_locYearBox->setRange(1,99999); m_locYearBox->setPrefix("Year ");
    m_locNoteEdit = new QLineEdit(addGroup); m_locNoteEdit->setPlaceholderText("Note (optional)");
    m_addLocButton = new QPushButton("Move", addGroup);
    addLayout->addWidget(new QLabel("When:"));
    addLayout->addWidget(m_locEraBox);
    addLayout->addWidget(m_locYearBox);
    addLayout->addWidget(m_locNoteEdit, 1);
    addLayout->addWidget(m_addLocButton);
    layout->addWidget(addGroup);

    tabs->addTab(widget, "Locations");
}

void CharacterEditorWidget::setupFactionsTab(QTabWidget* tabs)
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(8, 8, 8, 8);

    m_factionsTable = new QTableWidget(0, 5, widget);
    m_factionsTable->setHorizontalHeaderLabels(
        {"Faction ID", "From Era", "From Year", "Active", "Role"});
    m_factionsTable->horizontalHeader()->setStretchLastSection(true);
    m_factionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_factionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_factionsTable, 1);

    auto* addGroup = new QGroupBox("Faction membership", widget);
    auto* addLayout = new QHBoxLayout(addGroup);
    m_facEraBox    = new QSpinBox(addGroup); m_facEraBox->setRange(1,9999); m_facEraBox->setPrefix("Era ");
    m_facYearBox   = new QSpinBox(addGroup); m_facYearBox->setRange(1,99999); m_facYearBox->setPrefix("Year ");
    m_facRoleEdit  = new QLineEdit(addGroup); m_facRoleEdit->setPlaceholderText("Role (e.g. Commander)");
    m_joinFacButton  = new QPushButton("Join", addGroup);
    m_leaveFacButton = new QPushButton("Leave selected", addGroup);
    addLayout->addWidget(new QLabel("When:"));
    addLayout->addWidget(m_facEraBox);
    addLayout->addWidget(m_facYearBox);
    addLayout->addWidget(m_facRoleEdit, 1);
    addLayout->addWidget(m_joinFacButton);
    addLayout->addWidget(m_leaveFacButton);
    layout->addWidget(addGroup);

    tabs->addTab(widget, "Factions");
}

void CharacterEditorWidget::setupRelationsTab(QTabWidget* tabs)
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(8, 8, 8, 8);

    m_relationsTable = new QTableWidget(0, 4, widget);
    m_relationsTable->setHorizontalHeaderLabels(
        {"Target", "Type", "Since", "Note"});
    m_relationsTable->horizontalHeader()->setStretchLastSection(true);
    m_relationsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_relationsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_relationsTable, 1);

    auto* addGroup = new QGroupBox("Add relationship", widget);
    auto* addLayout = new QHBoxLayout(addGroup);

    m_relTypeCombo = new QComboBox(addGroup);
    for (const auto& t : {"Ally","Enemy","Mentor","Student","Family","Romantic","Neutral"})
        m_relTypeCombo->addItem(t);

    m_relTargetEdit = new QLineEdit(addGroup);
    m_relTargetEdit->setPlaceholderText("Target character ID");
    m_relEraBox   = new QSpinBox(addGroup); m_relEraBox->setRange(1,9999); m_relEraBox->setPrefix("Era ");
    m_relYearBox  = new QSpinBox(addGroup); m_relYearBox->setRange(1,99999); m_relYearBox->setPrefix("Year ");
    m_relNoteEdit = new QLineEdit(addGroup); m_relNoteEdit->setPlaceholderText("Note");
    m_addRelButton    = new QPushButton("Add", addGroup);
    m_removeRelButton = new QPushButton("Remove", addGroup);

    addLayout->addWidget(m_relTypeCombo);
    addLayout->addWidget(new QLabel("Target ID:"));
    addLayout->addWidget(m_relTargetEdit);
    addLayout->addWidget(m_relEraBox);
    addLayout->addWidget(m_relYearBox);
    addLayout->addWidget(m_relNoteEdit, 1);
    addLayout->addWidget(m_addRelButton);
    addLayout->addWidget(m_removeRelButton);
    layout->addWidget(addGroup);

    tabs->addTab(widget, "Relations");
}

void CharacterEditorWidget::setupConnections()
{
    connect(m_saveButton, &QPushButton::clicked,
            this, &CharacterEditorWidget::onSaveClicked);
    connect(m_addEvoButton, &QPushButton::clicked,
            this, &CharacterEditorWidget::onAddEvolutionClicked);
    connect(m_removeEvoButton, &QPushButton::clicked,
            this, &CharacterEditorWidget::onRemoveEvolutionClicked);
    connect(m_addLocButton, &QPushButton::clicked,
            this, &CharacterEditorWidget::onAddLocationClicked);
    connect(m_joinFacButton, &QPushButton::clicked,
            this, &CharacterEditorWidget::onAddFactionClicked);
    connect(m_leaveFacButton, &QPushButton::clicked,
            this, &CharacterEditorWidget::onLeaveFactiomClicked);
    connect(m_addRelButton, &QPushButton::clicked,
            this, &CharacterEditorWidget::onAddRelationshipClicked);
    connect(m_removeRelButton, &QPushButton::clicked,
            this, &CharacterEditorWidget::onRemoveRelationshipClicked);

    // Refresh editor when the underlying model updates this character
    connect(m_viewModel.get(), &CharacterViewModel::characterUpdated,
            this, [this](qint64 id) {
                if (m_currentCharacter && m_currentCharacter->id.value() == id) {
                    loadCharacter(CharacterId(id));
                }
            });
}

// ── Populate ──────────────────────────────────────────────────────────────────

void CharacterEditorWidget::populateFromCharacter(const Character& c)
{
    m_nameEdit->setText(QString::fromStdString(c.name));
    m_speciesEdit->setText(QString::fromStdString(c.species));
    m_biographyEdit->setPlainText(QString::fromStdString(c.biography));
    m_bornEraBox->setValue(c.born.era);
    m_bornYearBox->setValue(c.born.year);
    m_isDeadCheck->setChecked(!c.isAlive());

    if (c.died.has_value()) {
        m_diedEraBox->setValue(c.died->era);
        m_diedYearBox->setValue(c.died->year);
    }

    populateEvolutionsTable(c);
    populateLocationsTable(c);
    populateFactionsTable(c);
    populateRelationsTable(c);
}

void CharacterEditorWidget::populateEvolutionsTable(const Character& c)
{
    m_evolutionsTable->setRowCount(0);
    for (const auto& evo : c.evolutions) {
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

void CharacterEditorWidget::populateLocationsTable(const Character& c)
{
    m_locationsTable->setRowCount(0);
    for (const auto& stint : c.locationHistory) {
        const int row = m_locationsTable->rowCount();
        m_locationsTable->insertRow(row);
        m_locationsTable->setItem(row, 0,
            new QTableWidgetItem(QString::number(stint.placeId.value())));
        m_locationsTable->setItem(row, 1,
            new QTableWidgetItem(QString::number(stint.from.era)));
        m_locationsTable->setItem(row, 2,
            new QTableWidgetItem(QString::number(stint.from.year)));
        m_locationsTable->setItem(row, 3,
            new QTableWidgetItem(stint.isCurrent()
                ? "Current"
                : (stint.to ? QString("Era %1, Y%2")
                    .arg(stint.to->era).arg(stint.to->year) : "-")));
        m_locationsTable->setItem(row, 4,
            new QTableWidgetItem(QString::fromStdString(stint.note)));
    }
}

void CharacterEditorWidget::populateFactionsTable(const Character& c)
{
    m_factionsTable->setRowCount(0);
    for (const auto& m : c.factionMemberships) {
        const int row = m_factionsTable->rowCount();
        m_factionsTable->insertRow(row);
        m_factionsTable->setItem(row, 0,
            new QTableWidgetItem(QString::number(m.factionId.value())));
        m_factionsTable->setItem(row, 1,
            new QTableWidgetItem(QString::number(m.from.era)));
        m_factionsTable->setItem(row, 2,
            new QTableWidgetItem(QString::number(m.from.year)));
        m_factionsTable->setItem(row, 3,
            new QTableWidgetItem(m.isCurrent() ? "Active" : "Ended"));
        m_factionsTable->setItem(row, 4,
            new QTableWidgetItem(QString::fromStdString(m.role)));
    }
}

void CharacterEditorWidget::populateRelationsTable(const Character& c)
{
    m_relationsTable->setRowCount(0);
    for (const auto& rel : c.relationships) {
        const int row = m_relationsTable->rowCount();
        m_relationsTable->insertRow(row);
        m_relationsTable->setItem(row, 0,
            new QTableWidgetItem(QString::number(rel.targetId.value())));
        m_relationsTable->setItem(row, 1,
            new QTableWidgetItem(QString::fromStdString(
                Relationship::typeToString(rel.type))));
        m_relationsTable->setItem(row, 2,
            new QTableWidgetItem(QString::fromStdString(rel.since.display())));
        m_relationsTable->setItem(row, 3,
            new QTableWidgetItem(QString::fromStdString(rel.note)));
    }
}

// ── Slot implementations ──────────────────────────────────────────────────────

void CharacterEditorWidget::onSaveClicked()
{
    if (!m_currentCharacter.has_value()) return;

    Character updated = buildCharacterFromForm();
    m_viewModel->updateCharacter(updated);
    emit characterSaved(updated.id);
}

void CharacterEditorWidget::onAddEvolutionClicked()
{
    if (!m_currentCharacter.has_value()) return;
    if (m_evoDescEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation", "Evolution description is required.");
        return;
    }

    m_viewModel->addEvolution(
        m_currentCharacter->id,
        m_evoEraBox->value(),
        m_evoYearBox->value(),
        m_evoDescEdit->text().trimmed()
    );
    m_evoDescEdit->clear();
}

void CharacterEditorWidget::onRemoveEvolutionClicked()
{
    if (!m_currentCharacter.has_value()) return;
    const int row = m_evolutionsTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_currentCharacter->evolutions.size()))
        return;

    const auto& evo = m_currentCharacter->evolutions[row];
    m_viewModel->removeEvolution(m_currentCharacter->id, evo.at.era, evo.at.year);
}

void CharacterEditorWidget::onAddLocationClicked()
{
    if (!m_currentCharacter.has_value()) return;

    // Ouvre le picker en lui passant le PlaceViewModel
    // Le dialog itère le cache du ViewModel — pas d'appel direct au service
    PlacePickerDialog dlg(m_placeViewModel, this);

    if (dlg.exec() != QDialog::Accepted) return;

    const auto placeId = dlg.selectedPlaceId();
    if (!placeId.has_value()) return;

    // Les spinboxes era/year de l'onglet Locations donnent le moment du déplacement
    m_viewModel->moveCharacter(
        m_currentCharacter->id,
        *placeId,
        m_locEraBox->value(),
        m_locYearBox->value(),
        m_locNoteEdit->text().trimmed()
    );

    m_locNoteEdit->clear();
}

void CharacterEditorWidget::onAddFactionClicked()
{
    if (!m_currentCharacter.has_value()) return;

    // Ouvre le picker en lui passant le FactionViewModel
    FactionPickerDialog dlg(m_factionViewModel, this);

    if (dlg.exec() != QDialog::Accepted) return;

    const auto factionId = dlg.selectedFactionId();
    if (!factionId.has_value()) return;

    m_viewModel->joinFaction(
        m_currentCharacter->id,
        *factionId,
        m_facEraBox->value(),
        m_facYearBox->value(),
        m_facRoleEdit->text().trimmed()
    );

    m_facRoleEdit->clear();
}

void CharacterEditorWidget::onLeaveFactiomClicked()
{
    if (!m_currentCharacter.has_value()) return;
    const int row = m_factionsTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_currentCharacter->factionMemberships.size()))
        return;

    const auto& membership = m_currentCharacter->factionMemberships[row];
    if (!membership.isCurrent()) {
        QMessageBox::information(this, "Info", "This membership is already closed.");
        return;
    }

    m_viewModel->leaveFaction(
        m_currentCharacter->id,
        membership.factionId,
        m_facEraBox->value(),
        m_facYearBox->value()
    );
}

void CharacterEditorWidget::onAddRelationshipClicked()
{
    if (!m_currentCharacter.has_value()) return;

    // Ouvre le picker en lui passant le CharacterViewModel
    // On exclut le personnage courant de la liste
    CharacterPickerDialog dlg(
        m_viewModel,
        m_currentCharacter->id,
        this);

    if (dlg.exec() != QDialog::Accepted) return;

    const auto targetId = dlg.selectedCharacterId();
    if (!targetId.has_value()) return;

    Domain::Relationship rel;
    rel.targetId = *targetId;
    rel.type     = Domain::Relationship::typeFromString(
                       m_relTypeCombo->currentText().toStdString());
    rel.since    = { m_relEraBox->value(), m_relYearBox->value() };
    rel.note     = m_relNoteEdit->text().toStdString();

    m_viewModel->setRelationship(m_currentCharacter->id, rel);
    m_relNoteEdit->clear();
}

void CharacterEditorWidget::onRemoveRelationshipClicked()
{
    if (!m_currentCharacter.has_value()) return;
    const int row = m_relationsTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_currentCharacter->relationships.size()))
        return;

    const auto& rel = m_currentCharacter->relationships[row];
    m_viewModel->removeRelationship(m_currentCharacter->id, rel.targetId);
}

// ── Build Character from form ─────────────────────────────────────────────────

Character CharacterEditorWidget::buildCharacterFromForm() const
{
    CF_ASSERT(m_currentCharacter.has_value(), "No character loaded in editor");

    Character c = *m_currentCharacter;   // Start from existing (preserves sub-data)
    c.name      = m_nameEdit->text().trimmed().toStdString();
    c.species   = m_speciesEdit->text().trimmed().toStdString();
    c.biography = m_biographyEdit->toPlainText().toStdString();
    c.born      = { m_bornEraBox->value(), m_bornYearBox->value() };

    if (m_isDeadCheck->isChecked()) {
        c.died = TimePoint{ m_diedEraBox->value(), m_diedYearBox->value() };
    } else {
        c.died.reset();
    }

    return c;
}

} // namespace CF::UI