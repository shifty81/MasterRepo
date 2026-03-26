#include "Runtime/Analytics/RuntimeAnalytics.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace Runtime {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

static std::string NewSessionId() {
    static uint32_t counter = 0;
    return "session_" + std::to_string(++counter) +
           "_" + std::to_string(NowMs());
}

struct RuntimeAnalytics::Impl {
    std::string                              flushPath;
    AnalyticsSession                         current;
    bool                                     sessionActive{false};
    std::vector<AnalyticsEvent>              buffer;
    std::vector<AnalyticsSession>            sessions;
    std::unordered_map<std::string,int32_t>  counters;
    std::unordered_map<std::string,float>    gauges;
    uint32_t                                 autoFlushThreshold{100};
    std::function<void(const std::vector<AnalyticsEvent>&)> onAutoFlush;
};

RuntimeAnalytics::RuntimeAnalytics() : m_impl(new Impl()) {}
RuntimeAnalytics::~RuntimeAnalytics() { delete m_impl; }

void RuntimeAnalytics::Init(const std::string& flushPath) {
    m_impl->flushPath = flushPath;
}

void RuntimeAnalytics::Shutdown() {
    if (m_impl->sessionActive) EndSession();
    Flush();
}

void RuntimeAnalytics::StartSession(const std::string& playerId) {
    m_impl->current         = {};
    m_impl->current.sessionId = NewSessionId();
    m_impl->current.playerId  = playerId;
    m_impl->current.startMs   = NowMs();
    m_impl->sessionActive     = true;
    Track("session_start", {{"player", playerId}});
}

void RuntimeAnalytics::EndSession() {
    if (!m_impl->sessionActive) return;
    m_impl->current.endMs      = NowMs();
    m_impl->current.eventCount = static_cast<uint32_t>(m_impl->buffer.size());
    m_impl->sessions.push_back(m_impl->current);
    m_impl->sessionActive = false;
    Track("session_end");
}

bool RuntimeAnalytics::IsSessionActive() const { return m_impl->sessionActive; }

AnalyticsSession RuntimeAnalytics::CurrentSession() const { return m_impl->current; }

void RuntimeAnalytics::Track(const std::string& name,
    const std::unordered_map<std::string,std::string>& props) {
    AnalyticsEvent ev;
    ev.name        = name;
    ev.properties  = props;
    ev.timestampMs = NowMs();
    ev.sessionId   = m_impl->current.sessionId;
    m_impl->buffer.push_back(ev);

    if (m_impl->buffer.size() >= m_impl->autoFlushThreshold) {
        if (m_impl->onAutoFlush) m_impl->onAutoFlush(m_impl->buffer);
        Flush();
    }
}

void RuntimeAnalytics::Increment(const std::string& counter, int32_t delta) {
    m_impl->counters[counter] += delta;
    Track("counter", {{"name", counter}, {"value", std::to_string(m_impl->counters[counter])}});
}

void RuntimeAnalytics::Gauge(const std::string& metric, float value) {
    m_impl->gauges[metric] = value;
    Track("gauge", {{"name", metric}, {"value", std::to_string(value)}});
}

void RuntimeAnalytics::FunnelStep(const std::string& funnelName,
                                  uint32_t step, const std::string& label) {
    Track("funnel", {{"funnel", funnelName},
                     {"step", std::to_string(step)},
                     {"label", label}});
}

void RuntimeAnalytics::Flush() {
    if (m_impl->flushPath.empty() || m_impl->buffer.empty()) return;
    std::ofstream f(m_impl->flushPath, std::ios::app);
    if (!f.is_open()) return;
    for (auto& ev : m_impl->buffer) {
        f << "{\"name\":\"" << ev.name
          << "\",\"session\":\"" << ev.sessionId
          << "\",\"ts\":" << ev.timestampMs << "}\n";
    }
    m_impl->buffer.clear();
}

void RuntimeAnalytics::ClearBuffer() { m_impl->buffer.clear(); }

std::vector<AnalyticsEvent> RuntimeAnalytics::BufferedEvents() const {
    return m_impl->buffer;
}

std::vector<AnalyticsSession> RuntimeAnalytics::SessionHistory() const {
    return m_impl->sessions;
}

uint32_t RuntimeAnalytics::BufferedEventCount() const {
    return static_cast<uint32_t>(m_impl->buffer.size());
}

void RuntimeAnalytics::OnAutoFlush(
    std::function<void(const std::vector<AnalyticsEvent>&)> cb) {
    m_impl->onAutoFlush = std::move(cb);
}

void RuntimeAnalytics::SetAutoFlushThreshold(uint32_t n) {
    m_impl->autoFlushThreshold = n;
}

} // namespace Runtime
