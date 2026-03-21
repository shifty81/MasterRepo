#include "AI/ConflictResolver/ConflictResolver.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace AI {

// ── Impl ─────────────────────────────────────────────────────────────────────
struct ConflictResolver::Impl {
    std::unordered_map<std::string, float>  weights;   // pipeline → weight
    // adjacency list: from → {to, ...} (dependency edges)
    std::unordered_map<std::string, std::unordered_set<std::string>> deps;
    std::vector<AISuggestion>     pending;
    std::vector<ConflictCallback> callbacks;
};

ConflictResolver::ConflictResolver() : m_impl(new Impl()) {}
ConflictResolver::~ConflictResolver() { delete m_impl; }

void ConflictResolver::SetPipelineWeight(const std::string& pipeline,
                                          float weight) {
    m_impl->weights[pipeline] = weight;
}

void ConflictResolver::AddDependency(const std::string& from,
                                      const std::string& to) {
    m_impl->deps[from].insert(to);
}

void ConflictResolver::ClearDependencies() { m_impl->deps.clear(); }

void ConflictResolver::Submit(const AISuggestion& suggestion) {
    m_impl->pending.push_back(suggestion);
}

void ConflictResolver::ClearPending() { m_impl->pending.clear(); }

std::vector<AISuggestion> ConflictResolver::GetPending() const {
    return m_impl->pending;
}

// ── scoring helper ────────────────────────────────────────────────────────────
static float Score(const AISuggestion& s,
                   const std::unordered_map<std::string, float>& weights)
{
    float w = 1.0f;
    auto it = weights.find(s.pipeline);
    if (it != weights.end()) w = it->second;
    return static_cast<float>(s.priority) * w + s.confidence * w;
}

// ── cycle detection (DFS) ─────────────────────────────────────────────────────
static bool HasCycle(
    const std::string& node,
    const std::unordered_map<std::string, std::unordered_set<std::string>>& adj,
    std::unordered_set<std::string>& visited,
    std::unordered_set<std::string>& stack)
{
    visited.insert(node);
    stack.insert(node);
    auto it = adj.find(node);
    if (it != adj.end()) {
        for (const auto& nb : it->second) {
            if (!visited.count(nb) &&
                HasCycle(nb, adj, visited, stack)) return true;
            if (stack.count(nb)) return true;
        }
    }
    stack.erase(node);
    return false;
}

std::vector<AISuggestion> ConflictResolver::Resolve() {
    if (m_impl->pending.empty()) return {};

    // Group by target (description used as proxy for target here).
    std::unordered_map<std::string, std::vector<AISuggestion*>> byTarget;
    for (auto& s : m_impl->pending)
        byTarget[s.description].push_back(&s);

    std::vector<AISuggestion> accepted;

    for (auto& [target, candidates] : byTarget) {
        if (candidates.size() == 1) {
            accepted.push_back(*candidates[0]);
            continue;
        }

        // Detect cyclic pipeline dependencies among candidates.
        std::unordered_set<std::string> visited, stack;
        bool cycleDetected = false;
        for (auto* s : candidates) {
            if (!visited.count(s->pipeline) &&
                HasCycle(s->pipeline, m_impl->deps, visited, stack)) {
                cycleDetected = true;
                break;
            }
        }

        // Sort by score descending; highest score wins.
        std::sort(candidates.begin(), candidates.end(),
            [&](const AISuggestion* a, const AISuggestion* b){
                return Score(*a, m_impl->weights) > Score(*b, m_impl->weights);
            });

        AISuggestion* winner = candidates[0];
        ResolutionResult res;
        res.hadConflict = true;
        res.winnerId    = winner->id;
        res.reason      = cycleDetected
                          ? "Cyclic dependency broken by priority score"
                          : "Higher priority/confidence score selected";
        for (size_t i = 1; i < candidates.size(); ++i)
            res.discardedIds.push_back(candidates[i]->id);

        accepted.push_back(*winner);
        for (auto& cb : m_impl->callbacks) cb(res);
    }

    m_impl->pending.clear();
    return accepted;
}

void ConflictResolver::OnConflict(ConflictCallback cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

} // namespace AI
