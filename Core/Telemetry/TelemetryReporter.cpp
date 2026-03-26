#include "Core/Telemetry/TelemetryReporter.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>

namespace Core {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct TelemetryReporter::Impl {
    bool                          optedIn{false};
    std::string                   storageDir;
    std::vector<TelemetryEvent>   buffer;
    std::function<void(const TelemetryEvent&)> onEvent;

    static constexpr const char* ENGINE_VERSION = "1.0.0";
    static constexpr const char* PLATFORM       = "Win64";

    TelemetryEvent MakeEvent(TelemetryEventType type,
                             const std::string& name,
                             const std::string& detail,
                             float value = 0.f) {
        TelemetryEvent ev;
        ev.type          = type;
        ev.name          = name;
        ev.detail        = detail;
        ev.numericValue  = value;
        ev.timestampMs   = NowMs();
        ev.engineVersion = ENGINE_VERSION;
        ev.platform      = PLATFORM;
        return ev;
    }

    std::string CurrentLogPath() const {
        return storageDir + "telemetry_" + std::to_string(NowMs()) + ".jsonl";
    }
};

TelemetryReporter::TelemetryReporter() : m_impl(new Impl()) {}
TelemetryReporter::~TelemetryReporter() { delete m_impl; }

void TelemetryReporter::Init(const std::string& storageDir) {
    m_impl->storageDir = storageDir;
    if (!m_impl->storageDir.empty() && m_impl->storageDir.back() != '/')
        m_impl->storageDir += '/';
}

void TelemetryReporter::Shutdown() {
    if (m_impl->optedIn) FlushLocal();
}

void TelemetryReporter::SetOptIn(bool enabled) { m_impl->optedIn = enabled; }
bool TelemetryReporter::IsOptedIn()      const { return m_impl->optedIn; }

void TelemetryReporter::RecordFeatureUsage(const std::string& featureName) {
    if (!m_impl->optedIn) return;
    auto ev = m_impl->MakeEvent(TelemetryEventType::FeatureUsage, featureName, "");
    m_impl->buffer.push_back(ev);
    if (m_impl->onEvent) m_impl->onEvent(ev);
}

void TelemetryReporter::RecordCrash(const std::string& message,
                                    const std::string& stackTrace) {
    if (!m_impl->optedIn) return;
    auto ev = m_impl->MakeEvent(TelemetryEventType::CrashReport, message, stackTrace);
    m_impl->buffer.push_back(ev);
    if (m_impl->onEvent) m_impl->onEvent(ev);
    FlushLocal(); // always flush crashes immediately
}

void TelemetryReporter::RecordPerformanceOutlier(const std::string& metric,
                                                 float actual, float threshold) {
    if (!m_impl->optedIn) return;
    std::ostringstream detail;
    detail << "actual=" << actual << " threshold=" << threshold;
    auto ev = m_impl->MakeEvent(TelemetryEventType::PerformanceOutlier,
                                metric, detail.str(), actual);
    m_impl->buffer.push_back(ev);
    if (m_impl->onEvent) m_impl->onEvent(ev);
}

void TelemetryReporter::RecordFeedback(const std::string& text) {
    if (!m_impl->optedIn) return;
    auto ev = m_impl->MakeEvent(TelemetryEventType::UserFeedback, "feedback", text);
    m_impl->buffer.push_back(ev);
    if (m_impl->onEvent) m_impl->onEvent(ev);
}

void TelemetryReporter::FlushLocal() {
    if (m_impl->buffer.empty()) return;
    std::string path = m_impl->storageDir + "telemetry.jsonl";
    std::ofstream f(path, std::ios::app);
    if (!f.is_open()) return;
    for (auto& ev : m_impl->buffer) {
        f << "{\"type\":" << static_cast<int>(ev.type)
          << ",\"name\":\"" << ev.name << "\""
          << ",\"detail\":\"" << ev.detail << "\""
          << ",\"ts\":" << ev.timestampMs
          << ",\"platform\":\"" << ev.platform << "\""
          << "}\n";
    }
    m_impl->buffer.clear();
}

void TelemetryReporter::ClearLocal() {
    std::string path = m_impl->storageDir + "telemetry.jsonl";
    std::remove(path.c_str());
    m_impl->buffer.clear();
}

std::vector<TelemetryEvent> TelemetryReporter::BufferedEvents() const {
    return m_impl->buffer;
}

uint32_t TelemetryReporter::BufferedEventCount() const {
    return static_cast<uint32_t>(m_impl->buffer.size());
}

std::vector<std::string> TelemetryReporter::LocalReportFiles() const {
    // Would enumerate storageDir for *.jsonl; stub returns known file
    return {m_impl->storageDir + "telemetry.jsonl"};
}

void TelemetryReporter::OnEvent(std::function<void(const TelemetryEvent&)> cb) {
    m_impl->onEvent = std::move(cb);
}

} // namespace Core
