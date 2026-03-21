#include "AI/ResourceMonitor/ResourceMonitor.h"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

// Platform-specific RAM sampling.
#ifdef _WIN32
#  include <windows.h>
#  include <psapi.h>
#else
#  include <sys/resource.h>
#  include <unistd.h>
#endif

namespace AI {

// ── helpers ──────────────────────────────────────────────────────────────────

static uint64_t SampleRamUsed() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return static_cast<uint64_t>(pmc.WorkingSetSize);
    return 0;
#else
    // Read /proc/self/status for VmRSS on Linux.
    std::ifstream f("/proc/self/status");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            uint64_t kb = 0;
            if (std::sscanf(line.c_str(), "VmRSS: %llu kB",
                            (unsigned long long*)&kb) == 1)
                return kb * 1024;
        }
    }
    return 0;
#endif
}

static uint64_t SampleRamTotal() {
#ifdef _WIN32
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms))
        return static_cast<uint64_t>(ms.ullTotalPhys);
    return 0;
#else
    long pages     = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return (pages > 0 && page_size > 0)
           ? static_cast<uint64_t>(pages) * static_cast<uint64_t>(page_size)
           : 0;
#endif
}

// ── Impl ─────────────────────────────────────────────────────────────────────
struct ResourceMonitor::Impl {
    bool                  running{false};
    uint32_t              pollMs{1000};
    ResourceThresholds    thresholds;
    ResourceSnapshot      last;
    std::chrono::steady_clock::time_point sessionStart;
    std::vector<SnapshotCallback> onSample;
    std::vector<AlertCallback>    onAlert;
};

ResourceMonitor::ResourceMonitor() : m_impl(new Impl()) {}
ResourceMonitor::~ResourceMonitor() { Stop(); delete m_impl; }

void ResourceMonitor::Start() {
    if (m_impl->running) return;
    m_impl->running      = true;
    m_impl->sessionStart = std::chrono::steady_clock::now();
}

void ResourceMonitor::Stop() {
    m_impl->running = false;
}

bool ResourceMonitor::IsRunning() const { return m_impl->running; }

void ResourceMonitor::SetThresholds(const ResourceThresholds& t) {
    m_impl->thresholds = t;
}
ResourceThresholds ResourceMonitor::GetThresholds() const {
    return m_impl->thresholds;
}
void ResourceMonitor::SetPollIntervalMs(uint32_t ms) { m_impl->pollMs = ms; }

ResourceSnapshot ResourceMonitor::Sample() {
    ResourceSnapshot snap;

    // RAM
    snap.ramUsedBytes  = SampleRamUsed();
    snap.ramTotalBytes = SampleRamTotal();

    // CPU — stub: returns 0.0; replace with platform perf counters if desired.
    snap.cpuUsagePercent = 0.0f;

    // GPU — stub.
    snap.gpuUsagePercent  = 0.0f;
    snap.gpuVramUsedBytes = 0;

    // Session elapsed time.
    if (m_impl->running) {
        auto elapsed = std::chrono::steady_clock::now() - m_impl->sessionStart;
        snap.sessionElapsedSec = static_cast<float>(
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count())
            / 1000.0f;
    }

    m_impl->last = snap;
    for (auto& cb : m_impl->onSample) cb(snap);

    // Fire alerts for exceeded thresholds.
    const auto& t = m_impl->thresholds;
    if (snap.cpuUsagePercent > t.maxCpuPercent)
        for (auto& cb : m_impl->onAlert) cb(ResourceAlert::CPU, snap);
    if (snap.ramTotalBytes > 0) {
        float ramPct = 100.0f * static_cast<float>(snap.ramUsedBytes) /
                                static_cast<float>(snap.ramTotalBytes);
        if (ramPct > t.maxRamPercent)
            for (auto& cb : m_impl->onAlert) cb(ResourceAlert::RAM, snap);
    }
    if (snap.gpuUsagePercent > t.maxGpuPercent)
        for (auto& cb : m_impl->onAlert) cb(ResourceAlert::GPU, snap);
    if (snap.sessionElapsedSec > t.maxSessionSec)
        for (auto& cb : m_impl->onAlert) cb(ResourceAlert::SessionTime, snap);

    return snap;
}

ResourceSnapshot ResourceMonitor::GetLast() const { return m_impl->last; }

void ResourceMonitor::OnSample(SnapshotCallback cb) {
    m_impl->onSample.push_back(std::move(cb));
}
void ResourceMonitor::OnAlert(AlertCallback cb) {
    m_impl->onAlert.push_back(std::move(cb));
}

} // namespace AI
