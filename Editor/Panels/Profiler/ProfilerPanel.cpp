#include "Editor/Panels/Profiler/ProfilerPanel.h"
#include <stdexcept>

namespace Editor {

ProfilerPanel::ProfilerPanel(uint32_t maxHistory)
    : m_maxHistory(maxHistory > 0 ? maxHistory : 256)
{}

// ──────────────────────────────────────────────────────────────
// Recording
// ──────────────────────────────────────────────────────────────

void ProfilerPanel::BeginFrame(uint32_t frameIndex) {
    if (m_paused) return;
    m_currentFrame = {};
    m_currentFrame.frameIndex = frameIndex;
    m_frameOpen = true;
}

void ProfilerPanel::EndFrame(float cpuMs, float gpuMs, float memMB,
                             uint32_t entities, uint32_t draws, uint64_t tris) {
    if (m_paused || !m_frameOpen) return;
    m_frameOpen = false;

    m_currentFrame.cpuTime_ms    = cpuMs;
    m_currentFrame.gpuTime_ms    = gpuMs;
    m_currentFrame.frameTime_ms  = cpuMs + gpuMs; // conservative combined
    m_currentFrame.memoryMB      = memMB;
    m_currentFrame.entityCount   = entities;
    m_currentFrame.drawCalls     = draws;
    m_currentFrame.triangleCount = tris;

    if (static_cast<uint32_t>(m_frames.size()) >= m_maxHistory)
        m_frames.erase(m_frames.begin());
    m_frames.push_back(m_currentFrame);
}

void ProfilerPanel::PushSample(const std::string& name, uint64_t duration_us,
                               int depth, uint32_t threadId) {
    if (m_paused) return;
    ProfileSample s;
    s.name        = name;
    s.duration_us = duration_us;
    s.depth       = depth;
    s.threadId    = threadId;
    s.frameIndex  = m_currentFrame.frameIndex;
    m_samples.push_back(std::move(s));

    // Prune very old samples (keep at most maxHistory * 512 to cap memory)
    const size_t sampleCap = static_cast<size_t>(m_maxHistory) * 512u;
    if (m_samples.size() > sampleCap)
        m_samples.erase(m_samples.begin(),
                        m_samples.begin() + static_cast<ptrdiff_t>(m_samples.size() - sampleCap));
}

// ──────────────────────────────────────────────────────────────
// Queries
// ──────────────────────────────────────────────────────────────

const FrameStats* ProfilerPanel::GetCurrentFrame() const {
    return m_frames.empty() ? nullptr : &m_frames.back();
}

std::vector<ProfileSample> ProfilerPanel::GetSamplesForFrame(uint32_t frameIndex) const {
    std::vector<ProfileSample> result;
    for (const auto& s : m_samples)
        if (s.frameIndex == frameIndex) result.push_back(s);
    return result;
}

float ProfilerPanel::GetAverageFPS(int lastN) const {
    if (m_frames.empty()) return 0.f;
    int count = std::min(lastN, static_cast<int>(m_frames.size()));
    float sumMs = 0.f;
    for (int i = static_cast<int>(m_frames.size()) - count;
         i < static_cast<int>(m_frames.size()); ++i)
        sumMs += m_frames[i].frameTime_ms;
    if (sumMs <= 0.f) return 0.f;
    return (static_cast<float>(count) / sumMs) * 1000.f;
}

float ProfilerPanel::GetAverageCPUMs(int lastN) const {
    if (m_frames.empty()) return 0.f;
    int count = std::min(lastN, static_cast<int>(m_frames.size()));
    float sum = 0.f;
    for (int i = static_cast<int>(m_frames.size()) - count;
         i < static_cast<int>(m_frames.size()); ++i)
        sum += m_frames[i].cpuTime_ms;
    return sum / static_cast<float>(count);
}

float ProfilerPanel::GetAverageGPUMs(int lastN) const {
    if (m_frames.empty()) return 0.f;
    int count = std::min(lastN, static_cast<int>(m_frames.size()));
    float sum = 0.f;
    for (int i = static_cast<int>(m_frames.size()) - count;
         i < static_cast<int>(m_frames.size()); ++i)
        sum += m_frames[i].gpuTime_ms;
    return sum / static_cast<float>(count);
}

float ProfilerPanel::GetPeakMemoryMB() const {
    float peak = 0.f;
    for (const auto& f : m_frames)
        if (f.memoryMB > peak) peak = f.memoryMB;
    return peak;
}

const FrameStats* ProfilerPanel::GetSlowestFrame() const {
    if (m_frames.empty()) return nullptr;
    return &*std::max_element(m_frames.begin(), m_frames.end(),
        [](const FrameStats& a, const FrameStats& b) {
            return a.frameTime_ms < b.frameTime_ms;
        });
}

// ──────────────────────────────────────────────────────────────
// Control
// ──────────────────────────────────────────────────────────────

void ProfilerPanel::Clear() {
    m_frames.clear();
    m_samples.clear();
    m_currentFrame = {};
    m_frameOpen    = false;
}

void ProfilerPanel::SetMaxHistory(uint32_t n) {
    m_maxHistory = (n > 0) ? n : 256;
    while (static_cast<uint32_t>(m_frames.size()) > m_maxHistory)
        m_frames.erase(m_frames.begin());
}

} // namespace Editor
