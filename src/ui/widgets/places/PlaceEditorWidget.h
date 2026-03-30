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
#include <QFormLayout>
#include <optional>
#include <memory>

#include "ui/viewmodels/PlaceViewModel.h"
#include "domain/entities/Place.h"

namespace CF::UI {

/**
 * PlaceEditorWidget — two-tab editor for a single place.
 *
 * Tab 1 — Identity : name, type, region, mobile flag,
 *                    description, map position
 * Tab 2 — Timeline : evolutions list with add/remove
 */
class PlaceEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit PlaceEditorWidget(
        std::shared_ptr<PlaceViewModel> viewModel,
        QWidget* parent = nullptr);

    void loadPlace(Domain::PlaceId id);
    void clearEditor();

signals:
    void placeSaved(Domain::PlaceId id);

private slots:
    void onSaveClicked();
    void onAddEvolutionClicked();
    void onRemoveEvolutionClicked();

private:
    void setupUi();
    void setupIdentityTab(QTabWidget* tabs);
    void setupEvolutionsTab(QTabWidget* tabs);
    void setupConnections();

    void populateFromPlace(const Domain::Place& p);
    void populateEvolutionsTable(const Domain::Place& p);

    [[nodiscard]] Domain::Place buildPlaceFromForm() const;

    std::shared_ptr<PlaceViewModel>  m_viewModel;
    std::optional<Domain::Place>     m_currentPlace;

    // Identity tab
    QLineEdit*   m_nameEdit        = nullptr;
    QLineEdit*   m_typeEdit        = nullptr;
    QLineEdit*   m_regionEdit      = nullptr;
    QCheckBox*   m_mobileCheck     = nullptr;
    QTextEdit*   m_descriptionEdit = nullptr;
    QSpinBox*    m_mapXBox         = nullptr;  // 0–100 (%)
    QSpinBox*    m_mapYBox         = nullptr;
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