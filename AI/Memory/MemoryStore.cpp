#include "AI/Memory/MemoryStore.h"
#include <algorithm>
#include <chrono>
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

bool MemoryStore::SaveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f << "[\n";
    for (size_t i = 0; i < m_entries.size(); ++i) {
        const auto& e = m_entries[i];
        f << "{\"id\":" << e.id << ",\"key\":\"" << e.key << "\","
          << "\"value\":\"" << e.value << "\","
          << "\"importance\":" << e.importance << ","
          << "\"timestamp\":" << e.timestamp << "}";
        if (i + 1 < m_entries.size()) f << ",";
        f << "\n";
    }
    f << "]\n";
    return true;
}

bool MemoryStore::LoadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    // Minimal stub — full JSON parsing omitted
    return false;
}

} // namespace AI
