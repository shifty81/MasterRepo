#include "Tools/Profiler/GPUProfiler/GPUProfiler.h"
#include <chrono>
#include <deque>
#include <stack>
#include <numeric>
#include <algorithm>

namespace Tools {

// ── Impl ──────────────────────────────────────────────────────────────────
struct GPUProfiler::Impl {
    // Per-frame state
    bool       inFrame{false};
    uint64_t   frameIndex{0};
    double     frameStartMs{0.0};

    // Open zones (stack of zones-in-progress)
    struct OpenZone {
        std::string label;
        double      startMs{0.0};
        uint32_t    depth{0};
    };
    std::vector<OpenZone>  openStack;
    std::vector<GPUZone>   pendingZones;

    // Completed frames
    std::deque<GPUFrame> history;

    // Accumulated stats
    uint64_t totalZones{0};

    static double NowMs() {
        using namespace std::chrono;
        return duration<double, std::milli>(high_resolution_clock::now().time_since_epoch()).count();
    }
};

// ── Lifecycle ─────────────────────────────────────────────────────────────
GPUProfiler::GPUProfiler() : m_impl(new Impl()) {}
GPUProfiler::~GPUProfiler() { delete m_impl; }

// ── Frame control ─────────────────────────────────────────────────────────
void GPUProfiler::BeginFrame() {
    m_impl->inFrame      = true;
    m_impl->frameStartMs = Impl::NowMs();
    m_impl->openStack.clear();
    m_impl->pendingZones.clear();
}

GPUFrame GPUProfiler::EndFrame() {
    // Auto-pop any unclosed zones
    double now = Impl::NowMs();
    while (!m_impl->openStack.empty()) {
        auto& oz = m_impl->openStack.back();
        GPUZone z;
        z.label      = oz.label;
        z.startMs    = oz.startMs;
        z.durationMs = now - oz.startMs;
        z.depth      = oz.depth;
        m_impl->pendingZones.push_back(z);
        m_impl->openStack.pop_back();
    }

    GPUFrame frame;
    frame.frameIndex = m_impl->frameIndex++;
    frame.zones      = m_impl->pendingZones;

    // Sum top-level durations
    double total = 0;
    for (const auto& z : frame.zones)
        if (z.depth == 0) total += z.durationMs;
    frame.totalGPUTimeMs = total;

    m_impl->totalZones += frame.zones.size();

    if (m_impl->history.size() >= kMaxHistory) m_impl->history.pop_front();
    m_impl->history.push_back(frame);

    m_impl->inFrame = false;
    m_impl->pendingZones.clear();
    m_impl->openStack.clear();
    return frame;
}

// ── Zone control ──────────────────────────────────────────────────────────
void GPUProfiler::PushZone(const std::string& label) {
    Impl::OpenZone oz;
    oz.label   = label;
    oz.startMs = Impl::NowMs();
    oz.depth   = static_cast<uint32_t>(m_impl->openStack.size());
    m_impl->openStack.push_back(oz);
}

void GPUProfiler::PopZone() {
    if (m_impl->openStack.empty()) return;
    auto& oz = m_impl->openStack.back();
    GPUZone z;
    z.label      = oz.label;
    z.startMs    = oz.startMs;
    z.durationMs = Impl::NowMs() - oz.startMs;
    z.depth      = oz.depth;
    m_impl->pendingZones.push_back(z);
    m_impl->openStack.pop_back();
}

// ── History queries ───────────────────────────────────────────────────────
const GPUFrame* GPUProfiler::GetLastFrame() const {
    if (m_impl->history.empty()) return nullptr;
    return &m_impl->history.back();
}

std::vector<const GPUFrame*> GPUProfiler::GetFrameHistory(size_t n) const {
    std::vector<const GPUFrame*> out;
    size_t count = std::min(n, m_impl->history.size());
    for (size_t i = m_impl->history.size() - count; i < m_impl->history.size(); ++i)
        out.push_back(&m_impl->history[i]);
    return out;
}

double GPUProfiler::AverageFrameTimeMs() const {
    if (m_impl->history.empty()) return 0.0;
    double sum = 0;
    for (const auto& f : m_impl->history) sum += f.totalGPUTimeMs;
    return sum / static_cast<double>(m_impl->history.size());
}

double GPUProfiler::PeakFrameTimeMs() const {
    double peak = 0;
    for (const auto& f : m_impl->history)
        if (f.totalGPUTimeMs > peak) peak = f.totalGPUTimeMs;
    return peak;
}

// ── Reset ─────────────────────────────────────────────────────────────────
void GPUProfiler::Reset() {
    m_impl->history.clear();
    m_impl->totalZones  = 0;
    m_impl->frameIndex  = 0;
    m_impl->inFrame     = false;
    m_impl->openStack.clear();
    m_impl->pendingZones.clear();
}

// ── Stats ─────────────────────────────────────────────────────────────────
GPUProfilerStats GPUProfiler::Stats() const {
    GPUProfilerStats s;
    s.framesRecorded = m_impl->frameIndex;
    s.totalZones     = m_impl->totalZones;
    s.avgFrameMs     = AverageFrameTimeMs();
    s.peakFrameMs    = PeakFrameTimeMs();
    return s;
}

} // namespace Tools
