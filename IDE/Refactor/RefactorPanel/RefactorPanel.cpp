#include "IDE/Refactor/RefactorPanel/RefactorPanel.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>

namespace IDE {

// ── Internal index: maps filename → lines ─────────────────────────────────
struct RefactorPanel::Impl {
    std::unordered_map<std::string, std::vector<std::string>> index;
    RefactorStats   stats;
    RefactorResult  lastApplied;
    bool            hasUndo{false};

    ApplyCompleteCb onApplyCb;
    IndexReadyCb    onIndexCb;

    // Read file into lines
    static std::vector<std::string> ReadLines(const std::string& path) {
        std::vector<std::string> lines;
        std::ifstream f(path);
        if (!f.is_open()) return lines;
        std::string line;
        while (std::getline(f, line)) lines.push_back(line);
        return lines;
    }

    // Write lines back to file
    static bool WriteLines(const std::string& path,
                           const std::vector<std::string>& lines) {
        std::ofstream f(path, std::ios::trunc);
        if (!f.is_open()) return false;
        for (size_t i = 0; i < lines.size(); ++i) {
            f << lines[i];
            if (i + 1 < lines.size()) f << '\n';
        }
        return true;
    }

    // Apply a single TextEdit in-memory (on a lines vector)
    static bool ApplyEdit(std::vector<std::string>& lines, const TextEdit& edit) {
        if (edit.startLine < 1 || edit.startLine > lines.size()) return false;
        uint32_t sl = edit.startLine - 1; // 0-based
        std::string& line = lines[sl];
        uint32_t sc = std::min(edit.startCol, static_cast<uint32_t>(line.size()));
        uint32_t ec = std::min(edit.endCol,   static_cast<uint32_t>(line.size()));
        line.replace(sc, ec - sc, edit.newText);
        return true;
    }

    RefactorResult DoRename(const RefactorRequest& req) const {
        RefactorResult res;
        // Find symbol at location to determine old name
        auto fit = index.find(req.location.filePath);
        if (fit == index.end()) {
            res.diagnostics.push_back("File not indexed: " + req.location.filePath);
            return res;
        }
        const auto& lines = fit->second;
        if (req.location.line < 1 || req.location.line > lines.size()) {
            res.diagnostics.push_back("Line out of range");
            return res;
        }
        const std::string& srcLine = lines[req.location.line - 1];
        // Extract word at column
        uint32_t col = std::min(req.location.column, static_cast<uint32_t>(srcLine.size()));
        uint32_t lo = col, hi = col;
        while (lo > 0 && (std::isalnum(srcLine[lo-1]) || srcLine[lo-1] == '_')) --lo;
        while (hi < srcLine.size() && (std::isalnum(srcLine[hi]) || srcLine[hi] == '_')) ++hi;
        if (lo == hi) { res.diagnostics.push_back("No identifier at location"); return res; }
        std::string symbolOld = srcLine.substr(lo, hi - lo);

        // Produce rename edits across all indexed files
        for (const auto& [fpath, flines] : index) {
            std::regex re("\\b" + symbolOld + "\\b");
            for (uint32_t i = 0; i < flines.size(); ++i) {
                auto it = std::sregex_iterator(flines[i].begin(), flines[i].end(), re);
                std::sregex_iterator end;
                // Collect matches in reverse so column offsets stay valid
                std::vector<std::pair<uint32_t,uint32_t>> matches;
                for (; it != end; ++it) matches.push_back({
                    static_cast<uint32_t>(it->position()),
                    static_cast<uint32_t>(it->length())});
                for (auto& m : matches) {
                    TextEdit te;
                    te.filePath   = fpath;
                    te.startLine  = i + 1;
                    te.startCol   = m.first;
                    te.endLine    = i + 1;
                    te.endCol     = m.first + m.second;
                    te.newText    = req.newName;
                    res.edits.push_back(te);
                }
            }
        }
        res.success = !res.edits.empty();
        return res;
    }

    RefactorResult DoExtractVariable(const RefactorRequest& req) const {
        RefactorResult res;
        auto fit = index.find(req.location.filePath);
        if (fit == index.end()) {
            res.diagnostics.push_back("File not indexed: " + req.location.filePath);
            return res;
        }
        if (req.scopeStartLine < 1 || req.scopeEndLine < req.scopeStartLine) {
            res.diagnostics.push_back("Invalid scope lines");
            return res;
        }
        const auto& lines = fit->second;
        if (req.scopeEndLine > lines.size()) {
            res.diagnostics.push_back("Scope end line out of range");
            return res;
        }
        // Build extracted expression (join selected lines)
        std::string expr;
        for (uint32_t i = req.scopeStartLine; i <= req.scopeEndLine; ++i)
            expr += lines[i-1] + (i < req.scopeEndLine ? " " : "");

        // Insert declaration before scopeStartLine
        std::string decl = "auto " + req.newName + " = " + expr + ";";
        TextEdit insert;
        insert.filePath  = req.location.filePath;
        insert.startLine = req.scopeStartLine;
        insert.startCol  = 0;
        insert.endLine   = req.scopeStartLine;
        insert.endCol    = 0;
        insert.newText   = decl + "\n";
        res.edits.push_back(insert);
        res.success = true;
        return res;
    }
};

// ── Constructor / destructor ──────────────────────────────────────────────
RefactorPanel::RefactorPanel()  : m_impl(std::make_unique<Impl>()) {}
RefactorPanel::~RefactorPanel() = default;

// ── Indexing ──────────────────────────────────────────────────────────────
void RefactorPanel::IndexFiles(const std::vector<std::string>& paths) {
    for (const auto& p : paths) IndexFile(p);
    if (m_impl->onIndexCb)
        m_impl->onIndexCb(static_cast<uint32_t>(m_impl->index.size()));
}

void RefactorPanel::IndexFile(const std::string& path) {
    auto lines = Impl::ReadLines(path);
    if (!lines.empty()) {
        m_impl->index[path] = std::move(lines);
        ++m_impl->stats.indexedFiles;
    }
}

uint32_t RefactorPanel::GetIndexedFileCount() const {
    return static_cast<uint32_t>(m_impl->index.size());
}

// ── Preview ───────────────────────────────────────────────────────────────
RefactorResult RefactorPanel::Preview(const RefactorRequest& req) const {
    switch (req.kind) {
    case RefactorKind::Rename:          return m_impl->DoRename(req);
    case RefactorKind::ExtractVariable: return m_impl->DoExtractVariable(req);
    default: {
        RefactorResult r;
        r.diagnostics.push_back("Refactor kind not yet implemented");
        return r;
    }
    }
}

// ── Apply ─────────────────────────────────────────────────────────────────
RefactorResult RefactorPanel::Apply(const RefactorRequest& req) {
    using Clock = std::chrono::steady_clock;
    auto t0 = Clock::now();

    RefactorResult result = Preview(req);
    if (!result.success) {
        ++m_impl->stats.failedRefactors;
        return result;
    }

    // Apply edits to in-memory index and persist
    // Group edits by file
    std::unordered_map<std::string, std::vector<TextEdit>> byFile;
    for (const auto& ed : result.edits) byFile[ed.filePath].push_back(ed);

    for (auto& [fpath, edits] : byFile) {
        auto fit = m_impl->index.find(fpath);
        if (fit == m_impl->index.end()) continue;
        auto& lines = fit->second;
        // Apply edits in reverse order to preserve line/col offsets
        std::stable_sort(edits.begin(), edits.end(),
            [](const TextEdit& a, const TextEdit& b){
                if (a.startLine != b.startLine) return a.startLine > b.startLine;
                return a.startCol > b.startCol;
            });
        for (const auto& ed : edits) Impl::ApplyEdit(lines, ed);
        Impl::WriteLines(fpath, lines);
    }

    m_impl->lastApplied = result;
    m_impl->hasUndo     = true;
    ++m_impl->stats.totalRefactors;

    auto t1 = Clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    // Rolling average
    double total = (m_impl->stats.totalRefactors - 1) * m_impl->stats.avgRefactorMs + ms;
    m_impl->stats.avgRefactorMs = total / m_impl->stats.totalRefactors;

    if (m_impl->onApplyCb) m_impl->onApplyCb(result);
    return result;
}

// ── Undo ──────────────────────────────────────────────────────────────────
bool RefactorPanel::Undo() {
    // Single-level undo: re-index affected files from disk
    if (!m_impl->hasUndo) return false;
    std::unordered_set<std::string> affected;
    for (const auto& ed : m_impl->lastApplied.edits)
        affected.insert(ed.filePath);
    for (const auto& f : affected) {
        auto lines = Impl::ReadLines(f);
        if (!lines.empty()) m_impl->index[f] = std::move(lines);
    }
    m_impl->hasUndo = false;
    return true;
}

// ── Available refactors ───────────────────────────────────────────────────
std::vector<RefactorKind> RefactorPanel::GetAvailableRefactors(
    const RefactorLocation& loc) const {
    std::vector<RefactorKind> out;
    out.push_back(RefactorKind::Rename);
    if (!loc.filePath.empty()) out.push_back(RefactorKind::ExtractVariable);
    out.push_back(RefactorKind::ExtractFunction);
    out.push_back(RefactorKind::InlineVariable);
    out.push_back(RefactorKind::MoveDeclaration);
    out.push_back(RefactorKind::ChangeSignature);
    return out;
}

// ── Callbacks ─────────────────────────────────────────────────────────────
void RefactorPanel::OnApplyComplete(ApplyCompleteCb cb) { m_impl->onApplyCb = std::move(cb); }
void RefactorPanel::OnIndexReady   (IndexReadyCb    cb) { m_impl->onIndexCb = std::move(cb); }

// ── Stats ─────────────────────────────────────────────────────────────────
RefactorStats RefactorPanel::GetStats()  const { return m_impl->stats; }
void          RefactorPanel::ResetStats()      { m_impl->stats = {}; }

} // namespace IDE
