#pragma once
/**
 * @file TelemetryReporter.h
 * @brief Opt-in crash and feature-usage telemetry.
 *
 * All reporting is strictly opt-in (disabled by default).  Data stays local
 * unless the user explicitly enables remote reporting.  Reports include:
 *   - Crash reports (stack trace, platform info, engine version)
 *   - Feature usage counters (which editor panels / game systems were used)
 *   - Performance outlier events (frame > threshold ms)
 *   - Engine/OS version for compatibility analysis
 *
 * Typical usage:
 * @code
 *   TelemetryReporter tel;
 *   tel.Init("Logs/telemetry/");
 *   tel.SetOptIn(true);   // user must explicitly enable
 *   tel.RecordFeatureUsage("editor.gizmo.scale");
 *   tel.RecordCrash("NullPtrException in PhysicsWorld", stackTrace);
 *   tel.RecordPerformanceOutlier("frame_time_ms", 120.f, 16.f);
 *   tel.FlushLocal();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Core {

// ── Telemetry event types ─────────────────────────────────────────────────────

enum class TelemetryEventType : uint8_t {
    FeatureUsage       = 0,
    CrashReport        = 1,
    PerformanceOutlier = 2,
    SessionBoundary    = 3,
    UserFeedback       = 4,
};

// ── A single telemetry event ──────────────────────────────────────────────────

struct TelemetryEvent {
    TelemetryEventType type{TelemetryEventType::FeatureUsage};
    std::string        name;
    std::string        detail;
    float              numericValue{0.f};
    uint64_t           timestampMs{0};
    std::string        engineVersion;
    std::string        platform;
};

// ── TelemetryReporter ─────────────────────────────────────────────────────────

class TelemetryReporter {
public:
    TelemetryReporter();
    ~TelemetryReporter();

    /// Initialise and set local storage directory.
    void Init(const std::string& storageDir = "Logs/telemetry/");
    void Shutdown();

    // ── Opt-in control ────────────────────────────────────────────────────────

    void SetOptIn(bool enabled);
    bool IsOptedIn() const;

    // ── Event recording ───────────────────────────────────────────────────────

    /// Record a named feature as used (e.g. "editor.content_browser.open").
    void RecordFeatureUsage(const std::string& featureName);

    /// Record a crash with optional stack trace.
    void RecordCrash(const std::string& message, const std::string& stackTrace = "");

    /// Record a performance outlier (actual vs threshold).
    void RecordPerformanceOutlier(const std::string& metric,
                                  float actualValue,
                                  float threshold);

    /// Record arbitrary user feedback text.
    void RecordFeedback(const std::string& feedbackText);

    // ── Persistence ───────────────────────────────────────────────────────────

    /// Write buffered events to local JSONL files.
    void FlushLocal();

    /// Clear local telemetry files (e.g. after confirmed upload).
    void ClearLocal();

    // ── Query ─────────────────────────────────────────────────────────────────

    std::vector<TelemetryEvent> BufferedEvents()         const;
    uint32_t                    BufferedEventCount()     const;
    std::vector<std::string>    LocalReportFiles()       const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnEvent(std::function<void(const TelemetryEvent&)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Core
