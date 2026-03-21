#include "Tools/Profiler/PerformanceProfiler.h"
#include <chrono>
#include <deque>
#include <stack>
#include <unordered_map>
#include <numeric>
#include <algorithm>

namespace Tools {

using Clock    = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

static uint64_t NowUs(TimePoint epoch) {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - epoch).count());
}

struct ScopeFrame {
    std::string name;
    uint64_t    startUs{0};
    int         depth{0};
};

struct PerformanceProfiler::Impl {
    bool        running{false};
    uint32_t    currentFrame{0};
    TimePoint   epoch;
    uint64_t    frameStartUs{0};

    std::stack<ScopeFrame>             openScopes;
    std::vector<ProfileSample>         currentSamples;
    std::vector<ProfileSample>         lastFrameSamples;

    // Rolling frame times (microseconds).
    std::deque<uint64_t>               frameTimes;
    static constexpr size_t kMaxFrameHistory = 300;

    // Per-scope aggregated stats.
    std::unordered_map<std::string, ScopeStats> stats;

    std::vector<ProfileCallback> callbacks;
};

PerformanceProfiler::PerformanceProfiler() : m_impl(new Impl()) {}
PerformanceProfiler::~PerformanceProfiler() { delete m_impl; }

void PerformanceProfiler::Start() {
    if (m_impl->running) return;
    m_impl->epoch   = Clock::now();
    m_impl->running = true;
}

void PerformanceProfiler::Stop() {
    m_impl->running = false;
}

bool PerformanceProfiler::IsRunning() const { return m_impl->running; }

void PerformanceProfiler::BeginFrame() {
    if (!m_impl->running) return;
    m_impl->currentSamples.clear();
    m_impl->frameStartUs = NowUs(m_impl->epoch);
}

void PerformanceProfiler::EndFrame() {
    if (!m_impl->running) return;
    uint64_t frameTime = NowUs(m_impl->epoch) - m_impl->frameStartUs;
    m_impl->frameTimes.push_back(frameTime);
    while (m_impl->frameTimes.size() > Impl::kMaxFrameHistory)
        m_impl->frameTimes.pop_front();

    // Archive samples.
    m_impl->lastFrameSamples = m_impl->currentSamples;
    for (auto& s : m_impl->currentSamples) {
        auto& st = m_impl->stats[s.name];
        if (st.name.empty()) st.name = s.name;
        st.totalUs    += s.durationUs;
        st.minUs       = std::min(st.minUs, s.durationUs);
        st.maxUs       = std::max(st.maxUs, s.durationUs);
        st.callCount  += 1;
        st.frameCount += 1;
        st.avgUs       = st.totalUs / st.callCount;
    }
    ++m_impl->currentFrame;
}

uint32_t PerformanceProfiler::CurrentFrame() const { return m_impl->currentFrame; }

void PerformanceProfiler::BeginScope(const std::string& name) {
    if (!m_impl->running) return;
    ScopeFrame f;
    f.name    = name;
    f.startUs = NowUs(m_impl->epoch);
    f.depth   = static_cast<int>(m_impl->openScopes.size());
    m_impl->openScopes.push(f);
}

void PerformanceProfiler::EndScope() {
    if (!m_impl->running || m_impl->openScopes.empty()) return;
    uint64_t endUs = NowUs(m_impl->epoch);
    ScopeFrame f = m_impl->openScopes.top();
    m_impl->openScopes.pop();

    ProfileSample s;
    s.name       = f.name;
    s.startUs    = f.startUs;
    s.durationUs = endUs - f.startUs;
    s.depth      = f.depth;
    s.frameIndex = m_impl->currentFrame;
    m_impl->currentSamples.push_back(s);
    for (auto& cb : m_impl->callbacks) cb(s);
}

std::vector<ProfileSample> PerformanceProfiler::GetLastFrameSamples() const {
    return m_impl->lastFrameSamples;
}

ScopeStats PerformanceProfiler::GetStats(const std::string& name) const {
    auto it = m_impl->stats.find(name);
    return it != m_impl->stats.end() ? it->second : ScopeStats{};
}

std::vector<ScopeStats> PerformanceProfiler::GetAllStats() const {
    std::vector<ScopeStats> out;
    out.reserve(m_impl->stats.size());
    for (auto& [_, s] : m_impl->stats) out.push_back(s);
    return out;
}

double PerformanceProfiler::AvgFrameTimeMs(uint32_t lastN) const {
    if (m_impl->frameTimes.empty()) return 0.0;
    size_t n = std::min(static_cast<size_t>(lastN), m_impl->frameTimes.size());
    auto start = m_impl->frameTimes.end() - static_cast<ptrdiff_t>(n);
    uint64_t sum = std::accumulate(start, m_impl->frameTimes.end(), uint64_t{0});
    return static_cast<double>(sum) / static_cast<double>(n) / 1000.0;
}

void PerformanceProfiler::ResetStats() {
    m_impl->stats.clear();
    m_impl->frameTimes.clear();
    m_impl->currentFrame = 0;
}

void PerformanceProfiler::OnSample(ProfileCallback cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

} // namespace Tools
