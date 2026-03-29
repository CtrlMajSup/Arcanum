#pragma once

#include <string>

namespace CF::Core {

/**
 * AppConfig holds runtime configuration resolved at startup.
 * It is constructed once in App.cpp and injected where needed.
 * Never use global singletons for config — pass this explicitly.
 */
struct AppConfig {
    std::string databasePath;   // Absolute path to the .db file
    bool        debugMode;      // Mirrors the CF_DEBUG compile flag at runtime
    std::string appVersion;     // Injected from CMake via CF_APP_VERSION

    // Factory: builds a config from compile-time flags and environment
    static AppConfig fromEnvironment();
};

} // namespace CF::Core