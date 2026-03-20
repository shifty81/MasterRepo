#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace AI {

struct MemoryEntry {
    uint64_t                 id         = 0;
    std::string              key;
    std::string              value;
    uint64_t                 timestamp  = 0;
    float                    importance = 0.5f;
    std::vector<std::string> tags;
};

class MemoryStore {
public:
    uint64_t      Store(const std::string& key, const std::string& value, float importance = 0.5f);
    MemoryEntry*  Recall(const std::string& key);
    std::vector<MemoryEntry> Search(const std::string& query) const;
    void          Forget(uint64_t id);
    void          Prune(size_t maxSize);
    std::vector<MemoryEntry> GetAll() const;
    size_t        Count() const;
    void          Clear();
    bool          SaveToFile(const std::string& path) const;
    bool          LoadFromFile(const std::string& path);

private:
    std::vector<MemoryEntry>                  m_entries;
    std::unordered_map<std::string, uint64_t> m_keyIndex;
    uint64_t                                  m_nextId    = 1;
};

} // namespace AI
