#include "Editor/Panels/ErrorPanel/ErrorPanel.h"
#include <sstream>
#include <algorithm>

namespace Editor {

// ─────────────────────────────────────────────────────────────────────────────
// Parsing helpers
// ─────────────────────────────────────────────────────────────────────────────

// Handles GCC/Clang format:  file:line:col: severity: message
// Also handles MSVC format:  file(line,col): severity Cxxxx: message
bool ErrorPanel::ParseLine(const std::string& line, ParsedError& out) const {
    out.rawLine = line;

    // ── GCC / Clang ──────────────────────────────────────────────────────────
    // Look for the pattern  <token>: (error|warning|note|info):
    // which is preceded by  file:line:col
    {
        // Find severity keyword
        static const std::vector<std::pair<std::string, ErrorSeverity>> kSeverities = {
            {": error: ",   ErrorSeverity::Error},
            {": warning: ", ErrorSeverity::Warning},
            {": note: ",    ErrorSeverity::Note},
            {": info: ",    ErrorSeverity::Info},
        };

        for (const auto& [kw, sev] : kSeverities) {
            auto sevPos = line.find(kw);
            if (sevPos == std::string::npos) continue;

            // Everything before sevPos should be  file:line:col
            std::string prefix = line.substr(0, sevPos);
            // Find last colon for column
            auto colColon = prefix.rfind(':');
            if (colColon == std::string::npos) continue;
            std::string colStr = prefix.substr(colColon + 1);
            // Find preceding colon for line
            auto lineColon = prefix.rfind(':', colColon > 0 ? colColon - 1 : 0);
            if (lineColon == std::string::npos) continue;
            std::string lineStr = prefix.substr(lineColon + 1, colColon - lineColon - 1);

            uint32_t lineNum = 0, colNum = 0;
            try { lineNum = static_cast<uint32_t>(std::stoul(lineStr)); } catch (...) { continue; }
            try { colNum  = static_cast<uint32_t>(std::stoul(colStr));  } catch (...) {}

            out.file    = prefix.substr(0, lineColon);
            out.line    = lineNum;
            out.column  = colNum;
            out.severity = sev;
            out.message  = line.substr(sevPos + kw.size());
            return true;
        }
    }

    // ── MSVC ─────────────────────────────────────────────────────────────────
    // Format: file(line,col): error Cxxxx: message
    //      or file(line): error Cxxxx: message
    {
        auto paren = line.find('(');
        if (paren != std::string::npos) {
            auto closeParen = line.find(')', paren);
            if (closeParen != std::string::npos) {
                std::string fileStr   = line.substr(0, paren);
                std::string lineColStr = line.substr(paren + 1, closeParen - paren - 1);
                std::string rest      = line.substr(closeParen + 1);

                uint32_t lineNum = 0, colNum = 0;
                auto comma = lineColStr.find(',');
                try {
                    lineNum = static_cast<uint32_t>(std::stoul(
                        comma != std::string::npos ? lineColStr.substr(0, comma) : lineColStr));
                    if (comma != std::string::npos)
                        colNum = static_cast<uint32_t>(std::stoul(lineColStr.substr(comma + 1)));
                } catch (...) {}

                ErrorSeverity sev = ErrorSeverity::Info;
                std::string msg;
                if (rest.find(": error") != std::string::npos) {
                    sev = ErrorSeverity::Error;
                    auto pos = rest.find(": error");
                    auto colon2 = rest.find(':', pos + 7);
                    msg = (colon2 != std::string::npos) ? rest.substr(colon2 + 2) : rest.substr(pos + 7);
                } else if (rest.find(": warning") != std::string::npos) {
                    sev = ErrorSeverity::Warning;
                    auto pos = rest.find(": warning");
                    auto colon2 = rest.find(':', pos + 9);
                    msg = (colon2 != std::string::npos) ? rest.substr(colon2 + 2) : rest.substr(pos + 9);
                } else if (rest.find(": note") != std::string::npos) {
                    sev = ErrorSeverity::Note;
                    auto pos = rest.find(": note");
                    msg = rest.substr(pos + 6);
                } else {
                    return false;
                }

                if (lineNum > 0 && !fileStr.empty()) {
                    out.file     = fileStr;
                    out.line     = lineNum;
                    out.column   = colNum;
                    out.severity = sev;
                    out.message  = msg;
                    return true;
                }
            }
        }
    }

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void ErrorPanel::ParseBuildOutput(const std::string& compilerOutput) {
    std::istringstream ss(compilerOutput);
    std::string line;
    while (std::getline(ss, line)) {
        ParsedError pe;
        if (ParseLine(line, pe))
            AddError(std::move(pe));
    }
}

void ErrorPanel::AddError(ParsedError e) {
    if (e.severity == ErrorSeverity::Error || e.severity == ErrorSeverity::Note)
        m_errors.push_back(e);
    else
        m_warnings.push_back(e);
}

void ErrorPanel::Clear() {
    m_errors.clear();
    m_warnings.clear();
}

const std::vector<ParsedError>& ErrorPanel::GetErrors()   const { return m_errors; }
const std::vector<ParsedError>& ErrorPanel::GetWarnings() const { return m_warnings; }

void ErrorPanel::SetNavigateCallback(NavigateCallback cb) {
    m_navigateCb = std::move(cb);
}

void ErrorPanel::NavigateTo(size_t errorIndex) {
    if (!m_navigateCb) return;
    std::vector<ParsedError> filtered = GetFiltered();
    if (errorIndex < filtered.size())
        m_navigateCb(filtered[errorIndex].file, filtered[errorIndex].line);
}

size_t ErrorPanel::ErrorCount()   const { return m_errors.size(); }
size_t ErrorPanel::WarningCount() const { return m_warnings.size(); }
bool   ErrorPanel::HasErrors()    const { return !m_errors.empty(); }

void ErrorPanel::SetFilter(ErrorSeverity minSeverity) {
    m_filterMin = minSeverity;
}

std::vector<ParsedError> ErrorPanel::GetFiltered() const {
    std::vector<ParsedError> results;
    auto include = [&](const std::vector<ParsedError>& src) {
        for (const auto& e : src) {
            if (static_cast<int>(e.severity) <= static_cast<int>(m_filterMin))
                results.push_back(e);
        }
    };
    include(m_errors);
    include(m_warnings);
    return results;
}

} // namespace Editor
