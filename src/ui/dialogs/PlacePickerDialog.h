#pragma once

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <optional>
#include <memory>

#include "services/PlaceService.h"
#include "domain/entities/Place.h"
#include "ui/widgets/shared/EntityBadgeWidget.h"

namespace CF::UI {

/**
 * PlacePickerDialog lets the user pick a Place from a searchable list.
 * Used by CharacterEditorWidget when moving a character to a new location.
 *
 * Returns the selected PlaceId via selectedPlaceId().
 */
class PlacePickerDialog : public QDialog {
    Q_OBJECT

public:
    explicit PlacePickerDialog(
        std::shared_ptr<Services::PlaceService> placeService,
        QWidget* parent = nullptr);

    [[nodiscard]] std::optional<Domain::PlaceId> selectedPlaceId() const;

private slots:
    void onSearchChanged(const QString& text);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onAcceptClicked();

private:
    void setupUi();
    void setupConnections();
    void loadPlaces(const QString& filter = {});

    std::shared_ptr<Services::PlaceService> m_placeService;
    std::optional<Domain::PlaceId>          m_selectedId;

    QLineEdit*   m_searchEdit = nullptr;
    QListWidget* m_list       = nullptr;
    QPushButton* m_okBtn      = nullptr;
    QPushButton* m_cancelBtn  = nullptr;
};

} // namespace CF::UI