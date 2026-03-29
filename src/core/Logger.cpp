#include "Logger.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace CF::Core {

Logger::Logger(LogLevel minLevel)
    : m_minLevel(minLevel)
{}

void Logger::addConsoleSink()
{
    addCustomSink([](LogLevel level, std::string_view line) {
        // Errors go to stderr, everything else to stdout
        if (level == LogLevel::Error || level == LogLevel::Warning) {
            std::cerr << line << '\n';
        } else {
            std::cout << line << '\n';
        }
    });
}

void Logger::addFileSink(const std::string& filePath)
{
    m_fileStream.open(filePath, std::ios::app);
    if (!m_fileStream.is_open()) {
        std::cerr << "[Logger] Failed to open log file: " << filePath << '\n';
        return;
    }
    addCustomSink([this](LogLevel /*level*/, std::string_view line) {
        m_fileStream << line << '\n';
        m_fileStream.flush();
    });
}

void Logger::addCustomSink(Sink sink)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sinks.push_back(std::move(sink));
}

void Logger::setMinLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minLevel = level;
}

void Logger::debug(std::string_view message, std::string_view context) const
{
    dispatch(LogLevel::Debug, message, context);
}

void Logger::info(std::string_view message, std::string_view context) const
{
    dispatch(LogLevel::Info, message, context);
}

void Logger::warning(std::string_view message, std::string_view context) const
{
    dispatch(LogLevel::Warning, message, context);
}

void Logger::error(std::string_view message, std::string_view context) const
{
    dispatch(LogLevel::Error, message, context);
}

void Logger::dispatch(LogLevel level, std::string_view message, std::string_view context) const
{
    if (level < m_minLevel) return;

    const std::string line = format(level, message, context);

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& sink : m_sinks) {
        sink(level, line);
    }
}

std::string Logger::format(LogLevel level, std::string_view message, std::string_view context)
{
    // Timestamp
    const auto now   = std::chrono::system_clock::now();
    const auto time  = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%H:%M:%S");

    oss << " [" << levelToString(level) << "]";

    if (!context.empty()) {
        oss << " [" << context << "]";
    }

    oss << " " << message;
    return oss.str();
}

std::string_view Logger::levelToString(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
    }
    return "?????";
}

} // namespace CF::Core