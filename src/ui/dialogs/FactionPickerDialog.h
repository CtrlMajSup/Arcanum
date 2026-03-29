#pragma once

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <optional>
#include <memory>

#include "services/FactionService.h"
#include "domain/entities/Faction.h"

namespace CF::UI {

/**
 * FactionPickerDialog lets the user pick a Faction from a searchable list.
 * Used by CharacterEditorWidget when joining a faction.
 */
class FactionPickerDialog : public QDialog {
    Q_OBJECT

public:
    explicit FactionPickerDialog(
        std::shared_ptr<Services::FactionService> factionService,
        QWidget* parent = nullptr);

    [[nodiscard]] std::optional<Domain::FactionId> selectedFactionId() const;

private slots:
    void onSearchChanged(const QString& text);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onAcceptClicked();

private:
    void setupUi();
    void setupConnections();
    void loadFactions(const QString& filter = {});

    std::shared_ptr<Services::FactionService> m_factionService;
    std::optional<Domain::FactionId>          m_selectedId;

    QLineEdit*   m_searchEdit = nullptr;
    QListWidget* m_list       = nullptr;
    QPushButton* m_okBtn      = nullptr;
    QPushButton* m_cancelBtn  = nullptr;
};

} // namespace CF::UI