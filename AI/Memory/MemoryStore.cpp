#include "AI/Memory/MemoryStore.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>

namespace AI {

static uint64_t NowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

uint64_t MemoryStore::Store(const std::string& key, const std::string& value, float importance) {
    auto it = m_keyIndex.find(key);
    if (it != m_keyIndex.end()) {
        for (auto& e : m_entries) {
            if (e.id == it->second) {
                e.value = value;
                e.importance = importance;
                e.timestamp = NowMs();
                return e.id;
            }
        }
    }
    MemoryEntry entry;
    entry.id = m_nextId++;
    entry.key = key;
    entry.value = value;
    entry.importance = importance;
    entry.timestamp = NowMs();
    m_entries.push_back(entry);
    m_keyIndex[key] = entry.id;
    return entry.id;
}

MemoryEntry* MemoryStore::Recall(const std::string& key) {
    auto it = m_keyIndex.find(key);
    if (it == m_keyIndex.end()) return nullptr;
    for (auto& e : m_entries) {
        if (e.id == it->second) return &e;
    }
    return nullptr;
}

std::vector<MemoryEntry> MemoryStore::Search(const std::string& query) const {
    std::vector<MemoryEntry> results;
    for (const auto& e : m_entries) {
        if (e.key.find(query) != std::string::npos ||
            e.value.find(query) != std::string::npos) {
            results.push_back(e);
        }
    }
    return results;
}

void MemoryStore::Forget(uint64_t id) {
    auto it = std::remove_if(m_entries.begin(), m_entries.end(),
        [id](const MemoryEntry& e){ return e.id == id; });
    if (it != m_entries.end()) {
        m_keyIndex.erase(it->key);
        m_entries.erase(it, m_entries.end());
    }
}

void MemoryStore::Prune(size_t maxSize) {
    if (m_entries.size() <= maxSize) return;
    std::sort(m_entries.begin(), m_entries.end(),
        [](const MemoryEntry& a, const MemoryEntry& b){ return a.importance > b.importance; });
    while (m_entries.size() > maxSize) {
        m_keyIndex.erase(m_entries.back().key);
        m_entries.pop_back();
    }
}

std::vector<MemoryEntry> MemoryStore::GetAll() const { return m_entries; }
size_t MemoryStore::Count() const { return m_entries.size(); }
void MemoryStore::Clear() { m_entries.clear(); m_keyIndex.clear(); }

static std::string JsonEscapeStr(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        if (c == '"')       out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
            out += buf;
        } else {
            out += static_cast<char>(c);
        }
    }
    return out;
}

bool MemoryStore::SaveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f << "[\n";
    for (size_t i = 0; i < m_entries.size(); ++i) {
        const auto& e = m_entries[i];
        f << "{\"id\":" << e.id
          << ",\"key\":\"" << JsonEscapeStr(e.key) << "\""
          << ",\"value\":\"" << JsonEscapeStr(e.value) << "\""
          << ",\"importance\":" << e.importance
          << ",\"timestamp\":" << e.timestamp << "}";
        if (i + 1 < m_entries.size()) f << ",";
        f << "\n";
    }
    f << "]\n";
    return true;
}

bool MemoryStore::LoadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    // TODO: implement full JSON parsing for MemoryEntry deserialization.
    // Currently a stub — returns false to indicate load is not yet supported.
    return false;
}

} // namespace AI
