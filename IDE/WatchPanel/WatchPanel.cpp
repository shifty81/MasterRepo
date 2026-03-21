#include "IDE/WatchPanel/WatchPanel.h"
#include <algorithm>
#include <sstream>
#include <chrono>
#include <deque>
#include <unordered_map>

namespace IDE {

struct WatchPanel::Impl {
    std::vector<WatchEntry>                           entries;
    std::vector<WatchChangeCb>                        changeCbs;
    uint32_t                                          nextId{1};
    uint32_t                                          roundCount{0};
    uint32_t                                          historyDepth{20};
    std::unordered_map<uint32_t, std::deque<std::string>> history;
};

WatchPanel::WatchPanel() : m_impl(new Impl()) {}
WatchPanel::~WatchPanel() { delete m_impl; }

uint32_t WatchPanel::AddWatch(const std::string& expression, const std::string& note) {
    WatchEntry e;
    e.id         = m_impl->nextId++;
    e.expression = expression;
    e.note       = note;
    m_impl->entries.push_back(e);
    return e.id;
}

bool WatchPanel::RemoveWatch(uint32_t id) {
    auto it = std::find_if(m_impl->entries.begin(), m_impl->entries.end(),
        [id](const WatchEntry& e){ return e.id == id; });
    if (it == m_impl->entries.end()) return false;
    m_impl->entries.erase(it);
    m_impl->history.erase(id);
    return true;
}

bool WatchPanel::RemoveWatchByExpr(const std::string& expr) {
    auto it = std::find_if(m_impl->entries.begin(), m_impl->entries.end(),
        [&expr](const WatchEntry& e){ return e.expression == expr; });
    if (it == m_impl->entries.end()) return false;
    uint32_t id = it->id;
    m_impl->entries.erase(it);
    m_impl->history.erase(id);
    return true;
}

void WatchPanel::ClearAll() {
    m_impl->entries.clear();
    m_impl->history.clear();
}

bool WatchPanel::SetEnabled(uint32_t id, bool enabled) {
    for (auto& e : m_impl->entries) if (e.id == id) { e.enabled = enabled; return true; }
    return false;
}

bool WatchPanel::SetNote(uint32_t id, const std::string& note) {
    for (auto& e : m_impl->entries) if (e.id == id) { e.note = note; return true; }
    return false;
}

const WatchEntry* WatchPanel::GetEntry(uint32_t id) const {
    for (const auto& e : m_impl->entries) if (e.id == id) return &e;
    return nullptr;
}

std::vector<WatchEntry> WatchPanel::GetAll() const { return m_impl->entries; }

std::vector<WatchEntry> WatchPanel::GetChanged() const {
    std::vector<WatchEntry> out;
    for (const auto& e : m_impl->entries) if (e.changed) out.push_back(e);
    return out;
}

size_t WatchPanel::Count()      const { return m_impl->entries.size(); }
bool   WatchPanel::HasChanges() const {
    for (const auto& e : m_impl->entries) if (e.changed) return true;
    return false;
}

void WatchPanel::Snapshot(EvalCb eval) {
    ++m_impl->roundCount;
    for (auto& e : m_impl->entries) {
        if (!e.enabled) { e.changed = false; continue; }
        e.previousValue = e.currentValue;
        e.currentValue  = eval(e.expression);
        e.changed       = (e.currentValue != e.previousValue);
        ++e.snapshotCount;
        auto& hist = m_impl->history[e.id];
        hist.push_back(e.currentValue);
        if (hist.size() > m_impl->historyDepth) hist.pop_front();
        if (e.changed) for (auto& cb : m_impl->changeCbs) cb(e);
    }
}

uint32_t WatchPanel::SnapshotRound() const { return m_impl->roundCount; }

void WatchPanel::OnChange(WatchChangeCb cb) { m_impl->changeCbs.push_back(std::move(cb)); }

std::string WatchPanel::ExportText() const {
    std::ostringstream ss;
    for (const auto& e : m_impl->entries) {
        ss << "[" << e.id << "] " << e.expression << " = " << e.currentValue;
        if (e.changed) ss << "  (changed from: " << e.previousValue << ")";
        if (!e.note.empty()) ss << "  // " << e.note;
        ss << "\n";
    }
    return ss.str();
}

std::string WatchPanel::ExportCSV() const {
    std::ostringstream ss;
    ss << "id,expression,currentValue,previousValue,changed,snapshotCount,note\n";
    for (const auto& e : m_impl->entries) {
        ss << e.id << ",\"" << e.expression << "\",\"" << e.currentValue << "\",\""
           << e.previousValue << "\"," << (e.changed?1:0) << ","
           << e.snapshotCount << ",\"" << e.note << "\"\n";
    }
    return ss.str();
}

void WatchPanel::SetHistoryDepth(uint32_t max) { m_impl->historyDepth = max; }

std::vector<std::string> WatchPanel::GetHistory(uint32_t id) const {
    auto it = m_impl->history.find(id);
    if (it == m_impl->history.end()) return {};
    return {it->second.begin(), it->second.end()};
}

} // namespace IDE
