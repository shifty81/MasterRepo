#include "AI/DecisionVisualizer/DecisionVisualizer.h"
#include <algorithm>
#include <unordered_map>
#include <random>
#include <sstream>
#include <iomanip>
#include <mutex>

namespace AI {

// ── helpers ──────────────────────────────────────────────────────────────────
static std::string MakeId() {
    static std::mt19937_64 rng{std::random_device{}()};
    static std::mutex      rngMutex;
    std::uniform_int_distribution<uint64_t> dist;
    std::unique_lock<std::mutex> lock(rngMutex);
    uint64_t val = dist(rng);
    lock.unlock();
    std::ostringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << val;
    return ss.str();
}

// ── Impl ─────────────────────────────────────────────────────────────────────
struct DecisionVisualizer::Impl {
    std::unordered_map<std::string, DecisionTrace> pending;
    std::vector<DecisionTrace>                     history;
    std::vector<TraceCallback>                     callbacks;
};

DecisionVisualizer::DecisionVisualizer() : m_impl(new Impl()) {}
DecisionVisualizer::~DecisionVisualizer() { delete m_impl; }

std::string DecisionVisualizer::BeginTrace(const std::string& actionTaken) {
    std::string id = MakeId();
    DecisionTrace t;
    t.decisionId  = id;
    t.actionTaken = actionTaken;
    t.overallConf = 0.0f;
    m_impl->pending[id] = std::move(t);
    return id;
}

void DecisionVisualizer::AddStep(const std::string& decisionId,
                                  const ReasoningStep& step) {
    auto it = m_impl->pending.find(decisionId);
    if (it == m_impl->pending.end()) return;
    it->second.steps.push_back(step);
    // Update running average confidence.
    float sum = 0.0f;
    for (auto& s : it->second.steps) sum += s.confidence;
    it->second.overallConf = sum / static_cast<float>(it->second.steps.size());
}

void DecisionVisualizer::CommitTrace(const std::string& decisionId) {
    auto it = m_impl->pending.find(decisionId);
    if (it == m_impl->pending.end()) return;
    DecisionTrace t = std::move(it->second);
    m_impl->pending.erase(it);
    m_impl->history.insert(m_impl->history.begin(), t);
    for (auto& cb : m_impl->callbacks) cb(t);
}

void DecisionVisualizer::DiscardTrace(const std::string& decisionId) {
    m_impl->pending.erase(decisionId);
}

std::vector<DecisionTrace>
DecisionVisualizer::GetRecentTraces(size_t maxCount) const {
    if (m_impl->history.size() <= maxCount) return m_impl->history;
    return std::vector<DecisionTrace>(m_impl->history.begin(),
                                      m_impl->history.begin() +
                                          static_cast<std::ptrdiff_t>(maxCount));
}

DecisionTrace
DecisionVisualizer::GetTrace(const std::string& decisionId) const {
    for (auto& t : m_impl->history)
        if (t.decisionId == decisionId) return t;
    return {};
}

void DecisionVisualizer::ClearHistory() { m_impl->history.clear(); }

void DecisionVisualizer::OnTrace(TraceCallback cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

} // namespace AI
