#pragma once

#include <QFrame>
#include <QListWidget>
#include <QVBoxLayout>
#include <vector>
#include <functional>

#include "services/AutoCompleteService.h"

namespace CF::UI {

/**
 * AutoCompletePopup is a frameless floating list shown below the cursor
 * when the user types '@' in the chapter editor.
 *
 * It is parented to the editor widget and positioned dynamically.
 * Accepting a suggestion calls the provided callback with the
 * insertion text (e.g. "@[Kael|character:1]").
 */
class AutoCompletePopup : public QFrame {
    Q_OBJECT

public:
    using AcceptCallback = std::function<void(const QString& insertionText)>;

    explicit AutoCompletePopup(QWidget* parent = nullptr);

    // Populates the list with new suggestions
    void setSuggestions(
        const std::vector<Services::AutoCompleteSuggestion>& suggestions);

    // Registers the callback invoked when user picks a suggestion
    void setAcceptCallback(AcceptCallback cb);

    // Keyboard navigation — called by the editor's keyPressEvent
    void navigateUp();
    void navigateDown();
    void acceptCurrent();

    [[nodiscard]] bool hasSuggestions() const;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onItemActivated(QListWidgetItem* item);

private:
    QListWidget* m_list      = nullptr;
    AcceptCallback m_callback;

    std::vector<Services::AutoCompleteSuggestion> m_suggestions;

    static QString typeLabel(Services::AutoCompleteSuggestion::EntityType t);
    static QColor  typeColour(Services::AutoCompleteSuggestion::EntityType t);
};

} // namespace CF::UI