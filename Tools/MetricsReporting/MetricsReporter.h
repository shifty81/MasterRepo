#pragma once
// MetricsReporter — Metrics reporting pipeline (Blueprint: dev_tools/metrics_reporting_pipeline)
//
// Collects metrics from multiple in-engine sources and produces scheduled
// reports in Markdown, JSON, and CSV formats.
//
// Sources:
//   • AI::MetricsDashboard — PCG, rendering, gameplay, AI agent metrics
//   • AI::RenderingOptimizer — per-frame render stats
//   • Tools::TestPipeline — test pass/fail history
//   • Tools::AdminConsole — server uptime, player count
//
// Delivery:
//   • File — written to Logs/Reports/ on schedule or on-demand
//   • Webhook stub — POST to configurable URL (offline: logs to file)
//   • Console / stdout print

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace Tools {

// ─────────────────────────────────────────────────────────────────────────────
// Report format
// ─────────────────────────────────────────────────────────────────────────────

enum class ReportFormat { Markdown, JSON, CSV, Prometheus };

// ─────────────────────────────────────────────────────────────────────────────
// Metric snapshot — one value per named metric at a point in time
// ─────────────────────────────────────────────────────────────────────────────

struct MetricSnapshot {
    double  timestamp = 0.0;  // Unix seconds
    std::unordered_map<std::string,float> values;
    std::string source;       // subsystem that produced this
};

// ─────────────────────────────────────────────────────────────────────────────
// Report schedule
// ─────────────────────────────────────────────────────────────────────────────

struct ReportSchedule {
    std::string  name;
    float        intervalSec;      // 0 = manual only
    ReportFormat format = ReportFormat::Markdown;
    std::string  outputPath;       // relative to repo root; empty = stdout
    std::string  webhookUrl;       // empty = disabled
    bool         enabled = true;
};

// ─────────────────────────────────────────────────────────────────────────────
// Report document
// ─────────────────────────────────────────────────────────────────────────────

struct ReportDocument {
    std::string  scheduleName;
    std::string  timestamp;
    std::string  content;          // fully rendered text
    ReportFormat format = ReportFormat::Markdown;
    bool         delivered = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// MetricsReporterConfig
// ─────────────────────────────────────────────────────────────────────────────

struct MetricsReporterConfig {
    std::string  repoRoot;
    std::string  logDir    = "Logs/Reports";
    int          maxHistory = 100;  // keep last N reports per schedule
    bool         printToConsole = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// Renderer interface — formats a snapshot as the requested format
// ─────────────────────────────────────────────────────────────────────────────

class ReportRenderer {
public:
    static std::string RenderMarkdown(
        const std::string& title,
        const std::vector<MetricSnapshot>& snapshots);

    static std::string RenderJSON(
        const std::string& title,
        const std::vector<MetricSnapshot>& snapshots);

    static std::string RenderCSV(
        const std::vector<MetricSnapshot>& snapshots);

    static std::string RenderPrometheus(
        const MetricSnapshot& latest);
};

// ─────────────────────────────────────────────────────────────────────────────
// MetricsReporter
// ─────────────────────────────────────────────────────────────────────────────

class MetricsReporter {
public:
    explicit MetricsReporter(MetricsReporterConfig cfg = {});
    ~MetricsReporter();

    // ── Schedule management ──────────────────────────────────────────────────

    void AddSchedule(ReportSchedule schedule);
    void RemoveSchedule(const std::string& name);
    void SetScheduleEnabled(const std::string& name, bool enabled);

    // ── Metric ingestion ─────────────────────────────────────────────────────

    void Feed(const MetricSnapshot& snap);
    void FeedValues(const std::string& source,
                    const std::unordered_map<std::string,float>& values,
                    double timestamp = 0.0);

    // ── Per-frame tick ───────────────────────────────────────────────────────

    /// Checks schedules and fires any due reports.
    void Tick(double now = 0.0);

    // ── Manual report generation ─────────────────────────────────────────────

    ReportDocument GenerateReport(const std::string& scheduleName) const;
    ReportDocument GenerateReport(const std::string& title,
                                   ReportFormat format) const;

    bool DeliverReport(ReportDocument& doc) const;

    // ── History ──────────────────────────────────────────────────────────────

    std::vector<ReportDocument> GetHistory(const std::string& scheduleName,
                                            int lastN = 0) const;

    // ── Callbacks ────────────────────────────────────────────────────────────

    using DeliveryFn = std::function<void(const ReportDocument&)>;
    void SetDeliveryCallback(DeliveryFn fn);

    // ── Stats ────────────────────────────────────────────────────────────────

    size_t SnapshotCount()  const;
    size_t ScheduleCount()  const;
    size_t ReportsFired()   const;

private:
    MetricsReporterConfig                  m_cfg;
    std::vector<ReportSchedule>            m_schedules;
    std::vector<MetricSnapshot>            m_snapshots;
    std::unordered_map<std::string,std::vector<ReportDocument>> m_history;
    std::unordered_map<std::string,double> m_lastFired;
    size_t                                 m_reportsFired = 0;
    DeliveryFn                             m_deliveryCb;

    void FireSchedule(const ReportSchedule& sched, double now);
    bool WriteToFile(const ReportDocument& doc) const;
    bool PostWebhook(const ReportDocument& doc) const;
    static double NowSecs();
};

} // namespace Tools
