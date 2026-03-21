#pragma once
/**
 * @file OutputPanel.h
 * @brief IDE output/build log panel: append lines with severity, filter, search, export.
 *
 * OutputPanel captures build/run output lines in a bounded circular buffer:
 *   - OutputLine: text, severity (Info/Warning/Error/Debug), category tag, timestamp.
 *   - Append(text, severity, category): add a line; trims oldest when at capacity.
 *   - Filter(severity, category): returns lines matching both criteria.
 *   - Search(query): substring search across all lines.
 *   - TailMode: when enabled, GetLines() always returns the last N lines.
 *   - Export: text or CSV dump of all/filtered lines.
 *   - Clear(): discard all lines.
 *   - OnNewLine: callback fired on each Append().
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace IDE {

// ── Severity ───────────────────────────────────────────────────────────────
enum class OutputSeverity : uint8_t { Debug, Info, Warning, Error };
const char* SeverityName(OutputSeverity s);

// ── Output line ────────────────────────────────────────────────────────────
struct OutputLine {
    uint64_t       id{0};
    std::string    text;
    OutputSeverity severity{OutputSeverity::Info};
    std::string    category;   ///< e.g. "Build", "Run", "Test", "Lint"
    uint64_t       timestampMs{0};
    uint32_t       lineNumber{0}; ///< Sequential line counter
};

// ── Callbacks ──────────────────────────────────────────────────────────────
using NewLineCb = std::function<void(const OutputLine&)>;

// ── Panel ──────────────────────────────────────────────────────────────────
class OutputPanel {
public:
    OutputPanel();
    explicit OutputPanel(uint32_t maxLines);
    ~OutputPanel();

    // ── append ────────────────────────────────────────────────
    void Append(const std::string& text,
                OutputSeverity severity = OutputSeverity::Info,
                const std::string& category = "");

    void AppendBatch(const std::vector<std::string>& lines,
                     OutputSeverity severity = OutputSeverity::Info,
                     const std::string& category = "");

    // ── access ────────────────────────────────────────────────
    std::vector<OutputLine> GetLines(std::optional<uint32_t> tail = {}) const;
    std::vector<OutputLine> Filter(OutputSeverity severity,
                                   const std::string& category = "") const;
    std::vector<OutputLine> Search(const std::string& query,
                                   bool caseSensitive = false) const;
    const OutputLine* LastLine() const;

    size_t TotalCount()   const;   ///< All lines ever appended (wraps at max)
    size_t CurrentCount() const;   ///< Lines currently held in buffer
    uint32_t ErrorCount()   const;
    uint32_t WarningCount() const;
    void Clear();

    // ── tail mode ─────────────────────────────────────────────
    void SetTailMode(bool enabled, uint32_t tailLines = 200);
    bool TailMode() const;

    // ── capacity ──────────────────────────────────────────────
    void SetMaxLines(uint32_t max);
    uint32_t MaxLines() const;

    // ── export ────────────────────────────────────────────────
    std::string ExportText(std::optional<OutputSeverity> severityFilter = {}) const;
    std::string ExportCSV()  const;

    // ── callbacks ─────────────────────────────────────────────
    void OnNewLine(NewLineCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace IDE
