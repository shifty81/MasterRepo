#include "Editor/Panels/AIProfiler/AIProfilerPanel.h"
#include <algorithm>
#include <chrono>
#include <numeric>
#include <vector>

namespace Editor {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct AIProfilerPanel::Impl {
    bool                          visible{true};
    uint32_t                      historyDepth{128};
    bool                          showAlerts{true};
    std::vector<AIProfileSample>  samples;
    float                         peakAI{0.f};
    float                         peakPCG{0.f};
};

AIProfilerPanel::AIProfilerPanel() : m_impl(new Impl()) {}
AIProfilerPanel::~AIProfilerPanel() { delete m_impl; }

void AIProfilerPanel::Init()     {}
void AIProfilerPanel::Shutdown() { Clear(); }

void AIProfilerPanel::PushSample(float aiCpuMs, float pcgCpuMs,
                                  uint32_t queueDepth) {
    AIProfileSample s;
    s.aiCpuMs   = aiCpuMs;
    s.pcgCpuMs  = pcgCpuMs;
    s.queueDepth = queueDepth;
    s.timestampMs = NowMs();
    m_impl->samples.push_back(s);
    if (m_impl->samples.size() > m_impl->historyDepth)
        m_impl->samples.erase(m_impl->samples.begin());
    m_impl->peakAI  = std::max(m_impl->peakAI,  aiCpuMs);
    m_impl->peakPCG = std::max(m_impl->peakPCG, pcgCpuMs);
}

void AIProfilerPanel::Draw() {
    if (!m_impl->visible) return;

    // Atlas rule: custom UI only — no ImGui.
    // The EditorRenderer calls this method and interprets the panel via
    // its own 2D drawing primitives.  This stub outputs a text summary
    // that the renderer can display in the panel area.
    //
    // Full integration: EditorRenderer queries Samples(), PeakAICpuMs(),
    // AvgAICpuMs() etc. and draws bar graphs / sparklines.
}

bool AIProfilerPanel::IsVisible() const { return m_impl->visible; }
void AIProfilerPanel::SetVisible(bool v) { m_impl->visible = v; }
void AIProfilerPanel::ToggleVisible() { m_impl->visible = !m_impl->visible; }

void AIProfilerPanel::SetHistoryDepth(uint32_t frames) {
    m_impl->historyDepth = frames;
    while (m_impl->samples.size() > frames)
        m_impl->samples.erase(m_impl->samples.begin());
}

void AIProfilerPanel::SetShowAlerts(bool show) { m_impl->showAlerts = show; }

std::vector<AIProfileSample> AIProfilerPanel::Samples() const {
    return m_impl->samples;
}

float AIProfilerPanel::PeakAICpuMs() const  { return m_impl->peakAI; }
float AIProfilerPanel::AvgAICpuMs() const {
    if (m_impl->samples.empty()) return 0.f;
    float sum = 0.f;
    for (auto& s : m_impl->samples) sum += s.aiCpuMs;
    return sum / static_cast<float>(m_impl->samples.size());
}

float AIProfilerPanel::PeakPCGCpuMs() const { return m_impl->peakPCG; }
float AIProfilerPanel::AvgPCGCpuMs() const {
    if (m_impl->samples.empty()) return 0.f;
    float sum = 0.f;
    for (auto& s : m_impl->samples) sum += s.pcgCpuMs;
    return sum / static_cast<float>(m_impl->samples.size());
}

void AIProfilerPanel::Clear() {
    m_impl->samples.clear();
    m_impl->peakAI  = 0.f;
    m_impl->peakPCG = 0.f;
}

} // namespace Editor
