#pragma once
/**
 * @file AnomalyAlerting.h
 * @brief Automated alerting when AI models produce outlier outputs or when
 *        PCG/gameplay metrics cross configured thresholds.
 *
 * Supports:
 *   - Rule-based thresholds (value < min or value > max)
 *   - Statistical outlier detection (z-score)
 *   - Configurable severity levels and alert channels
 *   - Deduplication / rate-limiting so spammy alerts are suppressed
 *
 * Typical usage:
 * @code
 *   AnomalyAlerting alerter;
 *   alerter.Init();
 *   alerter.SetThreshold("pcg_polygon_count", 0.f, 500000.f);
 *   alerter.SetZScoreLimit("ai_confidence", 2.5f);
 *   alerter.Sample("pcg_polygon_count", 800000.f);  // triggers alert
 *   alerter.OnAlert([](const AnomalyAlert& a){ Log(a.message); });
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace AI {

// ── Severity levels ───────────────────────────────────────────────────────────

enum class AlertSeverity : uint8_t {
    Info     = 0,
    Warning  = 1,
    Critical = 2,
};

// ── A fired alert ─────────────────────────────────────────────────────────────

struct AnomalyAlert {
    uint64_t      id{0};
    std::string   metric;
    float         value{0.0f};
    float         threshold{0.0f};
    AlertSeverity severity{AlertSeverity::Warning};
    std::string   message;
    uint64_t      timestampMs{0};
    bool          acknowledged{false};
};

// ── AnomalyAlerting ───────────────────────────────────────────────────────────

class AnomalyAlerting {
public:
    AnomalyAlerting();
    ~AnomalyAlerting();

    void Init();
    void Shutdown();

    // ── Rule configuration ────────────────────────────────────────────────────

    /// Set min/max bounds for a named metric.
    void SetThreshold(const std::string& metric, float minVal, float maxVal,
                      AlertSeverity severity = AlertSeverity::Warning);

    /// Set z-score limit for statistical outlier detection on a metric.
    void SetZScoreLimit(const std::string& metric, float maxZScore,
                        AlertSeverity severity = AlertSeverity::Warning);

    /// Minimum interval (ms) between repeated alerts for the same metric.
    void SetCooldownMs(const std::string& metric, uint64_t cooldownMs);

    // ── Data ingestion ────────────────────────────────────────────────────────

    /// Feed a new sample for a metric; fires alert if rule is violated.
    void Sample(const std::string& metric, float value);

    // ── Alert management ──────────────────────────────────────────────────────

    std::vector<AnomalyAlert> ActiveAlerts()       const;
    std::vector<AnomalyAlert> AlertHistory()       const;
    void                      Acknowledge(uint64_t alertId);
    void                      AcknowledgeAll();
    void                      ClearHistory();

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnAlert(std::function<void(const AnomalyAlert&)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace AI
