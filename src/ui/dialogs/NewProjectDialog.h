#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QString>

namespace CF::UI {

/**
 * NewProjectDialog is shown at first launch when no database exists.
 * It collects a project name and optionally a custom DB path.
 *
 * On accept it sets the chosen values which App::initialize() uses
 * to create the database at the right location.
 */
class NewProjectDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget* parent = nullptr);

    [[nodiscard]] QString projectName()   const;
    [[nodiscard]] QString databasePath()  const;

private slots:
    void onBrowseClicked();
    void onAcceptClicked();

private:
    void setupUi();
    void setupConnections();
    [[nodiscard]] bool validate() const;

    QLineEdit*   m_nameEdit    = nullptr;
    QLineEdit*   m_pathEdit    = nullptr;
    QPushButton* m_browseBtn   = nullptr;
    QPushButton* m_createBtn   = nullptr;
    QPushButton* m_cancelBtn   = nullptr;
    QLabel*      m_errorLabel  = nullptr;
};

} // namespace CF::UI