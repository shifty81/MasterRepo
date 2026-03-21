#pragma once
/**
 * @file DependencyAnalyzer.h
 * @brief Parse #include directives, build an include graph, detect cycles.
 *
 * DependencyAnalyzer scans C/C++ source trees and builds an include dependency graph:
 *   - IncludeNode: file path with its list of direct includes
 *   - IncludeGraph: adjacency map (file → set<file>)
 *   - Cycle detection via iterative DFS with coloring (white/gray/black)
 *   - Metrics: fanout (includes per file), fanin (included by), transitive depth
 *   - Report: text summary or DOT format for Graphviz
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace Tools {

// ── Graph node ────────────────────────────────────────────────────────────
struct IncludeNode {
    std::string              filePath;
    std::vector<std::string> directIncludes;   ///< As found in the source
    std::vector<std::string> resolvedIncludes; ///< Full paths after resolution
    uint32_t                 fanout{0};         ///< # files this includes
    uint32_t                 fanin{0};          ///< # files that include this
    uint32_t                 depth{0};          ///< Max transitive include depth
};

// ── Cycle record ──────────────────────────────────────────────────────────
struct IncludeCycle {
    std::vector<std::string> chain; ///< Files forming the cycle
    std::string Description() const;
};

// ── Analysis result ───────────────────────────────────────────────────────
struct DependencyReport {
    std::unordered_map<std::string, IncludeNode> nodes;
    std::vector<IncludeCycle>                    cycles;
    std::vector<std::string>                     roots;   ///< Files not included by others
    std::vector<std::string>                     leaves;  ///< Files that include nothing

    size_t FileCount()  const { return nodes.size(); }
    size_t CycleCount() const { return cycles.size(); }
    bool   HasCycles()  const { return !cycles.empty(); }

    std::string TopFanout(size_t n = 10) const; ///< Top-N by fanout
    std::string TopFanin(size_t n = 10)  const; ///< Top-N by fanin
};

// ── Analyzer ──────────────────────────────────────────────────────────────
class DependencyAnalyzer {
public:
    DependencyAnalyzer();
    ~DependencyAnalyzer();

    // ── scan configuration ────────────────────────────────────
    void AddSearchRoot(const std::string& dir);
    void AddIncludePath(const std::string& dir);  ///< For resolving angle-bracket includes
    void AddExtension(const std::string& ext);    ///< e.g. ".cpp", ".h"
    void SetFollowSystemIncludes(bool follow);    ///< If false, skip <...>

    // ── analyse ───────────────────────────────────────────────
    DependencyReport Analyse();
    DependencyReport AnalyseFile(const std::string& path);

    // ── query helpers ─────────────────────────────────────────
    /// Find all files that transitively depend on @p targetFile.
    std::vector<std::string> FindDependents(const std::string& targetFile,
                                             const DependencyReport& report) const;
    /// Find all transitive dependencies of @p sourceFile.
    std::vector<std::string> FindDependencies(const std::string& sourceFile,
                                               const DependencyReport& report) const;

    // ── report formats ────────────────────────────────────────
    std::string ExportText(const DependencyReport& report) const;
    std::string ExportDOT(const DependencyReport& report,
                          const std::string& graphName = "includes") const;

    // ── progress ──────────────────────────────────────────────
    using ProgressCb = std::function<void(size_t scanned, size_t total)>;
    void OnProgress(ProgressCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Tools
