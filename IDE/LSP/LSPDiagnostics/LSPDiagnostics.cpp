#include "IDE/LSP/LSPDiagnostics/LSPDiagnostics.h"
#include <algorithm>
#include <unordered_map>

namespace IDE {

const char* DiagnosticSeverityName(DiagnosticSeverity s) {
    switch (s) {
    case DiagnosticSeverity::Error:       return "Error";
    case DiagnosticSeverity::Warning:     return "Warning";
    case DiagnosticSeverity::Information: return "Information";
    case DiagnosticSeverity::Hint:        return "Hint";
    }
    return "Unknown";
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct LSPDiagnostics::Impl {
    // uri → diagnostics for that file
    std::unordered_map<std::string, std::vector<Diagnostic>> byFile;
    std::vector<ChangedCb> callbacks;

    void fireChanged(const std::string& uri) {
        for (auto& cb : callbacks) cb(uri);
    }
};

LSPDiagnostics::LSPDiagnostics() : m_impl(new Impl()) {}
LSPDiagnostics::~LSPDiagnostics() { delete m_impl; }

void LSPDiagnostics::Publish(const std::string& uri,
                              const std::vector<Diagnostic>& diagnostics) {
    m_impl->byFile[uri] = diagnostics;
    m_impl->fireChanged(uri);
}

void LSPDiagnostics::Clear(const std::string& uri) {
    m_impl->byFile.erase(uri);
    m_impl->fireChanged(uri);
}

void LSPDiagnostics::ClearAll() {
    auto keys = FilesWithDiagnostics();
    m_impl->byFile.clear();
    for (auto& uri : keys) m_impl->fireChanged(uri);
}

std::vector<Diagnostic> LSPDiagnostics::InFile(const std::string& uri) const {
    auto it = m_impl->byFile.find(uri);
    return it != m_impl->byFile.end() ? it->second : std::vector<Diagnostic>{};
}

std::vector<Diagnostic> LSPDiagnostics::Filter(
        DiagnosticSeverity minSeverity) const {
    std::vector<Diagnostic> out;
    for (auto& [uri, diags] : m_impl->byFile)
        for (auto& d : diags)
            if (static_cast<int>(d.severity) <= static_cast<int>(minSeverity))
                out.push_back(d);
    return out;
}

std::vector<Diagnostic> LSPDiagnostics::All() const {
    std::vector<Diagnostic> out;
    for (auto& [uri, diags] : m_impl->byFile)
        out.insert(out.end(), diags.begin(), diags.end());
    return out;
}

std::vector<std::string> LSPDiagnostics::FilesWithDiagnostics() const {
    std::vector<std::string> uris;
    uris.reserve(m_impl->byFile.size());
    for (auto& [uri, _] : m_impl->byFile) uris.push_back(uri);
    return uris;
}

FileDiagnosticSummary LSPDiagnostics::SummaryFor(const std::string& uri) const {
    FileDiagnosticSummary s;
    s.uri = uri;
    auto it = m_impl->byFile.find(uri);
    if (it == m_impl->byFile.end()) return s;
    for (auto& d : it->second) {
        switch (d.severity) {
        case DiagnosticSeverity::Error:       ++s.errors;   break;
        case DiagnosticSeverity::Warning:     ++s.warnings; break;
        case DiagnosticSeverity::Information: ++s.infos;    break;
        case DiagnosticSeverity::Hint:        ++s.hints;    break;
        }
    }
    return s;
}

std::vector<FileDiagnosticSummary> LSPDiagnostics::AllSummaries() const {
    std::vector<FileDiagnosticSummary> out;
    for (auto& [uri, _] : m_impl->byFile) out.push_back(SummaryFor(uri));
    return out;
}

size_t LSPDiagnostics::TotalCount() const {
    size_t n = 0;
    for (auto& [uri, diags] : m_impl->byFile) n += diags.size();
    return n;
}

size_t LSPDiagnostics::ErrorCount() const {
    size_t n = 0;
    for (auto& [uri, diags] : m_impl->byFile)
        for (auto& d : diags) if (d.severity == DiagnosticSeverity::Error) ++n;
    return n;
}

size_t LSPDiagnostics::WarningCount() const {
    size_t n = 0;
    for (auto& [uri, diags] : m_impl->byFile)
        for (auto& d : diags) if (d.severity == DiagnosticSeverity::Warning) ++n;
    return n;
}

std::vector<Diagnostic> LSPDiagnostics::AtLine(const std::string& uri,
                                                uint32_t line) const {
    std::vector<Diagnostic> out;
    auto it = m_impl->byFile.find(uri);
    if (it == m_impl->byFile.end()) return out;
    for (auto& d : it->second)
        if (d.range.startLine <= line && line <= d.range.endLine)
            out.push_back(d);
    return out;
}

void LSPDiagnostics::OnDiagnosticsChanged(ChangedCb cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

} // namespace IDE
