#include "Core/EventLog/EventLog.h"
#include <deque>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace Core {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

struct EventLog::Impl {
    std::deque<EventEntry>    entries;
    size_t                    capacity{10000};
    uint64_t                  nextId{1};
    std::string               flushPath;
    std::vector<EntryCallback> callbacks;
};

EventLog::EventLog() : m_impl(new Impl()) {}
EventLog::~EventLog() { delete m_impl; }

void EventLog::SetCapacity(size_t max) { m_impl->capacity = max; }
size_t EventLog::GetCapacity() const   { return m_impl->capacity; }
void EventLog::SetFlushPath(const std::string& p) { m_impl->flushPath = p; }

void EventLog::Append(const EventEntry& entry) {
    EventEntry e = entry;
    if (e.sequenceId == 0) e.sequenceId = m_impl->nextId++;
    if (e.timestampMs == 0) e.timestampMs = NowMs();
    m_impl->entries.push_back(e);
    while (m_impl->entries.size() > m_impl->capacity)
        m_impl->entries.pop_front();
    for (auto& cb : m_impl->callbacks) cb(e);
}

void EventLog::Log(const std::string& source, const std::string& type,
                   const std::string& message,
                   EventSeverity severity,
                   const std::string& extra)
{
    EventEntry e;
    e.source   = source;
    e.type     = type;
    e.message  = message;
    e.severity = severity;
    e.extra    = extra;
    Append(e);
}

void EventLog::Clear() { m_impl->entries.clear(); }

size_t EventLog::Count() const { return m_impl->entries.size(); }

std::vector<EventEntry> EventLog::GetAll() const {
    return {m_impl->entries.begin(), m_impl->entries.end()};
}

std::vector<EventEntry> EventLog::GetBySeverity(EventSeverity min) const {
    std::vector<EventEntry> out;
    for (const auto& e : m_impl->entries)
        if (static_cast<int>(e.severity) >= static_cast<int>(min)) out.push_back(e);
    return out;
}

std::vector<EventEntry> EventLog::GetBySource(const std::string& source) const {
    std::vector<EventEntry> out;
    for (const auto& e : m_impl->entries)
        if (e.source == source) out.push_back(e);
    return out;
}

std::vector<EventEntry> EventLog::GetByType(const std::string& type) const {
    std::vector<EventEntry> out;
    for (const auto& e : m_impl->entries)
        if (e.type == type) out.push_back(e);
    return out;
}

std::vector<EventEntry> EventLog::GetTail(size_t n) const {
    if (n >= m_impl->entries.size())
        return {m_impl->entries.begin(), m_impl->entries.end()};
    return {m_impl->entries.end() - static_cast<ptrdiff_t>(n), m_impl->entries.end()};
}

bool EventLog::FlushToDisk() {
    if (m_impl->flushPath.empty()) return false;
    // Build filename with timestamp.
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream fn;
    fn << m_impl->flushPath << "_"
       << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S")
       << ".jsonl";
    std::ofstream f(fn.str());
    if (!f.is_open()) return false;

    // Minimal JSON string escaper for the text fields we write.
    auto jsonStr = [](const std::string& s) -> std::string {
        std::string out;
        out.reserve(s.size() + 2);
        out += '"';
        for (unsigned char c : s) {
            if      (c == '"')  out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else if (c < 0x20) { out += "\\u00"; out += "0123456789abcdef"[c >> 4];
                                  out += "0123456789abcdef"[c & 0xF]; }
            else                 out += static_cast<char>(c);
        }
        out += '"';
        return out;
    };

    for (const auto& e : m_impl->entries) {
        f << "{\"seq\":"  << e.sequenceId
          << ",\"ts\":"   << e.timestampMs
          << ",\"src\":"  << jsonStr(e.source)
          << ",\"type\":" << jsonStr(e.type)
          << ",\"sev\":"  << static_cast<int>(e.severity)
          << ",\"msg\":"  << jsonStr(e.message)
          << ",\"extra\":" << (e.extra.empty() ? "null" : e.extra)
          << "}\n";
    }
    Clear();
    return true;
}

void EventLog::OnEntry(EntryCallback cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

} // namespace Core
