#include "NewProjectDialog.h"

#include <QHBoxLayout>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>

namespace CF::UI {

NewProjectDialog::NewProjectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("New Project — ChronicleForge");
    setFixedSize(480, 300);
    setModal(true);
    setupUi();
    setupConnections();
}

QString NewProjectDialog::projectName() const
{
    return m_nameEdit->text().trimmed();
}

QString NewProjectDialog::databasePath() const
{
    return m_pathEdit->text().trimmed();
}

void NewProjectDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 16);
    root->setSpacing(16);

    // Title
    auto* title = new QLabel("Create a new encyclopedia", this);
    title->setObjectName("sectionTitle");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto* subtitle = new QLabel(
        "Your world's data will be stored in a local SQLite file.", this);
    subtitle->setObjectName("countLabel");
    subtitle->setAlignment(Qt::AlignCenter);
    root->addWidget(subtitle);

    // Form
    auto* form = new QFormLayout();
    form->setSpacing(10);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("e.g. The Shattered Realm");
    form->addRow("Project name:", m_nameEdit);

    // DB path row
    auto* pathRow = new QHBoxLayout();
    m_pathEdit = new QLineEdit(this);

    // Default path: user documents folder
    const QString defaultPath =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        + "/ChronicleForge/project.db";
    m_pathEdit->setText(QDir::toNativeSeparators(defaultPath));

    m_browseBtn = new QPushButton("Browse…", this);
    m_browseBtn->setFixedWidth(80);
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(m_browseBtn);
    form->addRow("Database file:", pathRow);

    root->addLayout(form);

    // Error label (hidden until needed)
    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet("color: #f38ba8; font-size: 12px;");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->hide();
    root->addWidget(m_errorLabel);

    root->addStretch();

    // Buttons
    auto* btnRow = new QHBoxLayout();
    m_cancelBtn = new QPushButton("Cancel", this);
    m_createBtn = new QPushButton("Create project", this);
    m_createBtn->setObjectName("primaryButton");
    m_createBtn->setDefault(true);
    btnRow->addStretch();
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_createBtn);
    root->addLayout(btnRow);
}

void NewProjectDialog::setupConnections()
{
    connect(m_browseBtn,  &QPushButton::clicked,
            this, &NewProjectDialog::onBrowseClicked);
    connect(m_createBtn,  &QPushButton::clicked,
            this, &NewProjectDialog::onAcceptClicked);
    connect(m_cancelBtn,  &QPushButton::clicked,
            this, &QDialog::reject);
}

void NewProjectDialog::onBrowseClicked()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        "Choose database location",
        m_pathEdit->text(),
        "SQLite Database (*.db);;All files (*)"
    );
    if (!path.isEmpty()) {
        m_pathEdit->setText(QDir::toNativeSeparators(path));
    }
}

void NewProjectDialog::onAcceptClicked()
{
    if (!validate()) return;

    // Ensure parent directory exists
    const QFileInfo fi(m_pathEdit->text().trimmed());
    QDir().mkpath(fi.absolutePath());

    accept();
}

bool NewProjectDialog::validate() const
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        m_errorLabel->setText("Project name cannot be empty.");
        m_errorLabel->show();
        m_nameEdit->setFocus();
        return false;
    }
    if (m_pathEdit->text().trimmed().isEmpty()) {
        m_errorLabel->setText("Database path cannot be empty.");
        m_errorLabel->show();
        return false;
    }
    m_errorLabel->hide();
    return true;
}

} // namespace CF::UI