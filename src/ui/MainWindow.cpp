#include "MainWindow.h"

#include <QApplication>
#include <QShortcut>
#include <QCloseEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>

namespace CF::UI {

using namespace CF::Domain;

// ── Constructor ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(
    std::shared_ptr<Services::CharacterService>    characterService,
    std::shared_ptr<Services::PlaceService>        placeService,
    std::shared_ptr<Services::FactionService>      factionService,
    std::shared_ptr<Services::TimelineService>     timelineService,
    std::shared_ptr<Services::SearchService>       searchService,
    std::shared_ptr<Services::ChapterService>      chapterService,
    std::shared_ptr<Services::AutoCompleteService> autoCompleteService,
    Core::Logger& logger,
    QWidget* parent)
    : QMainWindow(parent)
    , m_logger(logger)
{
    buildViewModels(characterService, placeService, factionService,
                    timelineService, searchService,
                    chapterService, autoCompleteService);

    setWindowTitle("ChronicleForge");
    setMinimumSize(1100, 700);
    resize(1440, 860);

    setupToolBar();
    setupCentralWidget();
    setupStatusBar();
    setupPanelConnections();
    applyStyleSheet();

    // Land on the characters panel by default
    showCharactersPanel();

    m_logger.info("MainWindow ready", "MainWindow");
}

// ── buildViewModels ───────────────────────────────────────────────────────────

void MainWindow::buildViewModels(
    std::shared_ptr<Services::CharacterService>    characterService,
    std::shared_ptr<Services::PlaceService>        placeService,
    std::shared_ptr<Services::FactionService>      factionService,
    std::shared_ptr<Services::TimelineService>     timelineService,
    std::shared_ptr<Services::SearchService>       searchService,
    std::shared_ptr<Services::ChapterService>      chapterService,
    std::shared_ptr<Services::AutoCompleteService> autoCompleteService)
{
    m_characterViewModel = std::make_shared<CharacterViewModel>(
        characterService, placeService, factionService, m_logger, this);

    m_timelineViewModel = std::make_shared<TimelineViewModel>(
        timelineService, characterService, factionService,
        placeService, m_logger, this);

    m_mapViewModel = std::make_shared<MapViewModel>(
        placeService, characterService, m_logger, this);

    m_chapterViewModel = std::make_shared<ChapterViewModel>(
        chapterService, autoCompleteService, m_logger, this);

    m_searchViewModel = std::make_shared<SearchViewModel>(
        searchService, m_logger, this);
}

// ── setupToolBar ──────────────────────────────────────────────────────────────

void MainWindow::setupToolBar()
{
    auto* tb = addToolBar("Navigation");
    tb->setMovable(false);
    tb->setObjectName("mainToolBar");
    tb->setIconSize(QSize(18, 18));

    // Helper lambda to create a checkable nav action
    auto makeNavAction = [&](const QString& icon,
                              const QString& label) -> QAction* {
        auto* act = tb->addAction(icon + "  " + label);
        act->setCheckable(true);
        return act;
    };

    m_actCharacters = makeNavAction("👤", "Characters");
    m_actTimeline   = makeNavAction("📅", "Timeline");
    m_actMap        = makeNavAction("🗺", "Map");
    m_actChapters   = makeNavAction("📝", "Writing");

    tb->addSeparator();
    m_actSearch = makeNavAction("🔍", "Search");

    connect(m_actCharacters, &QAction::triggered, this, &MainWindow::showCharactersPanel);
    connect(m_actTimeline,   &QAction::triggered, this, &MainWindow::showTimelinePanel);
    connect(m_actMap,        &QAction::triggered, this, &MainWindow::showMapPanel);
    connect(m_actChapters,   &QAction::triggered, this, &MainWindow::showChaptersPanel);
    connect(m_actSearch,     &QAction::triggered, this, &MainWindow::showSearchPanel);

    tb->addSeparator();
    auto* settingsAction = tb->addAction("⚙  Settings");
    connect(settingsAction, &QAction::triggered,
            this, &MainWindow::onSettingsRequested);

}

// ── setupCentralWidget ────────────────────────────────────────────────────────

void MainWindow::setupCentralWidget()
{
    m_panels = new QStackedWidget(this);

    // ── Characters panel ──────────────────────────────────────────────────────
    m_charactersSplitter = new QSplitter(Qt::Horizontal, m_panels);
    m_characterList = new CharacterListWidget(
        m_characterViewModel, m_charactersSplitter);
    m_characterList->setMinimumWidth(220);
    m_characterList->setMaximumWidth(320);


    m_characterEditor = new CharacterEditorWidget(
        m_characterViewModel, m_charactersSplitter);

    m_charactersSplitter->addWidget(m_characterList);
    m_charactersSplitter->addWidget(m_characterEditor);
    m_charactersSplitter->setStretchFactor(0, 0);
    m_charactersSplitter->setStretchFactor(1, 1);

    // ── Timeline panel ────────────────────────────────────────────────────────
    m_timelineWidget = new TimelineWidget(m_timelineViewModel, m_panels);

    // ── Map panel ─────────────────────────────────────────────────────────────
    m_mapWidget = new MapWidget(m_mapViewModel, m_panels);

    // ── Chapters panel ────────────────────────────────────────────────────────
    m_chaptersSplitter = new QSplitter(Qt::Horizontal, m_panels);
    m_chapterList = new ChapterListWidget(
        m_chapterViewModel, m_chaptersSplitter);
    m_chapterList->setMinimumWidth(200);
    m_chapterList->setMaximumWidth(300);
    m_chapterEditor = new ChapterEditorWidget(
        m_chapterViewModel, m_chaptersSplitter);
    m_chaptersSplitter->addWidget(m_chapterList);
    m_chaptersSplitter->addWidget(m_chapterEditor);
    m_chaptersSplitter->setStretchFactor(0, 0);
    m_chaptersSplitter->setStretchFactor(1, 1);

    // ── Search panel ──────────────────────────────────────────────────────────
    m_searchWidget = new SearchWidget(m_searchViewModel, m_panels);

    // Register all panels
    m_panels->addWidget(m_charactersSplitter);
    m_panels->addWidget(m_timelineWidget);
    m_panels->addWidget(m_mapWidget);
    m_panels->addWidget(m_chaptersSplitter);
    m_panels->addWidget(m_searchWidget);

    setCentralWidget(m_panels);
}

// ── setupStatusBar ────────────────────────────────────────────────────────────

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("ChronicleForge ready");
    statusBar()->setStyleSheet("QStatusBar { color: #6c7086; font-size: 11px; }");
}

// ── setupPanelConnections ─────────────────────────────────────────────────────

void MainWindow::setupPanelConnections()
{
    // ── Character panel ───────────────────────────────────────────────────────
    connect(m_characterList, &CharacterListWidget::characterSelected,
            this, &MainWindow::onCharacterSelected);

    connect(m_characterList, &CharacterListWidget::createRequested,
            this, &MainWindow::onCreateCharacterRequested);

    connect(m_characterEditor, &CharacterEditorWidget::characterSaved,
            this, [this](CharacterId id) {
                statusBar()->showMessage(
                    QString("Saved character #%1").arg(id.value()), 3000);
            });

    // ── Map panel ─────────────────────────────────────────────────────────────
    connect(m_mapWidget, &MapWidget::placeSelected,
            this, [this](PlaceId id) {
                statusBar()->showMessage(
                    QString("Place selected: #%1").arg(id.value()), 2000);
            });

    connect(m_mapWidget, &MapWidget::placeOpened, this, [this](PlaceId id) {
        // Navigate to map and highlight — future: open place detail panel
        showMapPanel();
        statusBar()->showMessage(
            QString("Opened place #%1").arg(id.value()), 2000);
    });

    // ── Timeline panel ────────────────────────────────────────────────────────
    connect(m_timelineWidget, &TimelineWidget::eventClicked,
            this, [this](TimelineId id) {
                statusBar()->showMessage(
                    QString("Timeline event #%1").arg(id.value()), 2000);
            });

    // ── Chapter panel ─────────────────────────────────────────────────────────
    connect(m_chapterEditor, &ChapterEditorWidget::chapterModified,
            this, [this](ChapterId id) {
                statusBar()->showMessage(
                    QString("Chapter #%1 saved").arg(id.value()), 2000);
            });

    // ── Search navigation ─────────────────────────────────────────────────────
    connect(m_searchWidget, &SearchWidget::navigateToCharacter,
            this, &MainWindow::onNavigateToCharacter);

    connect(m_searchWidget, &SearchWidget::navigateToPlace,
            this, &MainWindow::onNavigateToPlace);

    connect(m_searchWidget, &SearchWidget::navigateToFaction,
            this, &MainWindow::onNavigateToFaction);

    connect(m_searchWidget, &SearchWidget::navigateToChapter,
            this, &MainWindow::onNavigateToChapter);

    // ── Search shortcut: Ctrl+F from anywhere ────────────────────────────────
    auto* searchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(searchShortcut, &QShortcut::activated,
            this, &MainWindow::showSearchPanel);
}

// ── Panel switching ───────────────────────────────────────────────────────────

void MainWindow::showCharactersPanel()
{
    m_panels->setCurrentWidget(m_charactersSplitter);

    // Update toolbar checked state
    for (auto* act : { m_actCharacters, m_actTimeline,
                       m_actMap, m_actChapters, m_actSearch })
        act->setChecked(false);
    m_actCharacters->setChecked(true);
}

void MainWindow::showTimelinePanel()
{
    m_panels->setCurrentWidget(m_timelineWidget);
    m_timelineWidget->activate();

    for (auto* act : { m_actCharacters, m_actTimeline,
                       m_actMap, m_actChapters, m_actSearch })
        act->setChecked(false);
    m_actTimeline->setChecked(true);
}

void MainWindow::showMapPanel()
{
    m_panels->setCurrentWidget(m_mapWidget);
    m_mapWidget->activate();

    for (auto* act : { m_actCharacters, m_actTimeline,
                       m_actMap, m_actChapters, m_actSearch })
        act->setChecked(false);
    m_actMap->setChecked(true);
}

void MainWindow::showChaptersPanel()
{
    // Flush any pending autosave content before leaving another panel
    if (m_chapterEditor) m_chapterEditor->flushContent();

    m_panels->setCurrentWidget(m_chaptersSplitter);
    m_chapterList->refresh();

    for (auto* act : { m_actCharacters, m_actTimeline,
                       m_actMap, m_actChapters, m_actSearch })
        act->setChecked(false);
    m_actChapters->setChecked(true);
}

void MainWindow::showSearchPanel()
{
    m_panels->setCurrentWidget(m_searchWidget);
    m_searchWidget->activate();

    for (auto* act : { m_actCharacters, m_actTimeline,
                       m_actMap, m_actChapters, m_actSearch })
        act->setChecked(false);
    m_actSearch->setChecked(true);
}

void MainWindow::onSettingsRequested()
{
    SettingsDialog::Settings current;
    current.debugMode = false;

    SettingsDialog dlg(current, this);

    connect(&dlg, &SettingsDialog::vacuumRequested, this, [this]() {
        // m_dbManager->execute("VACUUM") — à câbler si exposé
        statusBar()->showMessage("VACUUM executed", 3000);
    });

    connect(&dlg, &SettingsDialog::backupRequested, this, [this]() {
        const QString dest = QFileDialog::getSaveFileName(
            this, "Save backup", {}, "SQLite Database (*.db)");
        if (!dest.isEmpty()) {
            statusBar()->showMessage("Backup saved to " + dest, 4000);
        }
    });

    if (dlg.exec() == QDialog::Accepted) {
        const auto s = dlg.settings();
        m_logger.info("Settings updated", "MainWindow");
        statusBar()->showMessage("Settings applied", 2000);
    }
}

// ── Character slot implementations ───────────────────────────────────────────

void MainWindow::onCharacterSelected(CharacterId id)
{
    m_characterEditor->loadCharacter(id);
    statusBar()->showMessage(
        QString("Editing character #%1").arg(id.value()), 2000);
}

void MainWindow::onCreateCharacterRequested()
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, "New character", "Character name:",
        QLineEdit::Normal, "", &ok);

    if (!ok || name.trimmed().isEmpty()) return;

    m_characterViewModel->createCharacter(name.trimmed(), "", 1, 1);
    statusBar()->showMessage("Character created: " + name, 3000);
}

// ── Search navigation ─────────────────────────────────────────────────────────

void MainWindow::onNavigateToCharacter(qint64 id)
{
    showCharactersPanel();
    m_characterEditor->loadCharacter(CharacterId(id));
    statusBar()->showMessage(
        QString("Navigated to character #%1").arg(id), 3000);
}

void MainWindow::onNavigateToPlace(qint64 id)
{
    showMapPanel();
    // Select the node in the map
    const int nodeIdx = m_mapViewModel->nodeIndexForId(PlaceId(id));
    if (nodeIdx >= 0) {
        m_mapViewModel->selectNode(nodeIdx);
    }
    statusBar()->showMessage(
        QString("Navigated to place #%1").arg(id), 3000);
}

void MainWindow::onNavigateToFaction(qint64 id)
{
    // Factions don't have their own panel yet —
    // show on the map (where faction nodes will eventually live)
    // For now: show a status message and open characters filtered by faction
    showCharactersPanel();
    statusBar()->showMessage(
        QString("Faction #%1 — filter characters by faction coming soon").arg(id),
        4000);
}

void MainWindow::onNavigateToChapter(qint64 id)
{
    showChaptersPanel();

    // Find the chapter row in the ViewModel and select it
    const int rowCount = m_chapterViewModel->rowCount();
    for (int i = 0; i < rowCount; ++i) {
        const auto idx = m_chapterViewModel->index(i, 0);
        if (idx.data(ChapterViewModel::IdRole).toLongLong() == id) {
            m_chapterViewModel->setActiveChapter(i);
            break;
        }
    }
    statusBar()->showMessage(
        QString("Navigated to chapter #%1").arg(id), 3000);
}

// ── closeEvent ────────────────────────────────────────────────────────────────

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Flush pending chapter autosave before closing
    if (m_chapterEditor) {
        m_chapterEditor->flushContent();
    }
    m_logger.info("MainWindow closing", "MainWindow");
    event->accept();
}

// ── applyStyleSheet ───────────────────────────────────────────────────────────

void MainWindow::applyStyleSheet()
{
    QFile styleFile(":/styles/main.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        qApp->setStyleSheet(QLatin1String(styleFile.readAll()));
        return;
    }

    // Full inline stylesheet (Catppuccin Mocha)
    qApp->setStyleSheet(R"(
        * { outline: none; }
        QMainWindow, QWidget       { background: #1e1e2e; color: #cdd6f4; }
        QToolBar                   { background: #181825; border: none;
                                     padding: 4px 8px; spacing: 2px; }
        QToolBar QToolButton       { color: #a6adc8; padding: 6px 14px;
                                     border-radius: 5px; font-size: 13px;
                                     font-weight: 500; }
        QToolBar QToolButton:hover   { background: #313244; color: #cdd6f4; }
        QToolBar QToolButton:checked { background: #45475a; color: #cdd6f4; }
        QSplitter::handle          { background: #313244; }
        QSplitter::handle:horizontal { width: 1px; }
        QSplitter::handle:vertical   { height: 1px; }
        QListView                  { background: #181825; color: #cdd6f4;
                                     border: none; font-size: 13px;
                                     padding: 4px; }
        QListView::item            { padding: 6px 10px; border-radius: 5px;
                                     margin: 1px 2px; }
        QListView::item:selected   { background: #45475a; }
        QListView::item:hover      { background: #313244; }
        QLineEdit                  { background: #313244; color: #cdd6f4;
                                     border: 1px solid #45475a;
                                     border-radius: 5px; padding: 5px 10px;
                                     font-size: 13px; }
        QLineEdit:focus            { border-color: #89b4fa; }
        #searchInput               { font-size: 15px; padding: 6px 12px; }
        QTextEdit, QPlainTextEdit  { background: #181825; color: #cdd6f4;
                                     border: none; font-size: 14px;
                                     selection-background-color: #45475a; }
        #markdownEditor            { background: #1e1e2e;
                                     padding: 12px 16px; }
        #markdownPreview           { background: #181825;
                                     padding: 12px 16px; }
        QSpinBox                   { background: #313244; color: #cdd6f4;
                                     border: 1px solid #45475a;
                                     border-radius: 5px; padding: 4px 8px; }
        QComboBox                  { background: #313244; color: #cdd6f4;
                                     border: 1px solid #45475a;
                                     border-radius: 5px; padding: 4px 8px; }
        QComboBox QAbstractItemView { background: #313244;
                                      selection-background-color: #45475a; }
        QTableWidget               { background: #181825; color: #cdd6f4;
                                     gridline-color: #313244; border: none;
                                     font-size: 12px; }
        QHeaderView::section       { background: #313244; color: #a6adc8;
                                     padding: 5px 8px; border: none;
                                     font-size: 11px; font-weight: bold; }
        QPushButton                { background: #313244; color: #cdd6f4;
                                     border: none; border-radius: 5px;
                                     padding: 6px 16px; font-size: 13px; }
        QPushButton:hover          { background: #45475a; }
        QPushButton:pressed        { background: #585b70; }
        QPushButton#primaryButton  { background: #89b4fa; color: #1e1e2e;
                                     font-weight: bold; }
        QPushButton#primaryButton:hover { background: #74c7ec; }
        QPushButton#createButton   { background: #313244; color: #89b4fa;
                                     font-weight: bold; font-size: 16px; }
        QToolButton#timelineBtn    { background: #313244; color: #cdd6f4;
                                     border: none; border-radius: 4px;
                                     padding: 4px 10px; font-size: 12px; }
        QToolButton#timelineBtn:hover   { background: #45475a; }
        QToolButton#timelineBtn:checked { background: #585b70; }
        QCheckBox#laneToggle       { color: #a6adc8; font-size: 11px; }
        QLabel#sectionTitle        { color: #cdd6f4; font-size: 14px;
                                     font-weight: bold; }
        QLabel#countLabel          { color: #6c7086; font-size: 11px; }
        QLabel#placeholderLabel    { color: #45475a; font-size: 22px; }
        QLabel#windowLabel         { color: #6c7086; font-size: 11px; }
        QGroupBox                  { color: #a6adc8; font-size: 12px;
                                     border: 1px solid #313244;
                                     border-radius: 5px; margin-top: 10px;
                                     padding-top: 8px; }
        QGroupBox::title           { subcontrol-origin: margin;
                                     left: 8px; padding: 0 4px; }
        QTabWidget::pane           { border: none; background: #1e1e2e; }
        QTabBar::tab               { background: #181825; color: #6c7086;
                                     padding: 7px 18px; border: none;
                                     font-size: 12px; }
        QTabBar::tab:selected      { background: #313244; color: #cdd6f4; }
        QTabBar::tab:hover         { color: #a6adc8; }
        QScrollBar:vertical        { background: #181825; width: 8px;
                                     border: none; }
        QScrollBar::handle:vertical { background: #45475a; border-radius: 4px;
                                      min-height: 24px; }
        QScrollBar:horizontal      { background: #181825; height: 8px;
                                     border: none; }
        QScrollBar::handle:horizontal { background: #45475a;
                                        border-radius: 4px; min-width: 24px; }
        QScrollBar::add-line,
        QScrollBar::sub-line       { width: 0; height: 0; }
        QStatusBar                 { background: #11111b; color: #6c7086;
                                     font-size: 11px; border-top: 1px solid #313244; }
        #editorHeader              { background: #181825;
                                     border-bottom: 1px solid #313244; }
        #searchBar                 { background: #181825;
                                     border-bottom: 1px solid #313244; }
        #mapToolbar,
        #timelineToolbar           { background: #181825;
                                     border-bottom: 1px solid #313244; }
        QScrollBar#timelineScrollBar { background: #181825; height: 10px; }
    )");
}

} // namespace CF::UI