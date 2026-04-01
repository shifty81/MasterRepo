#pragma once
#include <string_view>

namespace NF {

/// @brief Severity levels for the logging system, ordered least to most severe.
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

/// @brief Central, thread-safe logging facility for NovaForge.
class Logger {
public:
    /// @brief Emit a log message.
    /// @param level   Severity of the message.
    /// @param channel Subsystem or category name (e.g. "Renderer", "Physics").
    /// @param msg     Human-readable message text.
    static void Log(LogLevel level, std::string_view channel, std::string_view msg);

    /// @brief Set the minimum severity level; messages below it are silently discarded.
    /// @param level The new minimum level.
    static void SetMinLevel(LogLevel level) noexcept;

    /// @brief Return the short label string for a given log level.
    /// @param level The level to stringify.
    static std::string_view LevelToString(LogLevel level) noexcept;
};

} // namespace NF

// ---------------------------------------------------------------------------
// Convenience logging macros
// ---------------------------------------------------------------------------
#define NF_LOG_TRACE(channel, msg)  ::NF::Logger::Log(::NF::LogLevel::Trace,   (channel), (msg))
#define NF_LOG_DEBUG(channel, msg)  ::NF::Logger::Log(::NF::LogLevel::Debug,   (channel), (msg))
#define NF_LOG_INFO(channel, msg)   ::NF::Logger::Log(::NF::LogLevel::Info,    (channel), (msg))
#define NF_LOG_WARN(channel, msg)   ::NF::Logger::Log(::NF::LogLevel::Warning, (channel), (msg))
#define NF_LOG_ERROR(channel, msg)  ::NF::Logger::Log(::NF::LogLevel::Error,   (channel), (msg))
#define NF_LOG_FATAL(channel, msg)  ::NF::Logger::Log(::NF::LogLevel::Fatal,   (channel), (msg))
