#pragma once
// MetricsDashboard — AI-layer metrics dashboard for PCG, rendering, and gameplay
//
// Aggregates metrics from multiple subsystems into a unified time-series
// store, then provides query, alerting, and Markdown/JSON report generation.
//
// Metric sources:
//   • PCG pipeline (generation time, constraint pass rate, module counts)
//   • Rendering (frame time, draw calls, VRAM — from RenderingOptimizer)
//   • Gameplay (player health, AI agent counts, NPC actions/second)
//   • AI agent (job throughput, error rate, model inference latency)
//
// Storage: in-memory ring buffer per metric stream; optional flush to CSV.

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace AI {

// ─────────────────────────────────────────────────────────────────────────────
// Metric sample
// ─────────────────────────────────────────────────────────────────────────────

enum class MetricType { Counter, Gauge, Histogram };
enum class MetricCategory { PCG, Rendering, Gameplay, AIAgent, Custom };

struct MetricSample {
    double   timestamp = 0.0;   // Unix seconds (double for sub-second)
    float    value     = 0.f;
};

struct MetricStream {
    std::string     id;
    std::string     displayName;
    std::string     unit;        // "ms", "MB", "count", etc.
    MetricType      type     = MetricType::Gauge;
    MetricCategory  category = MetricCategory::Custom;
    int             maxSamples = 1000;   // ring buffer capacity
    std::vector<MetricSample> samples;
};

// ─────────────────────────────────────────────────────────────────────────────
// Alert
// ─────────────────────────────────────────────────────────────────────────────

enum class AlertSeverity { Info, Warning, Critical };

struct MetricAlert {
    std::string   metricId;
    AlertSeverity severity  = AlertSeverity::Warning;
    float         threshold = 0.f;
    bool          aboveThreshold = true;  // true = alert when value > threshold
    std::string   message;
};

struct FiredAlert {
    MetricAlert   alert;
    float         value     = 0.f;
    double        timestamp = 0.0;
};

// ─────────────────────────────────────────────────────────────────────────────
// MetricsDashboardConfig
// ─────────────────────────────────────────────────────────────────────────────

struct MetricsDashboardConfig {
    int    defaultMaxSamples = 500;
    std::string csvFlushPath;       // empty = no CSV flush
    int    csvFlushIntervalSec = 60;
    bool   enableAlerts = true;
};

// ─────────────────────────────────────────────────────────────────────────────
// MetricsDashboard
// ─────────────────────────────────────────────────────────────────────────────

class MetricsDashboard {
public:
    explicit MetricsDashboard(MetricsDashboardConfig cfg = {});
    ~MetricsDashboard();

    // ── Registration ────────────────────────────────────────────────────────

    void RegisterMetric(MetricStream stream);
    void UnregisterMetric(const std::string& id);
    bool HasMetric(const std::string& id) const;

    // ── Recording ────────────────────────────────────────────────────────────

    void Record(const std::string& id, float value, double timestamp = 0.0);
    void RecordMany(const std::unordered_map<std::string,float>& values,
                    double timestamp = 0.0);

    // ── Queries ──────────────────────────────────────────────────────────────

    float Latest(const std::string& id) const;
    float Average(const std::string& id, int lastN = 0) const;
    float Min(const std::string& id, int lastN = 0) const;
    float Max(const std::string& id, int lastN = 0) const;
    float Percentile(const std::string& id, float p, int lastN = 0) const;

    std::vector<MetricSample> GetSamples(const std::string& id,
                                          int lastN = 0) const;
    std::vector<std::string>  GetMetricIds(MetricCategory cat) const;

    // ── Alerting ─────────────────────────────────────────────────────────────

    void AddAlert(MetricAlert alert);
    void RemoveAlert(const std::string& metricId);
    std::vector<FiredAlert> CheckAlerts() const;

    // Alert callback — called each tick when an alert fires
    using AlertFn = std::function<void(const FiredAlert&)>;
    void SetAlertCallback(AlertFn fn);

    // ── Per-frame tick ───────────────────────────────────────────────────────

    void Tick(double now = 0.0);

    // ── Reporting ─────────────────────────────────────────────────────────────

    std::string BuildMarkdownReport(MetricCategory cat) const;
    std::string BuildFullReport()  const;
    bool        FlushCSV()         const;

    // ── Stats ────────────────────────────────────────────────────────────────

    size_t MetricCount()      const;
    size_t TotalSamples()     const;

private:
    MetricsDashboardConfig               m_cfg;
    std::unordered_map<std::string,MetricStream> m_streams;
    std::vector<MetricAlert>             m_alerts;
    AlertFn                              m_alertCb;
    double                               m_lastFlush = 0.0;

    MetricStream* FindStream(const std::string& id);
    const MetricStream* FindStream(const std::string& id) const;
    static double NowSecs();
};

} // namespace AI
