#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>

namespace CF::UI {

/**
 * SettingsDialog provides application-level configuration.
 *
 * Tabs:
 *  1. General   — autosave interval, debug mode toggle
 *  2. Appearance — theme (future), font size
 *  3. Database  — current DB path, backup, vacuum
 */
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    struct Settings {
        int  autosaveIntervalMs = 1500;
        bool debugMode          = false;
        int  editorFontSize     = 14;
        int  previewDelayMs     = 500;
    };

    explicit SettingsDialog(const Settings& current,
                            QWidget* parent = nullptr);

    [[nodiscard]] Settings settings() const;

signals:
    void vacuumRequested();
    void backupRequested();

private slots:
    void onAcceptClicked();
    void onVacuumClicked();
    void onBackupClicked();

private:
    void setupUi();
    void setupGeneralTab(QTabWidget* tabs);
    void setupAppearanceTab(QTabWidget* tabs);
    void setupDatabaseTab(QTabWidget* tabs);
    void setupConnections();
    void populate(const Settings& s);

    // General
    QSpinBox*  m_autosaveBox    = nullptr;
    QCheckBox* m_debugCheck     = nullptr;
    QSpinBox*  m_previewDelayBox= nullptr;

    // Appearance
    QSpinBox*  m_fontSizeBox    = nullptr;

    // Database
    QLabel*    m_dbPathLabel    = nullptr;
    QPushButton* m_vacuumBtn    = nullptr;
    QPushButton* m_backupBtn    = nullptr;

    QPushButton* m_okBtn        = nullptr;
    QPushButton* m_cancelBtn    = nullptr;

    Settings m_current;
};

} // namespace CF::UI