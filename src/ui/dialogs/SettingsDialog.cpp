#include "SettingsDialog.h"

#include <QHBoxLayout>

namespace CF::UI {

SettingsDialog::SettingsDialog(const Settings& current, QWidget* parent)
    : QDialog(parent)
    , m_current(current)
{
    setWindowTitle("Settings — ChronicleForge");
    setFixedSize(500, 380);
    setModal(true);
    setupUi();
    setupConnections();
    populate(current);
}

SettingsDialog::Settings SettingsDialog::settings() const
{
    Settings s;
    s.autosaveIntervalMs = m_autosaveBox->value()     * 1000;
    s.debugMode          = m_debugCheck->isChecked();
    s.editorFontSize     = m_fontSizeBox->value();
    s.previewDelayMs     = m_previewDelayBox->value() * 1000;
    return s;
}

void SettingsDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 16);
    root->setSpacing(0);

    auto* tabs = new QTabWidget(this);
    setupGeneralTab(tabs);
    setupAppearanceTab(tabs);
    setupDatabaseTab(tabs);
    root->addWidget(tabs, 1);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(16, 0, 16, 0);
    m_cancelBtn = new QPushButton("Cancel", this);
    m_okBtn     = new QPushButton("Apply", this);
    m_okBtn->setObjectName("primaryButton");
    m_okBtn->setDefault(true);
    btnRow->addStretch();
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_okBtn);
    root->addLayout(btnRow);
}

void SettingsDialog::setupGeneralTab(QTabWidget* tabs)
{
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);

    auto* autosaveLabel = new QLabel("General", widget);
    autosaveLabel->setObjectName("dialogSectionLabel");
    layout->addRow(autosaveLabel);

    m_autosaveBox = new QSpinBox(widget);
    m_autosaveBox->setRange(1, 60);
    m_autosaveBox->setSuffix(" seconds");
    m_autosaveBox->setToolTip("How long to wait after the last keystroke "
                               "before autosaving a chapter.");
    layout->addRow("Autosave delay:", m_autosaveBox);

    m_previewDelayBox = new QSpinBox(widget);
    m_previewDelayBox->setRange(1, 10);
    m_previewDelayBox->setSuffix(" seconds");
    m_previewDelayBox->setToolTip("How long to wait before refreshing "
                                   "the Markdown preview.");
    layout->addRow("Preview refresh:", m_previewDelayBox);

    auto* devLabel = new QLabel("Developer", widget);
    devLabel->setObjectName("dialogSectionLabel");
    layout->addRow(devLabel);

    m_debugCheck = new QCheckBox("Enable debug logging", widget);
    m_debugCheck->setToolTip("Outputs verbose logs to the console. "
                              "Restart required.");
    layout->addRow("Debug mode:", m_debugCheck);

    tabs->addTab(widget, "General");
}

void SettingsDialog::setupAppearanceTab(QTabWidget* tabs)
{
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);

    auto* editorLabel = new QLabel("Editor", widget);
    editorLabel->setObjectName("dialogSectionLabel");
    layout->addRow(editorLabel);

    m_fontSizeBox = new QSpinBox(widget);
    m_fontSizeBox->setRange(10, 24);
    m_fontSizeBox->setSuffix(" px");
    layout->addRow("Editor font size:", m_fontSizeBox);

    auto* note = new QLabel(
        "Full theme customisation via resources/styles/main.qss", widget);
    note->setObjectName("countLabel");
    note->setWordWrap(true);
    layout->addRow(note);

    tabs->addTab(widget, "Appearance");
}

void SettingsDialog::setupDatabaseTab(QTabWidget* tabs)
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);

    auto* dbLabel = new QLabel("Database", widget);
    dbLabel->setObjectName("dialogSectionLabel");
    layout->addWidget(dbLabel);

    auto* pathRow = new QHBoxLayout();
    pathRow->addWidget(new QLabel("Current file:", widget));
    m_dbPathLabel = new QLabel(widget);
    m_dbPathLabel->setObjectName("countLabel");
    m_dbPathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_dbPathLabel->setWordWrap(true);
    pathRow->addWidget(m_dbPathLabel, 1);
    layout->addLayout(pathRow);

    auto* maintenanceLabel = new QLabel("Maintenance", widget);
    maintenanceLabel->setObjectName("dialogSectionLabel");
    layout->addWidget(maintenanceLabel);

    auto* vacuumNote = new QLabel(
        "VACUUM reclaims unused space and defragments the database.", widget);
    vacuumNote->setObjectName("countLabel");
    vacuumNote->setWordWrap(true);
    layout->addWidget(vacuumNote);

    m_vacuumBtn = new QPushButton("Run VACUUM", widget);
    m_vacuumBtn->setFixedWidth(140);
    layout->addWidget(m_vacuumBtn);

    auto* backupNote = new QLabel(
        "Create a timestamped copy of the database file.", widget);
    backupNote->setObjectName("countLabel");
    backupNote->setWordWrap(true);
    layout->addWidget(backupNote);

    m_backupBtn = new QPushButton("Create backup…", widget);
    m_backupBtn->setFixedWidth(140);
    layout->addWidget(m_backupBtn);

    layout->addStretch();
    tabs->addTab(widget, "Database");
}

void SettingsDialog::setupConnections()
{
    connect(m_okBtn,     &QPushButton::clicked,
            this, &SettingsDialog::onAcceptClicked);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &QDialog::reject);
    connect(m_vacuumBtn, &QPushButton::clicked,
            this, &SettingsDialog::onVacuumClicked);
    connect(m_backupBtn, &QPushButton::clicked,
            this, &SettingsDialog::onBackupClicked);
}

void SettingsDialog::populate(const Settings& s)
{
    m_autosaveBox->setValue(s.autosaveIntervalMs / 1000);
    m_previewDelayBox->setValue(s.previewDelayMs / 1000);
    m_debugCheck->setChecked(s.debugMode);
    m_fontSizeBox->setValue(s.editorFontSize);
}

void SettingsDialog::onAcceptClicked()
{
    accept();
}

void SettingsDialog::onVacuumClicked()
{
    emit vacuumRequested();
}

void SettingsDialog::onBackupClicked()
{
    emit backupRequested();
}

} // namespace CF::UI