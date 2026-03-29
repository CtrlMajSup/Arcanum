#pragma once

#include <memory>

// Forward declarations — App.cpp owns all includes
namespace CF::Core    { class Logger; struct AppConfig; }
namespace CF::Infra   { class DatabaseManager; }
namespace CF::Services { class CharacterService; class PlaceService;
                        class FactionService;   class TimelineService;
                        class SearchService;    class ChapterService;
                        class AutoCompleteService; }
namespace CF::UI      { class MainWindow; }

namespace CF {

/**
 * App is the composition root.
 * It constructs every dependency in the correct order and wires them together.
 * Nothing outside App should ever call `new` or `make_shared` for services.
 */
class App {
public:
    App();
    ~App();

    // Returns false if initialization failed (e.g. DB unreachable)
    [[nodiscard]] bool initialize();

    void showMainWindow();

private:
    // Ordered: each layer depends only on layers below it
    std::unique_ptr<Core::Logger>              m_logger;
    std::unique_ptr<Infra::DatabaseManager>    m_dbManager;

    std::shared_ptr<Services::CharacterService>    m_characterService;
    std::shared_ptr<Services::PlaceService>        m_placeService;
    std::shared_ptr<Services::FactionService>      m_factionService;
    std::shared_ptr<Services::TimelineService>     m_timelineService;
    std::shared_ptr<Services::SearchService>       m_searchService;
    std::shared_ptr<Services::ChapterService>      m_chapterService;
    std::shared_ptr<Services::AutoCompleteService> m_autoCompleteService;

    std::unique_ptr<UI::MainWindow> m_mainWindow;
};

} // namespace CF