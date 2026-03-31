#pragma once

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <optional>
#include <memory>

#include "services/CharacterService.h"
#include "domain/entities/Character.h"
#include "ui/viewmodels/CharacterViewModel.h"

namespace CF::UI {

/**
 * CharacterPickerDialog lets the user pick a Character from a list.
 * Used by CharacterEditorWidget when adding a relationship target.
 * Excludes the owner character from the list.
 */
class CharacterPickerDialog : public QDialog {
    Q_OBJECT

public:
    explicit CharacterPickerDialog(
        std::shared_ptr<CharacterViewModel> characterViewModel,
        Domain::CharacterId excludeId,
        QWidget* parent = nullptr);

    [[nodiscard]] std::optional<Domain::CharacterId> selectedCharacterId() const;

private slots:
    void onSearchChanged(const QString& text);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onAcceptClicked();

private:
    void setupUi();
    void setupConnections();
    void loadCharacters(const QString& filter = {});

    std::shared_ptr<Services::CharacterService> m_characterService;
    Domain::CharacterId                          m_excludeId;
    std::optional<Domain::CharacterId>           m_selectedId;
    std::shared_ptr<CharacterViewModel> m_characterViewModel;

    QLineEdit*   m_searchEdit = nullptr;
    QListWidget* m_list       = nullptr;
    QPushButton* m_okBtn      = nullptr;
    QPushButton* m_cancelBtn  = nullptr;
};

} // namespace CF::UI