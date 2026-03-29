#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QSplitter>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <memory>

// Services
#include "services/CharacterService.h"
#include "services/PlaceService.h"
#include "services/FactionService.h"
#include "services/TimelineService.h"
#include "services/SearchService.h"
#include "services/ChapterService.h"
#include "services/AutoCompleteService.h"

// ViewModels
#include "ui/viewmodels/CharacterViewModel.h"
#include "ui/viewmodels/TimelineViewModel.h"
#include "ui/viewmodels/MapViewModel.h"
#include "ui/viewmodels/ChapterViewModel.h"
#include "ui/viewmodels/SearchViewModel.h"

// Widgets
#include "ui/widgets/characters/CharacterListWidget.h"
#include "ui/widgets/characters/CharacterEditorWidget.h"
#include "ui/widgets/timeline/TimelineWidget.h"
#include "ui/widgets/map/MapWidget.h"
#include "ui/widgets/chapters/ChapterListWidget.h"
#include "ui/widgets/chapters/ChapterEditorWidget.h"
#include "ui/widgets/search/SearchWidget.h"

// Setttings
#include "ui/dialogs/NewProjectDialog.h"
#include "ui/dialogs/SettingsDialog.h"

#include "core/Logger.h"

namespace CF::UI {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(
        std::shared_ptr<Services::CharacterService>    characterService,
        std::shared_ptr<Services::PlaceService>        placeService,
        std::shared_ptr<Services::FactionService>      factionService,
        std::shared_ptr<Services::TimelineService>     timelineService,
        std::shared_ptr<Services::SearchService>       searchService,
        std::shared_ptr<Services::ChapterService>      chapterService,
        std::shared_ptr<Services::AutoCompleteService> autoCompleteService,
        Core::Logger& logger,
        QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Toolbar navigation
    void showCharactersPanel();
    void showTimelinePanel();
    void showMapPanel();
    void showChaptersPanel();
    void showSearchPanel();

    // Settings
    void onSettingsRequested();

    // Character panel slots
    void onCharacterSelected(Domain::CharacterId id);
    void onCreateCharacterRequested();

    // Search navigation slots
    void onNavigateToCharacter(qint64 id);
    void onNavigateToPlace(qint64 id);
    void onNavigateToFaction(qint64 id);
    void onNavigateToChapter(qint64 id);

private:
    void buildViewModels(
        std::shared_ptr<Services::CharacterService>    characterService,
        std::shared_ptr<Services::PlaceService>        placeService,
        std::shared_ptr<Services::FactionService>      factionService,
        std::shared_ptr<Services::TimelineService>     timelineService,
        std::shared_ptr<Services::SearchService>       searchService,
        std::shared_ptr<Services::ChapterService>      chapterService,
        std::shared_ptr<Services::AutoCompleteService> autoCompleteService);

    void setupToolBar();
    void setupCentralWidget();
    void setupStatusBar();
    void setupPanelConnections();
    void applyStyleSheet();

    // ── ViewModels ────────────────────────────────────────────────────────────
    std::shared_ptr<CharacterViewModel> m_characterViewModel;
    std::shared_ptr<TimelineViewModel>  m_timelineViewModel;
    std::shared_ptr<MapViewModel>       m_mapViewModel;
    std::shared_ptr<ChapterViewModel>   m_chapterViewModel;
    std::shared_ptr<SearchViewModel>    m_searchViewModel;

    // ── Layout ────────────────────────────────────────────────────────────────
    QStackedWidget* m_panels = nullptr;

    // Characters panel: QSplitter → list | editor
    QSplitter*            m_charactersSplitter = nullptr;
    CharacterListWidget*  m_characterList      = nullptr;
    CharacterEditorWidget* m_characterEditor   = nullptr;

    // Timeline panel
    TimelineWidget* m_timelineWidget = nullptr;

    // Map panel
    MapWidget* m_mapWidget = nullptr;

    // Chapters panel: QSplitter → list | editor
    QSplitter*           m_chaptersSplitter = nullptr;
    ChapterListWidget*   m_chapterList      = nullptr;
    ChapterEditorWidget* m_chapterEditor    = nullptr;

    // Search panel
    SearchWidget* m_searchWidget = nullptr;

    // Toolbar actions (kept for checkmark management)
    QAction* m_actCharacters = nullptr;
    QAction* m_actTimeline   = nullptr;
    QAction* m_actMap        = nullptr;
    QAction* m_actChapters   = nullptr;
    QAction* m_actSearch     = nullptr;

    Core::Logger& m_logger;
};

} // namespace CF::UI