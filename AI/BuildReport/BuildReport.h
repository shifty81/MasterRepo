#pragma once
/**
 * @file BuildReport.h
 * @brief Generates pull-request-style AI reasoning reports for each build or
 *        AI-assisted session.
 *
 * Collects metrics, AI suggestions made, anomalies detected, suggestions
 * accepted/rejected, and reasoning traces, then renders a Markdown report
 * suitable for saving to `Logs/Build/` or posting as a PR comment.
 *
 * Typical usage:
 * @code
 *   BuildReport reporter;
 *   reporter.Init();
 *   reporter.BeginSession("session-2026-03-26");
 *   reporter.RecordMetric("polygons_generated", 124000.f);
 *   reporter.RecordSuggestion("LOD reduction", true);
 *   reporter.RecordAnomaly("ai_confidence", 0.12f, "below threshold 0.5");
 *   reporter.EndSession();
 *   reporter.SaveMarkdown("Logs/Build/report_2026-03-26.md");
 * @endcode
 */

#include <cstdint>
#include <string>
#include <vector>

namespace AI {

// ── A metric snapshot ─────────────────────────────────────────────────────────

struct ReportMetric {
    std::string name;
    float       value{0.0f};
    std::string unit;
};

// ── A recorded AI suggestion ──────────────────────────────────────────────────

struct ReportSuggestion {
    std::string description;
    bool        accepted{false};
    float       confidence{0.0f};
};

// ── An anomaly entry ──────────────────────────────────────────────────────────

struct ReportAnomaly {
    std::string metric;
    float       value{0.0f};
    std::string reason;
};

// ── BuildReport ───────────────────────────────────────────────────────────────

class BuildReport {
public:
    BuildReport();
    ~BuildReport();

    void Init();
    void Shutdown();

    // ── Session lifecycle ─────────────────────────────────────────────────────

    void BeginSession(const std::string& sessionTag);
    void EndSession();
    bool IsSessionActive() const;

    // ── Data recording ────────────────────────────────────────────────────────

    void RecordMetric(const std::string& name, float value,
                      const std::string& unit = "");
    void RecordSuggestion(const std::string& description, bool accepted,
                          float confidence = 0.0f);
    void RecordAnomaly(const std::string& metric, float value,
                       const std::string& reason);
    void RecordNote(const std::string& text);

    // ── Output ────────────────────────────────────────────────────────────────

    /// Render the current session as a Markdown string.
    std::string ToMarkdown()                                   const;

    /// Write the Markdown report to a file.
    bool SaveMarkdown(const std::string& filePath)             const;

    /// Return a compact one-line summary for logging.
    std::string Summary()                                      const;

    // ── History ───────────────────────────────────────────────────────────────

    std::vector<std::string> PreviousSessions()                const;
    void                     ClearHistory();

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace AI
