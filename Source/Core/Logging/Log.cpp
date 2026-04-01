#include "Core/Logging/Log.h"
#include <iostream>
#include <mutex>
#include <chrono>
#include <string>
#include <cstdlib>

namespace NF {

namespace {
    LogLevel  s_MinLevel = LogLevel::Trace;
    std::mutex s_Mutex;
} // anonymous namespace

std::string_view Logger::LevelToString(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
    }
    return "?????";
}

void Logger::SetMinLevel(LogLevel level) noexcept {
    std::lock_guard lock(s_Mutex);
    s_MinLevel = level;
}

void Logger::Log(LogLevel level, std::string_view channel, std::string_view msg) {
    if (level < s_MinLevel)
        return;

    auto now = std::chrono::steady_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                   now.time_since_epoch()).count();

    auto& out = (level >= LogLevel::Error) ? std::cerr : std::cout;

    std::lock_guard lock(s_Mutex);
    out << '[' << ms << "ms]["
        << LevelToString(level) << "]["
        << channel << "] "
        << msg << '\n';

    if (level == LogLevel::Fatal)
        std::abort();
}

} // namespace NF
