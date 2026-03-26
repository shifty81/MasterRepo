#include "AI/ReasoningVisualizer/ReasoningVisualizer.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <unordered_map>

namespace AI {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct ReasoningVisualizer::Impl {
    std::unordered_map<uint64_t, ReasoningTrace>         traces;
    std::function<void(const ReasoningTrace&)>           onFinalized;
    uint64_t                                             nextId{1};
};

// ── Public API ────────────────────────────────────────────────────────────────

ReasoningVisualizer::ReasoningVisualizer() : m_impl(new Impl()) {}
ReasoningVisualizer::~ReasoningVisualizer() { delete m_impl; }

void ReasoningVisualizer::Init()     {}
void ReasoningVisualizer::Shutdown() { m_impl->traces.clear(); }

uint64_t ReasoningVisualizer::BeginTrace(const std::string& actionTag) {
    uint64_t id = m_impl->nextId++;
    ReasoningTrace& t = m_impl->traces[id];
    t.id          = id;
    t.actionTag   = actionTag;
    t.timestampMs = NowMs();
    return id;
}

void ReasoningVisualizer::AddEvidence(uint64_t traceId,
                                      const std::string& key,
                                      const std::string& value,
                                      float weight) {
    auto it = m_impl->traces.find(traceId);
    if (it == m_impl->traces.end()) return;
    it->second.evidence.push_back({key, value, weight});
}

void ReasoningVisualizer::AddDecisionStep(uint64_t traceId,
                                          const std::string& description,
                                          float confidence,
                                          const std::string& alternative,
                                          float alternativeScore) {
    auto it = m_impl->traces.find(traceId);
    if (it == m_impl->traces.end()) return;
    it->second.steps.push_back({description, confidence, alternative, alternativeScore});
}

void ReasoningVisualizer::Finalize(uint64_t traceId,
                                   const std::string& conclusion) {
    auto it = m_impl->traces.find(traceId);
    if (it == m_impl->traces.end()) return;
    it->second.conclusion = conclusion;
    it->second.finalized  = true;
    if (m_impl->onFinalized) m_impl->onFinalized(it->second);
}

ReasoningTrace ReasoningVisualizer::GetTrace(uint64_t traceId) const {
    auto it = m_impl->traces.find(traceId);
    if (it == m_impl->traces.end()) return {};
    return it->second;
}

std::vector<ReasoningTrace> ReasoningVisualizer::AllTraces() const {
    std::vector<ReasoningTrace> out;
    out.reserve(m_impl->traces.size());
    for (auto& [k, v] : m_impl->traces) out.push_back(v);
    return out;
}

std::vector<ReasoningTrace> ReasoningVisualizer::RecentTraces(uint32_t maxCount) const {
    auto all = AllTraces();
    std::sort(all.begin(), all.end(),
              [](const ReasoningTrace& a, const ReasoningTrace& b){
                  return a.timestampMs > b.timestampMs; });
    if (all.size() > maxCount) all.resize(maxCount);
    return all;
}

std::string ReasoningVisualizer::FormatTrace(uint64_t traceId) const {
    auto it = m_impl->traces.find(traceId);
    if (it == m_impl->traces.end()) return "(trace not found)";
    const ReasoningTrace& t = it->second;
    std::ostringstream ss;
    ss << "[ReasoningTrace] " << t.actionTag << " (id=" << t.id << ")\n";
    ss << "  Evidence:\n";
    for (auto& e : t.evidence)
        ss << "    " << e.key << " = " << e.value << " (w=" << e.weight << ")\n";
    ss << "  Steps:\n";
    for (size_t i = 0; i < t.steps.size(); ++i) {
        auto& s = t.steps[i];
        ss << "    " << (i+1) << ". " << s.description
           << " (conf=" << s.confidence << ")\n";
        if (!s.alternativeConsidered.empty())
            ss << "       alt: " << s.alternativeConsidered
               << " (score=" << s.alternativeScore << ")\n";
    }
    ss << "  Conclusion: " << t.conclusion << "\n";
    return ss.str();
}

std::string ReasoningVisualizer::SerializeTrace(uint64_t traceId) const {
    auto it = m_impl->traces.find(traceId);
    if (it == m_impl->traces.end()) return "{}";
    const ReasoningTrace& t = it->second;
    std::ostringstream ss;
    ss << "{\"id\":" << t.id
       << ",\"action\":\"" << t.actionTag << "\""
       << ",\"conclusion\":\"" << t.conclusion << "\""
       << ",\"finalized\":" << (t.finalized ? "true" : "false")
       << "}";
    return ss.str();
}

void ReasoningVisualizer::OnTraceFinalized(
    std::function<void(const ReasoningTrace&)> cb) {
    m_impl->onFinalized = std::move(cb);
}

void ReasoningVisualizer::Clear() { m_impl->traces.clear(); }

} // namespace AI
