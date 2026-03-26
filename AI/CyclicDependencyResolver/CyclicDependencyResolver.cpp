#include "AI/CyclicDependencyResolver/CyclicDependencyResolver.h"
#include <algorithm>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <unordered_set>

namespace AI {

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct CyclicDependencyResolver::Impl {
    std::unordered_map<std::string, std::vector<DepEdge>>  adj;  // from -> edges
    std::unordered_set<std::string>                        nodes;
    CycleResolutionStrategy                                strategy{
        CycleResolutionStrategy::BreakWeakestEdge};
    std::function<void(const std::vector<std::string>&)>   onCycleDetected;
    std::function<void(const DepEdge&)>                    onEdgeBroken;

    // Tarjan SCC (iterative)
    std::vector<std::vector<std::string>> FindSCCs() const {
        std::unordered_map<std::string, int> index, lowlink;
        std::unordered_map<std::string, bool> onStack;
        std::stack<std::string>               stk;
        std::vector<std::vector<std::string>> sccs;
        int idx = 0;

        std::function<void(const std::string&)> strongConnect =
            [&](const std::string& v) {
                index[v] = lowlink[v] = idx++;
                stk.push(v);
                onStack[v] = true;
                auto it = adj.find(v);
                if (it != adj.end()) {
                    for (auto& edge : it->second) {
                        const std::string& w = edge.to;
                        if (index.find(w) == index.end()) {
                            strongConnect(w);
                            lowlink[v] = std::min(lowlink[v], lowlink[w]);
                        } else if (onStack[w]) {
                            lowlink[v] = std::min(lowlink[v], index[w]);
                        }
                    }
                }
                if (lowlink[v] == index[v]) {
                    std::vector<std::string> scc;
                    std::string w;
                    do {
                        w = stk.top(); stk.pop();
                        onStack[w] = false;
                        scc.push_back(w);
                    } while (w != v);
                    sccs.push_back(std::move(scc));
                }
            };

        for (auto& n : nodes)
            if (index.find(n) == index.end())
                strongConnect(n);
        return sccs;
    }
};

// ── Public API ────────────────────────────────────────────────────────────────

CyclicDependencyResolver::CyclicDependencyResolver() : m_impl(new Impl()) {}
CyclicDependencyResolver::~CyclicDependencyResolver() { delete m_impl; }

void CyclicDependencyResolver::Init()     {}
void CyclicDependencyResolver::Shutdown() { Clear(); }

void CyclicDependencyResolver::AddNode(const std::string& name) {
    m_impl->nodes.insert(name);
}

void CyclicDependencyResolver::RemoveNode(const std::string& name) {
    m_impl->nodes.erase(name);
    m_impl->adj.erase(name);
    for (auto& [k, edges] : m_impl->adj) {
        edges.erase(std::remove_if(edges.begin(), edges.end(),
                    [&](const DepEdge& e){ return e.to == name; }),
                    edges.end());
    }
}

void CyclicDependencyResolver::AddEdge(const std::string& from,
                                       const std::string& to,
                                       float weight) {
    m_impl->nodes.insert(from);
    m_impl->nodes.insert(to);
    m_impl->adj[from].push_back({from, to, weight});
}

void CyclicDependencyResolver::RemoveEdge(const std::string& from,
                                          const std::string& to) {
    auto it = m_impl->adj.find(from);
    if (it == m_impl->adj.end()) return;
    it->second.erase(
        std::remove_if(it->second.begin(), it->second.end(),
                       [&](const DepEdge& e){ return e.to == to; }),
        it->second.end());
}

void CyclicDependencyResolver::Clear() {
    m_impl->adj.clear();
    m_impl->nodes.clear();
}

void CyclicDependencyResolver::SetStrategy(CycleResolutionStrategy s) {
    m_impl->strategy = s;
}

CycleResolutionStrategy CyclicDependencyResolver::GetStrategy() const {
    return m_impl->strategy;
}

ResolveResult CyclicDependencyResolver::Detect() const {
    ResolveResult result;
    auto sccs = m_impl->FindSCCs();
    for (auto& scc : sccs) {
        if (scc.size() > 1) {
            result.cyclesFound = true;
            result.cycles.push_back(scc);
        }
    }
    result.summary = result.cyclesFound
        ? ("Detected " + std::to_string(result.cycles.size()) + " cycle(s).")
        : "No cycles detected.";
    return result;
}

ResolveResult CyclicDependencyResolver::Resolve() {
    ResolveResult result = Detect();
    if (!result.cyclesFound) return result;

    if (m_impl->onCycleDetected) {
        for (auto& cycle : result.cycles) m_impl->onCycleDetected(cycle);
    }

    if (m_impl->strategy == CycleResolutionStrategy::NotifyOnly) return result;

    for (auto& cycle : result.cycles) {
        if (m_impl->strategy == CycleResolutionStrategy::BreakWeakestEdge) {
            DepEdge weakest{"","",1e9f};
            for (size_t i = 0; i < cycle.size(); ++i) {
                const std::string& from = cycle[i];
                const std::string& to   = cycle[(i+1) % cycle.size()];
                auto it = m_impl->adj.find(from);
                if (it == m_impl->adj.end()) continue;
                for (auto& e : it->second)
                    if (e.to == to && e.weight < weakest.weight) weakest = e;
            }
            if (!weakest.from.empty()) {
                RemoveEdge(weakest.from, weakest.to);
                result.brokenEdges.push_back(weakest);
                if (m_impl->onEdgeBroken) m_impl->onEdgeBroken(weakest);
            }
        } else { // ExcludeLatest — remove last node in cycle
            const std::string& last = cycle.front();
            RemoveNode(last);
        }
    }

    result.summary = "Resolved: broke " +
                     std::to_string(result.brokenEdges.size()) + " edge(s).";
    return result;
}

void CyclicDependencyResolver::OnCycleDetected(
    std::function<void(const std::vector<std::string>&)> cb) {
    m_impl->onCycleDetected = std::move(cb);
}

void CyclicDependencyResolver::OnEdgeBroken(
    std::function<void(const DepEdge&)> cb) {
    m_impl->onEdgeBroken = std::move(cb);
}

std::vector<std::string> CyclicDependencyResolver::TopologicalOrder() const {
    auto det = Detect();
    if (det.cyclesFound) return {};

    std::vector<std::string> order;
    std::unordered_map<std::string, int> inDegree;
    for (auto& n : m_impl->nodes) inDegree[n] = 0;
    for (auto& [from, edges] : m_impl->adj)
        for (auto& e : edges) inDegree[e.to]++;

    std::vector<std::string> queue;
    for (auto& [n, d] : inDegree) if (d == 0) queue.push_back(n);
    while (!queue.empty()) {
        std::string cur = queue.back(); queue.pop_back();
        order.push_back(cur);
        auto it = m_impl->adj.find(cur);
        if (it != m_impl->adj.end())
            for (auto& e : it->second)
                if (--inDegree[e.to] == 0) queue.push_back(e.to);
    }
    return order;
}

std::string CyclicDependencyResolver::DotGraph() const {
    std::ostringstream ss;
    ss << "digraph G {\n";
    for (auto& [from, edges] : m_impl->adj)
        for (auto& e : edges)
            ss << "  \"" << from << "\" -> \"" << e.to << "\";\n";
    ss << "}\n";
    return ss.str();
}

} // namespace AI
