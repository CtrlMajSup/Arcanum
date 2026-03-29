#include "AppConfig.h"

#include <cstdlib>
#include <filesystem>

namespace CF::Core {

AppConfig AppConfig::fromEnvironment()
{
    AppConfig cfg;

    // Resolve database path: prefer env var, fall back to user home dir
    const char* envDb = std::getenv("CF_DATABASE_PATH");
    if (envDb) {
        cfg.databasePath = envDb;
    } else {
        namespace fs = std::filesystem;
        // Place DB next to the executable in debug, in user data dir in release
#ifdef CF_DEBUG
        cfg.databasePath = "chronicleforge_dev.db";
#else
        // On Linux/macOS: ~/.local/share/ChronicleForge/data.db
        const char* home = std::getenv("HOME");
        fs::path dataDir = home
            ? fs::path(home) / ".local" / "share" / "ChronicleForge"
            : fs::current_path();
        fs::create_directories(dataDir);
        cfg.databasePath = (dataDir / "data.db").string();
#endif
    }

#ifdef CF_DEBUG
    cfg.debugMode = true;
#else
    cfg.debugMode = false;
#endif

    cfg.appVersion = CF_APP_VERSION;

    return cfg;
}

} // namespace CF::Core