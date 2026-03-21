#pragma once
/**
 * @file SymbolLocator.h
 * @brief Fast in-project symbol index: functions, classes, defines; fuzzy search.
 *
 * SymbolLocator scans C/C++ source files and builds an index of code symbols:
 *   - SymbolKind: Function, Class/Struct, Enum, Define, Namespace, Variable
 *   - SymbolEntry: symbol name, kind, file path, line number, signature
 *   - Index is built by scanning files for well-known lexical patterns (regex).
 *   - FuzzySearch: substring + simple trigram ranking for quick lookup.
 *   - GoToDefinition(name): returns best-match SymbolEntry.
 *   - IncrementalReindex(file): reindex a single changed file.
 *   - OnIndexReady: callback fired when initial indexing completes.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Tools {

// ── Symbol kind ────────────────────────────────────────────────────────────
enum class SymbolKind : uint8_t {
    Function, Class, Struct, Enum, Define, Namespace, Variable, Unknown
};
const char* SymbolKindName(SymbolKind k);

// ── Symbol entry ───────────────────────────────────────────────────────────
struct SymbolEntry {
    std::string name;
    SymbolKind  kind{SymbolKind::Unknown};
    std::string filePath;
    uint32_t    line{0};
    std::string signature;    ///< Full declaration line
    float       score{0.0f};  ///< Relevance score (filled by search)
};

// ── Index stats ────────────────────────────────────────────────────────────
struct SymbolIndexStats {
    size_t filesIndexed{0};
    size_t symbolCount{0};
    double indexTimeMs{0};
};

// ── Locator ────────────────────────────────────────────────────────────────
class SymbolLocator {
public:
    SymbolLocator();
    ~SymbolLocator();

    // ── scan config ───────────────────────────────────────────
    void AddSearchRoot(const std::string& dir);
    void AddExtension(const std::string& ext);  ///< e.g. ".h", ".cpp"
    void SetMaxFileSize(size_t bytes);

    // ── index build ───────────────────────────────────────────
    void BuildIndex();
    void IncrementalReindex(const std::string& filePath);
    void ClearIndex();
    bool IsIndexed() const;
    SymbolIndexStats Stats() const;

    // ── search ────────────────────────────────────────────────
    /// Fuzzy search: returns up to maxResults entries sorted by score.
    std::vector<SymbolEntry> Search(const std::string& query,
                                    size_t maxResults = 20) const;

    /// Exact or best-match go-to-definition.
    SymbolEntry GoToDefinition(const std::string& name) const;

    /// All symbols in a specific file.
    std::vector<SymbolEntry> SymbolsInFile(const std::string& filePath) const;

    /// All symbols of a given kind.
    std::vector<SymbolEntry> ByKind(SymbolKind kind) const;

    // ── callbacks ─────────────────────────────────────────────
    using IndexReadyCb = std::function<void(const SymbolIndexStats&)>;
    void OnIndexReady(IndexReadyCb cb);

    using ProgressCb = std::function<void(size_t done, size_t total)>;
    void OnProgress(ProgressCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Tools
