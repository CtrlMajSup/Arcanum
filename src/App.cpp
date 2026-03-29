#include "App.h"
#include "core/Logger.h"
#include "core/AppConfig.h"
#include "infrastructure/database/DatabaseManager.h"
#include "infrastructure/repositories/SqliteCharacterRepository.h"
#include "infrastructure/repositories/SqlitePlaceRepository.h"
#include "infrastructure/repositories/SqliteFactionRepository.h"
#include "infrastructure/repositories/SqliteSceneRepository.h"
#include "infrastructure/repositories/SqliteChapterRepository.h"
#include "infrastructure/repositories/SqliteTimelineRepository.h"
#include "infrastructure/repositories/SqliteSearchRepository.h"
#include "services/CharacterService.h"
#include "services/PlaceService.h"
#include "services/FactionService.h"
#include "services/TimelineService.h"
#include "services/SearchService.h"
#include "services/ChapterService.h"
#include "services/AutoCompleteService.h"
#include "ui/MainWindow.h"

namespace CF {

App::App()  = default;
App::~App() = default;

bool App::initialize()
{
    // ── 1. Config + Logger ────────────────────────────────────────────────────
    const auto config = Core::AppConfig::fromEnvironment();

    m_logger = std::make_unique<Core::Logger>(
        config.debugMode ? Core::LogLevel::Debug : Core::LogLevel::Info);
    m_logger->addConsoleSink();
    m_logger->info("ChronicleForge v" + config.appVersion + " starting", "App");

    // ── 2. Database ───────────────────────────────────────────────────────────
    m_dbManager = std::make_unique<Infra::DatabaseManager>(
        config.databasePath, *m_logger);

    const auto dbResult = m_dbManager->initialize();
    if (dbResult.isErr()) {
        m_logger->error("Database init failed: " + dbResult.error().message, "App");
        return false;
    }

    // ── 3. Repositories ───────────────────────────────────────────────────────
    auto characterRepo = std::make_shared<Infra::SqliteCharacterRepository>(
        *m_dbManager, *m_logger);
    auto placeRepo     = std::make_shared<Infra::SqlitePlaceRepository>(
        *m_dbManager, *m_logger);
    auto factionRepo   = std::make_shared<Infra::SqliteFactionRepository>(
        *m_dbManager, *m_logger);
    auto sceneRepo     = std::make_shared<Infra::SqliteSceneRepository>(
        *m_dbManager, *m_logger);
    auto chapterRepo   = std::make_shared<Infra::SqliteChapterRepository>(
        *m_dbManager, *m_logger);
    auto timelineRepo  = std::make_shared<Infra::SqliteTimelineRepository>(
        *m_dbManager, *m_logger);
    auto searchRepo    = std::make_shared<Infra::SqliteSearchRepository>(
        *m_dbManager, *m_logger);

    // ── 4. Services ───────────────────────────────────────────────────────────
    m_timelineService = std::make_shared<Services::TimelineService>(
        timelineRepo, *m_logger);

    m_characterService = std::make_shared<Services::CharacterService>(
        characterRepo, timelineRepo, *m_logger);
    m_placeService = std::make_shared<Services::PlaceService>(
        placeRepo, timelineRepo, *m_logger);
    m_factionService = std::make_shared<Services::FactionService>(
        factionRepo, timelineRepo, *m_logger);

    m_searchService = std::make_shared<Services::SearchService>(
        searchRepo, *m_logger);
    m_chapterService = std::make_shared<Services::ChapterService>(
        chapterRepo, *m_logger);
    m_autoCompleteService = std::make_shared<Services::AutoCompleteService>(
        characterRepo, placeRepo, factionRepo, sceneRepo, *m_logger);

    // ── 5. UI ─────────────────────────────────────────────────────────────────
    m_mainWindow = std::make_unique<UI::MainWindow>(
        m_characterService,
        m_placeService,
        m_factionService,
        m_timelineService,
        m_searchService,
        m_chapterService,
        m_autoCompleteService,
        *m_logger
    );

    m_logger->info("Application initialised successfully", "App");
    return true;
}

void App::showMainWindow()
{
    if (m_mainWindow) {
        m_mainWindow->show();
    }
}

} // namespace CF