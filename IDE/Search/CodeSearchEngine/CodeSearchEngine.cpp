#include "IDE/Search/CodeSearchEngine/CodeSearchEngine.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <regex>
#include <thread>
#include <unordered_map>

namespace fs = std::filesystem;

namespace IDE {

// ── Impl ──────────────────────────────────────────────────────────────────
struct CodeSearchEngine::Impl {
    // Index: filePath -> lines
    std::unordered_map<std::string, std::vector<std::string>> index;
    std::string rootPath;

    std::vector<IndexCompleteCb>  completeCbs;
    std::vector<IndexProgressCb>  progressCbs;

    mutable SearchStats lastSearch;
    IndexStats          lastIndex;

    // ── glob helpers ──────────────────────────────────────────
    static bool matchGlob(const std::string& pattern, const std::string& str) {
        if (pattern.empty()) return true;
        // Split by ';' and match any sub-pattern
        std::string p;
        for (size_t i = 0; i <= pattern.size(); ++i) {
            if (i == pattern.size() || pattern[i] == ';') {
                if (!p.empty() && matchSingle(p, str)) return true;
                p.clear();
            } else {
                p += pattern[i];
            }
        }
        return false;
    }

    static bool matchSingle(const std::string& pat, const std::string& str) {
        size_t pi = 0, si = 0;
        while (pi < pat.size() && si < str.size()) {
            if (pat[pi] == '*') {
                while (pi < pat.size() && pat[pi] == '*') ++pi;
                if (pi == pat.size()) return true;
                while (si < str.size()) {
                    if (matchSingle(pat.substr(pi), str.substr(si))) return true;
                    ++si;
                }
                return false;
            } else if (pat[pi] == '?' || pat[pi] == str[si]) {
                ++pi; ++si;
            } else {
                return false;
            }
        }
        while (pi < pat.size() && pat[pi] == '*') ++pi;
        return pi == pat.size() && si == str.size();
    }

    // ── line search ───────────────────────────────────────────
    static bool isWordBoundary(const std::string& s, size_t pos, size_t len) {
        bool before = (pos == 0 || !std::isalnum(static_cast<unsigned char>(s[pos-1])));
        bool after  = (pos+len >= s.size() || !std::isalnum(static_cast<unsigned char>(s[pos+len])));
        return before && after;
    }

    std::vector<SearchResult> searchLines(
        const std::string& filePath,
        const std::vector<std::string>& lines,
        const SearchOptions& opts) const
    {
        std::vector<SearchResult> results;
        for (uint32_t li = 0; li < static_cast<uint32_t>(lines.size()); ++li) {
            const std::string& line = lines[li];
            if (opts.useRegex) {
                try {
                    auto flags = std::regex::ECMAScript;
                    if (!opts.caseSensitive) flags |= std::regex::icase;
                    std::regex re(opts.query, flags);
                    std::sregex_iterator it(line.begin(), line.end(), re), end;
                    for (; it != end; ++it) {
                        if (results.size() >= opts.maxResults) return results;
                        SearchResult r;
                        r.filePath   = filePath;
                        r.line       = li + 1;
                        r.column     = static_cast<uint32_t>(it->position());
                        r.snippet    = line;
                        r.matchStart = r.column;
                        r.matchLen   = static_cast<uint32_t>((*it)[0].length());
                        results.push_back(r);
                    }
                } catch (...) {}
            } else {
                std::string haystack = line;
                std::string needle   = opts.query;
                if (!opts.caseSensitive) {
                    std::transform(haystack.begin(), haystack.end(), haystack.begin(),
                                   [](unsigned char c){ return std::tolower(c); });
                    std::transform(needle.begin(), needle.end(), needle.begin(),
                                   [](unsigned char c){ return std::tolower(c); });
                }
                size_t pos = 0;
                while ((pos = haystack.find(needle, pos)) != std::string::npos) {
                    if (results.size() >= opts.maxResults) return results;
                    if (!opts.wholeWord || isWordBoundary(haystack, pos, needle.size())) {
                        SearchResult r;
                        r.filePath   = filePath;
                        r.line       = li + 1;
                        r.column     = static_cast<uint32_t>(pos);
                        r.snippet    = line;
                        r.matchStart = r.column;
                        r.matchLen   = static_cast<uint32_t>(needle.size());
                        results.push_back(r);
                    }
                    pos += needle.empty() ? 1 : needle.size();
                }
            }
        }
        return results;
    }

    std::vector<SearchResult> doSearch(const SearchOptions& opts) const {
        auto t0 = std::chrono::high_resolution_clock::now();
        std::vector<SearchResult> all;
        uint32_t filesSearched = 0;

        for (const auto& [path, lines] : index) {
            if (all.size() >= opts.maxResults) break;
            // Scope filter
            if (opts.scope == SearchScope::CurrentFile || opts.scope == SearchScope::Folder) {
                if (path.find(opts.scopePath) == std::string::npos) continue;
            }
            // File glob filter
            std::string fname = fs::path(path).filename().string();
            if (!opts.fileGlob.empty() && !matchGlob(opts.fileGlob, fname)) continue;
            if (!opts.excludeGlob.empty() && matchGlob(opts.excludeGlob, path)) continue;

            auto res = searchLines(path, lines, opts);
            all.insert(all.end(), res.begin(), res.end());
            ++filesSearched;
        }

        if (all.size() > opts.maxResults) all.resize(opts.maxResults);

        auto t1 = std::chrono::high_resolution_clock::now();
        lastSearch.filesSearched = filesSearched;
        lastSearch.matchesFound  = static_cast<uint32_t>(all.size());
        lastSearch.searchTimeMs  = std::chrono::duration<double, std::milli>(t1-t0).count();
        return all;
    }
};

// ── Lifecycle ─────────────────────────────────────────────────────────────
CodeSearchEngine::CodeSearchEngine() : m_impl(new Impl()) {}
CodeSearchEngine::~CodeSearchEngine() { delete m_impl; }

// ── Indexing ──────────────────────────────────────────────────────────────
void CodeSearchEngine::Index(const std::string& rootPath) {
    auto t0 = std::chrono::high_resolution_clock::now();
    m_impl->rootPath = rootPath;
    m_impl->index.clear();

    std::vector<std::string> files;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(rootPath,
                fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) files.push_back(entry.path().string());
        }
    } catch (...) {}

    uint32_t total = static_cast<uint32_t>(files.size());
    for (uint32_t i = 0; i < total; ++i) {
        IndexFile(files[i]);
        if (!m_impl->progressCbs.empty())
            for (auto& cb : m_impl->progressCbs) cb(i+1, total);
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    m_impl->lastIndex.indexedFiles = static_cast<uint32_t>(m_impl->index.size());
    uint64_t lines = 0;
    for (auto& [_, v] : m_impl->index) lines += v.size();
    m_impl->lastIndex.indexedLines = lines;
    m_impl->lastIndex.indexTimeMs  = std::chrono::duration<double, std::milli>(t1-t0).count();
    for (auto& cb : m_impl->completeCbs) cb(m_impl->lastIndex);
}

void CodeSearchEngine::IndexFile(const std::string& filePath) {
    std::ifstream f(filePath);
    if (!f.is_open()) return;
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(f, line)) lines.push_back(line);
    m_impl->index[filePath] = std::move(lines);
}

void CodeSearchEngine::ClearIndex() { m_impl->index.clear(); }

uint32_t CodeSearchEngine::GetIndexedFileCount() const {
    return static_cast<uint32_t>(m_impl->index.size());
}
uint64_t CodeSearchEngine::GetIndexedLineCount() const {
    uint64_t n = 0;
    for (auto& [_, v] : m_impl->index) n += v.size();
    return n;
}
IndexStats CodeSearchEngine::GetIndexStats() const { return m_impl->lastIndex; }

// ── Search ────────────────────────────────────────────────────────────────
std::vector<SearchResult> CodeSearchEngine::Search(const SearchOptions& opts) const {
    return m_impl->doSearch(opts);
}

void CodeSearchEngine::SearchAsync(const SearchOptions& opts, SearchResultCb callback) const {
    std::thread([this, opts, cb = std::move(callback)]() {
        cb(m_impl->doSearch(opts));
    }).detach();
}

// ── Replace ───────────────────────────────────────────────────────────────
uint32_t CodeSearchEngine::ReplaceAll(const SearchOptions& opts, const std::string& replacement) {
    auto results = Search(opts);
    uint32_t count = 0;

    // Group by file
    std::unordered_map<std::string, std::vector<SearchResult*>> byFile;
    for (auto& r : results) byFile[r.filePath].push_back(&r);

    for (auto& [path, rlist] : byFile) {
        auto it = m_impl->index.find(path);
        if (it == m_impl->index.end()) continue;
        auto& lines = it->second;

        // Sort descending so column offsets remain valid after in-line replace
        std::sort(rlist.begin(), rlist.end(), [](const SearchResult* a, const SearchResult* b){
            if (a->line != b->line) return a->line > b->line;
            return a->column > b->column;
        });

        for (auto* r : rlist) {
            uint32_t li = r->line - 1;
            if (li >= lines.size()) continue;
            std::string& line = lines[li];
            if (r->matchStart + r->matchLen > line.size()) continue;
            line.replace(r->matchStart, r->matchLen, replacement);
            ++count;
        }

        // Write back to file
        std::ofstream f(path, std::ios::trunc);
        if (f.is_open()) {
            for (const auto& l : lines) f << l << '\n';
        }
    }
    return count;
}

// ── Callbacks ─────────────────────────────────────────────────────────────
void CodeSearchEngine::OnIndexComplete(IndexCompleteCb cb)  { m_impl->completeCbs.push_back(std::move(cb)); }
void CodeSearchEngine::OnIndexProgress(IndexProgressCb cb)  { m_impl->progressCbs.push_back(std::move(cb)); }

// ── Stats ─────────────────────────────────────────────────────────────────
SearchStats CodeSearchEngine::LastSearchStats() const { return m_impl->lastSearch; }

} // namespace IDE
