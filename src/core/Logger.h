#pragma once

#include <string>
#include <string_view>
#include <fstream>
#include <mutex>
#include <memory>
#include <functional>

namespace CF::Core {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

/**
 * Logger is a lightweight, thread-safe logging sink.
 *
 * Design: single Logger instance is created in App.cpp and passed
 * to components via constructor injection. No global state.
 *
 * Sinks are pluggable: by default logs to stderr + optional file.
 * In tests, inject a capturing sink instead.
 */
class Logger {
public:
    // A sink is any callable that receives a formatted log line
    using Sink = std::function<void(LogLevel, std::string_view)>;

    explicit Logger(LogLevel minLevel = LogLevel::Debug);

    // Add output destinations
    void addConsoleSink();
    void addFileSink(const std::string& filePath);
    void addCustomSink(Sink sink);

    // Core logging methods
    void debug  (std::string_view message, std::string_view context = "") const;
    void info   (std::string_view message, std::string_view context = "") const;
    void warning(std::string_view message, std::string_view context = "") const;
    void error  (std::string_view message, std::string_view context = "") const;

    void setMinLevel(LogLevel level);

private:
    void dispatch(LogLevel level, std::string_view message, std::string_view context) const;
    static std::string format(LogLevel level, std::string_view message, std::string_view context);
    static std::string_view levelToString(LogLevel level);

    LogLevel                 m_minLevel;
    mutable std::mutex       m_mutex;
    std::vector<Sink>        m_sinks;
    mutable std::ofstream    m_fileStream;
};

} // namespace CF::Core


// ─── Convenience macros ───────────────────────────────────────────────────────
// Usage: CF_LOG_DEBUG(logger, "loading entity", "CharacterService")
// The context parameter identifies the calling subsystem.

#define CF_LOG_DEBUG(logger, msg, ctx)   (logger).debug  (msg, ctx)
#define CF_LOG_INFO(logger, msg, ctx)    (logger).info   (msg, ctx)
#define CF_LOG_WARN(logger, msg, ctx)    (logger).warning(msg, ctx)
#define CF_LOG_ERROR(logger, msg, ctx)   (logger).error  (msg, ctx)