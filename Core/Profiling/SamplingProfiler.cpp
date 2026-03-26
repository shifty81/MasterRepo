#include "Core/Profiling/SamplingProfiler.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Core {

static uint64_t NowNs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
}

struct SamplingProfiler::Impl {
    bool                               running{false};
    uint32_t                           sampleRateHz{1000};
    uint32_t                           maxSamples{0};
    std::vector<ProfileSample>         samples;
    uint64_t                           startNs{0};
    uint64_t                           stopNs{0};
    std::function<void(const ProfileSample&)> onSample;

    // Simulate a stack capture for demonstration
    ProfileSample FakeSample() {
        ProfileSample s;
        s.threadId    = 1;
        s.timestampNs = NowNs();
        // In real impl: use platform backtrace()/CaptureStackBackTrace()
        StackFrame f;
        f.address = 0xDEADBEEF;
        f.symbol  = "unknown";
        s.frames.push_back(f);
        return s;
    }
};

SamplingProfiler::SamplingProfiler() : m_impl(new Impl()) {}
SamplingProfiler::~SamplingProfiler() { if (m_impl->running) Stop(); delete m_impl; }

void SamplingProfiler::Init()     {}
void SamplingProfiler::Shutdown() { if (m_impl->running) Stop(); }

void SamplingProfiler::SetSampleRateHz(uint32_t hz)    { m_impl->sampleRateHz = hz; }
void SamplingProfiler::SetMaxSamples(uint32_t n)       { m_impl->maxSamples   = n;  }
void SamplingProfiler::SetThreadFilter(uint32_t /*id*/,
                                       bool /*include*/) {}

void SamplingProfiler::Start() {
    m_impl->running  = true;
    m_impl->startNs  = NowNs();
    m_impl->samples.clear();
}

void SamplingProfiler::Stop() {
    m_impl->running = false;
    m_impl->stopNs  = NowNs();
    // In real impl: stop the background sampler thread.
    // For now generate a synthetic batch of samples to populate results.
    uint64_t durationNs = m_impl->stopNs - m_impl->startNs;
    uint64_t intervalNs = m_impl->sampleRateHz > 0
                          ? 1000000000ULL / m_impl->sampleRateHz : 1000000ULL;
    uint64_t count = durationNs / intervalNs;
    if (m_impl->maxSamples > 0)
        count = std::min(count, (uint64_t)m_impl->maxSamples);
    count = std::min(count, (uint64_t)10000); // cap at 10k for demo
    for (uint64_t i = 0; i < count; ++i) {
        ProfileSample s;
        s.threadId    = 1;
        s.timestampNs = m_impl->startNs + i * intervalNs;
        StackFrame f;
        f.address = 0x1000 + (i % 8) * 0x100; // rotating 8 fake functions
        f.symbol  = "func_" + std::to_string(i % 8);
        s.frames.push_back(f);
        m_impl->samples.push_back(s);
        if (m_impl->onSample) m_impl->onSample(s);
    }
}

void SamplingProfiler::Reset() { m_impl->samples.clear(); }
bool SamplingProfiler::IsRunning() const { return m_impl->running; }

ProfilingReport SamplingProfiler::GetReport() const {
    ProfilingReport r;
    r.totalSamples  = static_cast<uint32_t>(m_impl->samples.size());
    r.sampleRateHz  = m_impl->sampleRateHz;
    r.durationMs    = (m_impl->stopNs - m_impl->startNs) / 1000000ULL;

    std::unordered_map<std::string, uint32_t> selfHits, totalHits;
    for (auto& s : m_impl->samples) {
        if (!s.frames.empty()) ++selfHits[s.frames[0].symbol];
        for (auto& f : s.frames)     ++totalHits[f.symbol];
    }
    for (auto& [sym, th] : totalHits) {
        FlatProfileEntry e;
        e.symbol     = sym;
        e.selfHits   = selfHits.count(sym) ? selfHits[sym] : 0;
        e.totalHits  = th;
        e.selfPct    = r.totalSamples ? e.selfHits  * 100.f / r.totalSamples : 0.f;
        e.totalPct   = r.totalSamples ? e.totalHits * 100.f / r.totalSamples : 0.f;
        r.flatProfile.push_back(e);
    }
    std::sort(r.flatProfile.begin(), r.flatProfile.end(),
        [](const FlatProfileEntry& a, const FlatProfileEntry& b){
            return a.totalHits > b.totalHits; });
    return r;
}

std::vector<ProfileSample> SamplingProfiler::RawSamples() const {
    return m_impl->samples;
}

uint32_t SamplingProfiler::SampleCount() const {
    return static_cast<uint32_t>(m_impl->samples.size());
}

bool SamplingProfiler::ExportFlameGraph(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << "{\"type\":\"flamegraph\",\"samples\":[\n";
    bool first = true;
    for (auto& s : m_impl->samples) {
        if (!s.frames.empty()) {
            if (!first) f << ",\n"; first = false;
            f << "  {\"ts\":" << s.timestampNs
              << ",\"sym\":\"" << s.frames[0].symbol << "\"}";
        }
    }
    f << "\n]}\n";
    return true;
}

bool SamplingProfiler::ExportFlatCSV(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << "symbol,self_hits,total_hits,self_pct,total_pct\n";
    auto report = GetReport();
    for (auto& e : report.flatProfile)
        f << e.symbol << "," << e.selfHits << "," << e.totalHits
          << "," << e.selfPct << "," << e.totalPct << "\n";
    return true;
}

void SamplingProfiler::OnSampleCaptured(std::function<void(const ProfileSample&)> cb) {
    m_impl->onSample = std::move(cb);
}

} // namespace Core
