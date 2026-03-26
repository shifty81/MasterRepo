#pragma once
/**
 * @file RuntimeAnalytics.h
 * @brief Gameplay event analytics — records in-game events, session metadata,
 *        player funnel steps, and feature usage stats.
 *
 * All data stays local (offline-first).  A flush callback lets the game push
 * batched events to a local file or an opt-in remote endpoint.
 *
 * Typical usage:
 * @code
 *   RuntimeAnalytics analytics;
 *   analytics.Init("Logs/analytics.jsonl");
 *   analytics.StartSession("player_42");
 *   analytics.Track("ship_built", {{"type","cruiser"},{"parts","18"}});
 *   analytics.Track("enemy_killed", {{"weapon","plasma_cannon"}});
 *   analytics.EndSession();
 *   analytics.Flush();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Runtime {

// ── A single analytics event ──────────────────────────────────────────────────

struct AnalyticsEvent {
    std::string                               name;
    std::unordered_map<std::string,std::string> properties;
    uint64_t                                  timestampMs{0};
    std::string                               sessionId;
};

// ── Session summary ────────────────────────────────────────────────────────────

struct AnalyticsSession {
    std::string sessionId;
    std::string playerId;
    uint64_t    startMs{0};
    uint64_t    endMs{0};
    uint32_t    eventCount{0};
};

// ── RuntimeAnalytics ──────────────────────────────────────────────────────────

class RuntimeAnalytics {
public:
    RuntimeAnalytics();
    ~RuntimeAnalytics();

    /// Initialise with an optional local flush path (JSONL format).
    void Init(const std::string& flushPath = "Logs/analytics.jsonl");
    void Shutdown();

    // ── Session management ────────────────────────────────────────────────────

    void StartSession(const std::string& playerId = "");
    void EndSession();
    bool IsSessionActive() const;
    AnalyticsSession CurrentSession() const;

    // ── Event tracking ────────────────────────────────────────────────────────

    void Track(const std::string& eventName,
               const std::unordered_map<std::string,std::string>& props = {});

    void Increment(const std::string& counter, int32_t delta = 1);
    void Gauge(const std::string& metric, float value);

    // ── Funnel ────────────────────────────────────────────────────────────────

    /// Mark a named funnel step as reached.
    void FunnelStep(const std::string& funnelName, uint32_t step,
                    const std::string& label = "");

    // ── Persistence & export ──────────────────────────────────────────────────

    void Flush();
    void ClearBuffer();

    std::vector<AnalyticsEvent>   BufferedEvents()         const;
    std::vector<AnalyticsSession> SessionHistory()         const;
    uint32_t                      BufferedEventCount()     const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    /// Called when the buffer reaches flushThreshold events.
    void OnAutoFlush(std::function<void(const std::vector<AnalyticsEvent>&)> cb);
    void SetAutoFlushThreshold(uint32_t eventCount);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
