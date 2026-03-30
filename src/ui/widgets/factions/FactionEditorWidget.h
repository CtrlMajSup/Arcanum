#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <optional>
#include <memory>

#include "ui/viewmodels/FactionViewModel.h"
#include "domain/entities/Faction.h"

namespace CF::UI {

/**
 * FactionEditorWidget — two-tab editor for a single faction.
 *
 * Tab 1 — Identity  : name, type, description, founded, dissolved
 * Tab 2 — Timeline  : evolutions list with add/remove
 */
class FactionEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit FactionEditorWidget(
        std::shared_ptr<FactionViewModel> viewModel,
        QWidget* parent = nullptr);

    void loadFaction(Domain::FactionId id);
    void clearEditor();

signals:
    void factionSaved(Domain::FactionId id);

private slots:
    void onSaveClicked();
    void onDissolveClicked();
    void onAddEvolutionClicked();
    void onRemoveEvolutionClicked();

private:
    void setupUi();
    void setupIdentityTab(QTabWidget* tabs);
    void setupEvolutionsTab(QTabWidget* tabs);
    void setupConnections();

    void populateFromFaction(const Domain::Faction& f);
    void populateEvolutionsTable(const Domain::Faction& f);

    [[nodiscard]] Domain::Faction buildFactionFromForm() const;

    std::shared_ptr<FactionViewModel> m_viewModel;
    std::optional<Domain::Faction>    m_currentFaction;

    // Identity tab
    QLineEdit*   m_nameEdit        = nullptr;
    QLineEdit*   m_typeEdit        = nullptr;
    QTextEdit*   m_descriptionEdit = nullptr;
    QSpinBox*    m_foundedEraBox   = nullptr;
    QSpinBox*    m_foundedYearBox  = nullptr;
    QLabel*      m_dissolvedLabel  = nullptr;
    QSpinBox*    m_dissolveEraBox  = nullptr;
    QSpinBox*    m_dissolveYearBox = nullptr;
    QPushButton* m_dissolveButton  = nullptr;
    QPushButton* m_saveButton      = nullptr;

    // Evolutions tab
    QTableWidget* m_evolutionsTable = nullptr;
    QSpinBox*     m_evoEraBox       = nullptr;
    QSpinBox*     m_evoYearBox      = nullptr;
    QLineEdit*    m_evoDescEdit     = nullptr;
    QPushButton*  m_addEvoButton    = nullptr;
    QPushButton*  m_removeEvoButton = nullptr;
};

} // namespace CF::UI