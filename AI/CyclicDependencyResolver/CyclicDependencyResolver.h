#pragma once
/**
 * @file CyclicDependencyResolver.h
 * @brief Detects and resolves cyclic dependencies in AI suggestion graphs,
 *        PCG rule sets, and build-order dependency lists.
 *
 * Uses Tarjan's SCC algorithm for cycle detection and a configurable
 * resolution strategy (break weakest edge, exclude later arrival, or notify).
 *
 * Typical usage:
 * @code
 *   CyclicDependencyResolver cdr;
 *   cdr.Init();
 *   cdr.AddNode("ShipHull");
 *   cdr.AddNode("EngineThruster");
 *   cdr.AddEdge("ShipHull", "EngineThruster");
 *   cdr.AddEdge("EngineThruster", "ShipHull");  // cycle!
 *   auto result = cdr.Resolve();
 *   // result.cyclesFound == true, result.brokenEdges contains the removed edge
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace AI {

// ── Strategy for breaking a detected cycle ───────────────────────────────────

enum class CycleResolutionStrategy : uint8_t {
    BreakWeakestEdge = 0, ///< remove the edge with the lowest weight
    ExcludeLatest    = 1, ///< drop the most-recently added node in the cycle
    NotifyOnly       = 2, ///< report but do not modify graph
};

// ── A dependency edge in the graph ────────────────────────────────────────────

struct DepEdge {
    std::string from;
    std::string to;
    float       weight{1.0f}; ///< higher weight = less likely to be broken
};

// ── Result of a resolve pass ──────────────────────────────────────────────────

struct ResolveResult {
    bool                  cyclesFound{false};
    std::vector<std::vector<std::string>> cycles;  ///< each inner vec = one SCC
    std::vector<DepEdge>  brokenEdges;             ///< edges removed to fix cycles
    std::string           summary;
};

// ── CyclicDependencyResolver ──────────────────────────────────────────────────

class CyclicDependencyResolver {
public:
    CyclicDependencyResolver();
    ~CyclicDependencyResolver();

    void Init();
    void Shutdown();

    // ── Graph construction ────────────────────────────────────────────────────

    void AddNode(const std::string& name);
    void RemoveNode(const std::string& name);
    void AddEdge(const std::string& from, const std::string& to,
                 float weight = 1.0f);
    void RemoveEdge(const std::string& from, const std::string& to);
    void Clear();

    // ── Resolution ───────────────────────────────────────────────────────────

    void SetStrategy(CycleResolutionStrategy strategy);
    CycleResolutionStrategy GetStrategy() const;

    /// Run cycle detection and resolution in one step.
    ResolveResult Resolve();

    /// Only detect cycles without modifying the graph.
    ResolveResult Detect() const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    /// Called whenever a cycle is detected (before resolution).
    void OnCycleDetected(std::function<void(const std::vector<std::string>&)> cb);

    /// Called after an edge is broken during resolution.
    void OnEdgeBroken(std::function<void(const DepEdge&)> cb);

    // ── Utilities ─────────────────────────────────────────────────────────────

    std::vector<std::string> TopologicalOrder() const; ///< empty if cycles remain
    std::string              DotGraph()         const; ///< GraphViz DOT format

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace AI
