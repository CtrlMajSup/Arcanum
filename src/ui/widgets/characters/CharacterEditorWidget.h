#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <optional>
#include <memory>

#include "ui/viewmodels/CharacterViewModel.h"
#include "ui/viewmodels/PlaceViewModel.h"
#include "ui/viewmodels/FactionViewModel.h"
#include "domain/entities/Character.h"

namespace CF::UI {

/**
 * CharacterEditorWidget is a multi-tab editor for a single character.
 *
 * Tabs:
 *  1. Identity   — name, species, born/died, portrait, biography
 *  2. Timeline   — evolutions list with add/remove
 *  3. Locations  — location history with add/move
 *  4. Factions   — memberships with join/leave
 *  5. Relations  — relationships with other characters
 *
 * It holds the currently edited character and pushes changes
 * to the ViewModel on explicit Save or auto-save actions.
 * It never touches the service directly.
 */
class CharacterEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit CharacterEditorWidget(
        std::shared_ptr<CharacterViewModel> viewModel,
        std::shared_ptr<PlaceViewModel>     placeViewModel,
        std::shared_ptr<FactionViewModel>   factionViewModel,
        QWidget* parent = nullptr);

    // Load a character into the editor — called when list selection changes
    void loadCharacter(Domain::CharacterId id);

    // Clear the editor (no character selected)
    void clearEditor();

signals:
    void characterSaved(Domain::CharacterId id);
    void navigateToCharacter(Domain::CharacterId id);  // NEW
    void navigateToPlace(Domain::PlaceId id);          // NEW

private slots:
    void onSaveClicked();
    void onAddEvolutionClicked();
    void onRemoveEvolutionClicked();
    void onAddLocationClicked();
    void onAddFactionClicked();
    void onLeaveFactiomClicked();
    void onAddRelationshipClicked();
    void onRemoveRelationshipClicked();

private:
    void setupUi();
    void setupIdentityTab(QTabWidget* tabs);
    void setupEvolutionsTab(QTabWidget* tabs);
    void setupLocationsTab(QTabWidget* tabs);
    void setupFactionsTab(QTabWidget* tabs);
    void setupRelationsTab(QTabWidget* tabs);
    void setupConnections();

    void populateFromCharacter(const Domain::Character& c);
    void populateEvolutionsTable(const Domain::Character& c);
    void populateLocationsTable(const Domain::Character& c);
    void populateFactionsTable(const Domain::Character& c);
    void populateRelationsTable(const Domain::Character& c);

    [[nodiscard]] Domain::Character buildCharacterFromForm() const;

    std::shared_ptr<CharacterViewModel>  m_viewModel;
    std::shared_ptr<PlaceViewModel>     m_placeViewModel;
    std::shared_ptr<FactionViewModel>   m_factionViewModel;
    std::optional<Domain::Character>     m_currentCharacter;

    // ── Identity tab ──────────────────────────────────────────────────────────
    QLineEdit*  m_nameEdit        = nullptr;
    QLineEdit*  m_speciesEdit     = nullptr;
    QSpinBox*   m_bornEraBox      = nullptr;
    QSpinBox*   m_bornYearBox     = nullptr;
    QCheckBox*  m_isDeadCheck     = nullptr;
    QSpinBox*   m_diedEraBox      = nullptr;
    QSpinBox*   m_diedYearBox     = nullptr;
    QTextEdit*  m_biographyEdit   = nullptr;
    QPushButton* m_saveButton     = nullptr;

    // ── Evolutions tab ────────────────────────────────────────────────────────
    QTableWidget* m_evolutionsTable  = nullptr;
    QSpinBox*     m_evoEraBox        = nullptr;
    QSpinBox*     m_evoYearBox       = nullptr;
    QLineEdit*    m_evoDescEdit      = nullptr;
    QPushButton*  m_addEvoButton     = nullptr;
    QPushButton*  m_removeEvoButton  = nullptr;

    // ── Locations tab ─────────────────────────────────────────────────────────
    QTableWidget* m_locationsTable   = nullptr;
    QSpinBox*     m_locEraBox        = nullptr;
    QSpinBox*     m_locYearBox       = nullptr;
    QLineEdit*    m_locNoteEdit      = nullptr;
    QPushButton*  m_addLocButton     = nullptr;

    // ── Factions tab ─────────────────────────────────────────────────────────
    QTableWidget* m_factionsTable    = nullptr;
    QSpinBox*     m_facEraBox        = nullptr;
    QSpinBox*     m_facYearBox       = nullptr;
    QLineEdit*    m_facRoleEdit      = nullptr;
    QPushButton*  m_joinFacButton    = nullptr;
    QPushButton*  m_leaveFacButton   = nullptr;

    // ── Relations tab ─────────────────────────────────────────────────────────
    QTableWidget* m_relationsTable   = nullptr;
    QComboBox*    m_relTypeCombo     = nullptr;
    QLineEdit*    m_relTargetEdit    = nullptr;
    QSpinBox*     m_relEraBox        = nullptr;
    QSpinBox*     m_relYearBox       = nullptr;
    QLineEdit*    m_relNoteEdit      = nullptr;
    QPushButton*  m_addRelButton     = nullptr;
    QPushButton*  m_removeRelButton  = nullptr;
};

} // namespace CF::UI