#include "Editor/Panels/LiveProfilerBackend.h"
#include <algorithm>
#include <chrono>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Clock helper
// ---------------------------------------------------------------------------

double LiveProfilerBackend::NowMs() noexcept
{
    using namespace std::chrono;
    auto tp = steady_clock::now().time_since_epoch();
    return static_cast<double>(duration_cast<microseconds>(tp).count()) / 1000.0;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void LiveProfilerBackend::BeginFrame()
{
    m_FrameStartMs  = NowMs();
    m_CurrentScopes.clear();
    m_ScopeStack.clear();
    m_PendingGpuMs  = 0.0;
}

void LiveProfilerBackend::EndFrame()
{
    double frameEnd = NowMs();
    double cpuMs    = frameEnd - m_FrameStartMs;

    ProfilerSample sample;
    sample.cpuMs   = cpuMs;
    sample.gpuMs   = m_PendingGpuMs;
    sample.totalMs = cpuMs; // GPU may be async; treat CPU as wall time for now

    m_Samples.push_back(sample);
    if (m_Samples.size() > kHistorySize)
        m_Samples.pop_front();

    m_LastScopes = std::move(m_CurrentScopes);
    m_CurrentScopes.clear();
}

void LiveProfilerBackend::PushScope(const std::string& name)
{
    ScopeEntry entry;
    entry.name    = name;
    entry.startMs = NowMs();
    entry.depth   = static_cast<uint32_t>(m_ScopeStack.size());
    m_ScopeStack.push_back(entry);
}

void LiveProfilerBackend::PopScope()
{
    if (m_ScopeStack.empty()) return;
    ScopeEntry& top = m_ScopeStack.back();

    ProfilerScope scope;
    scope.name       = top.name;
    scope.durationMs = NowMs() - top.startMs;
    scope.depth      = top.depth;
    m_CurrentScopes.push_back(scope);
    m_ScopeStack.pop_back();
}

double LiveProfilerBackend::AverageCpuMs(uint32_t n) const noexcept
{
    if (m_Samples.empty()) return 0.0;
    uint32_t count = std::min(static_cast<uint32_t>(m_Samples.size()), n);
    double   sum   = 0.0;
    auto     begin = m_Samples.end() - static_cast<ptrdiff_t>(count);
    for (auto it = begin; it != m_Samples.end(); ++it)
        sum += it->cpuMs;
    return sum / static_cast<double>(count);
}

double LiveProfilerBackend::AverageGpuMs(uint32_t n) const noexcept
{
    if (m_Samples.empty()) return 0.0;
    uint32_t count = std::min(static_cast<uint32_t>(m_Samples.size()), n);
    double   sum   = 0.0;
    auto     begin = m_Samples.end() - static_cast<ptrdiff_t>(count);
    for (auto it = begin; it != m_Samples.end(); ++it)
        sum += it->gpuMs;
    return sum / static_cast<double>(count);
}

double LiveProfilerBackend::PeakCpuMs() const noexcept
{
    double peak = 0.0;
    for (const auto& s : m_Samples)
        if (s.cpuMs > peak) peak = s.cpuMs;
    return peak;
}

} // namespace NF::Editor
