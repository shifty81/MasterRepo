#pragma once
/**
 * @file LSPDiagnostics.h
 * @brief LSP diagnostic message manager: receive, store, filter, and display.
 *
 * LSPDiagnostics collects textDocument/publishDiagnostics notifications from
 * a language server and exposes them to the IDE UI:
 *
 *   - Diagnostic: file URI, range (line/col), severity, code, source, message.
 *   - DiagnosticSeverity: Error, Warning, Information, Hint.
 *   - Publish(uri, diagnostics): replace all diagnostics for a file.
 *   - Clear(uri): discard diagnostics for a single file.
 *   - ClearAll(): discard everything.
 *   - InFile(uri): retrieve diagnostics for one file.
 *   - Filter(severity): retrieve all diagnostics at or above a threshold.
 *   - Summary(): per-file counts for a status-bar badge.
 *   - OnDiagnosticsChanged: callback fired after every Publish/Clear call.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace IDE {

// ── Severity ───────────────────────────────────────────────────────────────
enum class DiagnosticSeverity : uint8_t { Error = 1, Warning = 2, Information = 3, Hint = 4 };
const char* DiagnosticSeverityName(DiagnosticSeverity s);

// ── Range ─────────────────────────────────────────────────────────────────
struct DiagnosticRange {
    uint32_t startLine{0};
    uint32_t startChar{0};
    uint32_t endLine{0};
    uint32_t endChar{0};
};

// ── Diagnostic ────────────────────────────────────────────────────────────
struct Diagnostic {
    std::string         uri;          ///< textDocument URI
    DiagnosticRange     range{};
    DiagnosticSeverity  severity{DiagnosticSeverity::Error};
    std::string         code;         ///< Diagnostic code (may be empty)
    std::string         source;       ///< e.g. "clangd", "rust-analyzer"
    std::string         message;
    std::vector<std::string> tags;    ///< e.g. "unnecessary", "deprecated"
};

// ── Per-file summary ──────────────────────────────────────────────────────
struct FileDiagnosticSummary {
    std::string uri;
    uint32_t    errors{0};
    uint32_t    warnings{0};
    uint32_t    infos{0};
    uint32_t    hints{0};
};

// ── Manager ───────────────────────────────────────────────────────────────
class LSPDiagnostics {
public:
    LSPDiagnostics();
    ~LSPDiagnostics();

    LSPDiagnostics(const LSPDiagnostics&) = delete;
    LSPDiagnostics& operator=(const LSPDiagnostics&) = delete;

    // ── receive ───────────────────────────────────────────────
    /// Replace all diagnostics for the given URI (LSP publishDiagnostics).
    void Publish(const std::string& uri,
                 const std::vector<Diagnostic>& diagnostics);

    // ── clear ─────────────────────────────────────────────────
    void Clear(const std::string& uri);
    void ClearAll();

    // ── query ─────────────────────────────────────────────────
    std::vector<Diagnostic>        InFile(const std::string& uri) const;
    std::vector<Diagnostic>        Filter(DiagnosticSeverity minSeverity) const;
    std::vector<Diagnostic>        All() const;
    std::vector<std::string>       FilesWithDiagnostics() const;
    FileDiagnosticSummary          SummaryFor(const std::string& uri) const;
    std::vector<FileDiagnosticSummary> AllSummaries() const;

    size_t TotalCount()   const;
    size_t ErrorCount()   const;
    size_t WarningCount() const;

    // ── diagnostic at position ────────────────────────────────
    std::vector<Diagnostic> AtLine(const std::string& uri, uint32_t line) const;

    // ── callback ──────────────────────────────────────────────
    using ChangedCb = std::function<void(const std::string& uri)>;
    void OnDiagnosticsChanged(ChangedCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace IDE
