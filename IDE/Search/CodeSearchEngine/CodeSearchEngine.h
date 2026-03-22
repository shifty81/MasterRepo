#pragma once
/**
 * @file CodeSearchEngine.h
 * @brief Code search engine with file indexing, pattern matching, and async search.
 *
 * CodeSearchEngine scans a workspace, builds an in-memory line index, and
 * provides fast search/replace over source files:
 *
 *   SearchScope: All, CurrentFile, Folder.
 *   SearchOptions: query, caseSensitive, wholeWord, useRegex, scope, maxResults,
 *                  fileGlob (e.g. "*.cpp;*.h"), excludeGlob.
 *   SearchResult: filePath, line (1-based), column (0-based), snippet (full line),
 *                 matchStart, matchLen.
 *
 *   CodeSearchEngine:
 *     - Index(rootPath): scan the workspace and build the line index.
 *     - IndexFile(path): (re-)index a single file incrementally.
 *     - Search(options): synchronous search; returns sorted results.
 *     - SearchAsync(options, callback): background search.
 *     - ReplaceAll(options, replacement): in-place replacement across files;
 *       returns the number of replacements made.
 *     - GetIndexedFileCount() / GetIndexedLineCount().
 *     - OnIndexComplete(cb) / OnProgress(cb).
 *     - SearchStats: filesSearched, matchesFound, searchTimeMs.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace IDE {

// ── Search scope ──────────────────────────────────────────────────────────
enum class SearchScope : uint8_t { All, CurrentFile, Folder };

// ── Search options ────────────────────────────────────────────────────────
struct SearchOptions {
    std::string  query;
    bool         caseSensitive{false};
    bool         wholeWord{false};
    bool         useRegex{false};
    SearchScope  scope{SearchScope::All};
    std::string  scopePath;      ///< Used when scope == Folder or CurrentFile
    std::string  fileGlob;       ///< e.g. "*.cpp;*.h" — empty = all files
    std::string  excludeGlob;    ///< e.g. "*/build/*"
    uint32_t     maxResults{500};
};

// ── Search result ─────────────────────────────────────────────────────────
struct SearchResult {
    std::string filePath;
    uint32_t    line{0};        ///< 1-based line number
    uint32_t    column{0};      ///< 0-based column of first match char
    std::string snippet;        ///< Full source line (trimmed)
    uint32_t    matchStart{0};  ///< Offset within snippet
    uint32_t    matchLen{0};
};

// ── Stats ─────────────────────────────────────────────────────────────────
struct SearchStats {
    uint32_t filesSearched{0};
    uint32_t matchesFound{0};
    double   searchTimeMs{0.0};
};

struct IndexStats {
    uint32_t indexedFiles{0};
    uint64_t indexedLines{0};
    double   indexTimeMs{0.0};
};

// ── Callbacks ─────────────────────────────────────────────────────────────
using SearchResultCb    = std::function<void(std::vector<SearchResult>)>;
using IndexCompleteCb   = std::function<void(const IndexStats&)>;
using IndexProgressCb   = std::function<void(uint32_t filesProcessed, uint32_t totalFiles)>;

// ── CodeSearchEngine ──────────────────────────────────────────────────────
class CodeSearchEngine {
public:
    CodeSearchEngine();
    ~CodeSearchEngine();

    CodeSearchEngine(const CodeSearchEngine&) = delete;
    CodeSearchEngine& operator=(const CodeSearchEngine&) = delete;

    // ── indexing ──────────────────────────────────────────────
    void Index(const std::string& rootPath);
    void IndexFile(const std::string& filePath);
    void ClearIndex();

    uint32_t GetIndexedFileCount() const;
    uint64_t GetIndexedLineCount() const;
    IndexStats GetIndexStats() const;

    // ── search ────────────────────────────────────────────────
    std::vector<SearchResult> Search(const SearchOptions& opts) const;
    void SearchAsync(const SearchOptions& opts, SearchResultCb callback) const;

    // ── replace ───────────────────────────────────────────────
    uint32_t ReplaceAll(const SearchOptions& opts, const std::string& replacement);

    // ── callbacks ─────────────────────────────────────────────
    void OnIndexComplete(IndexCompleteCb cb);
    void OnIndexProgress(IndexProgressCb cb);

    // ── stats ─────────────────────────────────────────────────
    SearchStats LastSearchStats() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace IDE
