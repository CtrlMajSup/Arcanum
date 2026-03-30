#include "FactionEditorWidget.h"
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

FactionEditorWidget::FactionEditorWidget(
    std::shared_ptr<FactionViewModel> viewModel, QWidget* parent)
    : QWidget(parent)
    , m_viewModel(std::move(viewModel))
{
    CF_REQUIRE(m_viewModel != nullptr, "FactionViewModel must not be null");
    setupUi();
    setupConnections();
    clearEditor();
}

void FactionEditorWidget::loadFaction(FactionId id)
{
    const auto faction = m_viewModel->factionById(id);
    if (!faction.has_value()) return;
    m_currentFaction = faction;
    populateFromFaction(*m_currentFaction);
    setEnabled(true);
}

void FactionEditorWidget::clearEditor()
{
    m_currentFaction.reset();
    if (m_nameEdit)        m_nameEdit->clear();
    if (m_typeEdit)        m_typeEdit->clear();
    if (m_descriptionEdit) m_descriptionEdit->clear();
    if (m_evolutionsTable) m_evolutionsTable->setRowCount(0);
    if (m_dissolvedLabel)  m_dissolvedLabel->setText("Active");
    setEnabled(false);
}

void FactionEditorWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    auto* tabs = new QTabWidget(this);
    setupIdentityTab(tabs);
    setupEvolutionsTab(tabs);
    root->addWidget(tabs, 1);
}

void FactionEditorWidget::setupIdentityTab(QTabWidget* tabs)
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
    m_nameEdit->setPlaceholderText("Faction name");
    layout->addRow("Name:", m_nameEdit);

    m_typeEdit = new QLineEdit(widget);
    m_typeEdit->setPlaceholderText("e.g. Empire, Crew, Guild, Religion");
    layout->addRow("Type:", m_typeEdit);

    m_descriptionEdit = new QTextEdit(widget);
    m_descriptionEdit->setPlaceholderText("Description...");
    m_descriptionEdit->setMinimumHeight(80);
    layout->addRow("Description:", m_descriptionEdit);

    // Founded
    auto* foundedRow = new QHBoxLayout();
    m_foundedEraBox  = new QSpinBox(widget);
    m_foundedEraBox->setRange(1, 9999); m_foundedEraBox->setPrefix("Era ");
    m_foundedYearBox = new QSpinBox(widget);
    m_foundedYearBox->setRange(1, 99999); m_foundedYearBox->setPrefix("Year ");
    foundedRow->addWidget(m_foundedEraBox);
    foundedRow->addWidget(m_foundedYearBox);
    layout->addRow("Founded:", foundedRow);

    // Status
    m_dissolvedLabel = new QLabel("Active", widget);
    m_dissolvedLabel->setStyleSheet("color: #a6e3a1; font-weight: bold;");
    layout->addRow("Status:", m_dissolvedLabel);

    // Dissolve section
    auto* dissolveGroup = new QGroupBox("Dissolve faction", widget);
    auto* dgl = new QHBoxLayout(dissolveGroup);
    m_dissolveEraBox  = new QSpinBox(dissolveGroup);
    m_dissolveEraBox->setRange(1, 9999); m_dissolveEraBox->setPrefix("Era ");
    m_dissolveYearBox = new QSpinBox(dissolveGroup);
    m_dissolveYearBox->setRange(1, 99999); m_dissolveYearBox->setPrefix("Year ");
    m_dissolveButton  = new QPushButton("Mark as dissolved", dissolveGroup);
    m_dissolveButton->setObjectName("dangerButton");
    dgl->addWidget(new QLabel("When:", dissolveGroup));
    dgl->addWidget(m_dissolveEraBox);
    dgl->addWidget(m_dissolveYearBox);
    dgl->addWidget(m_dissolveButton);
    layout->addRow(dissolveGroup);

    m_saveButton = new QPushButton("Save changes", widget);
    m_saveButton->setObjectName("primaryButton");
    layout->addRow("", m_saveButton);

    tabs->addTab(scroll, "Identity");
}

void FactionEditorWidget::setupEvolutionsTab(QTabWidget* tabs)
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

    auto* addGroup   = new QGroupBox("Add evolution", widget);
    auto* addLayout  = new QHBoxLayout(addGroup);
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

void FactionEditorWidget::setupConnections()
{
    connect(m_saveButton,      &QPushButton::clicked,
            this, &FactionEditorWidget::onSaveClicked);
    connect(m_dissolveButton,  &QPushButton::clicked,
            this, &FactionEditorWidget::onDissolveClicked);
    connect(m_addEvoButton,    &QPushButton::clicked,
            this, &FactionEditorWidget::onAddEvolutionClicked);
    connect(m_removeEvoButton, &QPushButton::clicked,
            this, &FactionEditorWidget::onRemoveEvolutionClicked);

    connect(m_viewModel.get(), &FactionViewModel::factionUpdated,
            this, [this](qint64 id) {
                if (m_currentFaction && m_currentFaction->id.value() == id)
                    loadFaction(FactionId(id));
            });
}

void FactionEditorWidget::populateFromFaction(const Faction& f)
{
    m_nameEdit->setText(QString::fromStdString(f.name));
    m_typeEdit->setText(QString::fromStdString(f.type));
    m_descriptionEdit->setPlainText(QString::fromStdString(f.description));
    m_foundedEraBox->setValue(f.founded.era);
    m_foundedYearBox->setValue(f.founded.year);

    if (f.dissolved.has_value()) {
        m_dissolvedLabel->setText(
            QString("Dissolved — %1")
            .arg(QString::fromStdString(f.dissolved->display())));
        m_dissolvedLabel->setStyleSheet("color: #f38ba8; font-weight: bold;");
        m_dissolveButton->setEnabled(false);
        m_dissolveButton->setText("Already dissolved");
    } else {
        m_dissolvedLabel->setText("Active");
        m_dissolvedLabel->setStyleSheet("color: #a6e3a1; font-weight: bold;");
        m_dissolveButton->setEnabled(true);
        m_dissolveButton->setText("Mark as dissolved");
    }

    populateEvolutionsTable(f);
}

void FactionEditorWidget::populateEvolutionsTable(const Faction& f)
{
    m_evolutionsTable->setRowCount(0);
    for (const auto& evo : f.evolutions) {
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

void FactionEditorWidget::onSaveClicked()
{
    if (!m_currentFaction.has_value()) return;
    Faction updated = buildFactionFromForm();
    m_viewModel->updateFaction(updated);
    emit factionSaved(updated.id);
}

void FactionEditorWidget::onDissolveClicked()
{
    if (!m_currentFaction.has_value()) return;
    const auto reply = QMessageBox::question(
        this, "Dissolve faction",
        QString("Mark \"%1\" as dissolved? This action records the end "
                "of the faction in the timeline.")
            .arg(QString::fromStdString(m_currentFaction->name)),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    m_viewModel->dissolveFaction(
        m_currentFaction->id,
        m_dissolveEraBox->value(),
        m_dissolveYearBox->value());
}

void FactionEditorWidget::onAddEvolutionClicked()
{
    if (!m_currentFaction.has_value()) return;
    if (m_evoDescEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation",
                             "Evolution description is required.");
        return;
    }
    m_viewModel->addEvolution(
        m_currentFaction->id,
        m_evoEraBox->value(),
        m_evoYearBox->value(),
        m_evoDescEdit->text().trimmed());
    m_evoDescEdit->clear();
}

void FactionEditorWidget::onRemoveEvolutionClicked()
{
    if (!m_currentFaction.has_value()) return;
    const int row = m_evolutionsTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_currentFaction->evolutions.size()))
        return;
    const auto& evo = m_currentFaction->evolutions[row];
    m_viewModel->removeEvolution(
        m_currentFaction->id, evo.at.era, evo.at.year);
}

Faction FactionEditorWidget::buildFactionFromForm() const
{
    CF_ASSERT(m_currentFaction.has_value(), "No faction loaded in editor");
    Faction f       = *m_currentFaction;
    f.name          = m_nameEdit->text().trimmed().toStdString();
    f.type          = m_typeEdit->text().trimmed().toStdString();
    f.description   = m_descriptionEdit->toPlainText().toStdString();
    f.founded       = { m_foundedEraBox->value(), m_foundedYearBox->value() };
    return f;
}

} // namespace CF::UI