#include "AI/SessionMemory/SessionMemory.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace AI {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

static float CosineSimilarity(const std::vector<float>& a,
                              const std::vector<float>& b) {
    if (a.empty() || a.size() != b.size()) return 0.0f;
    float dot = 0.f, na = 0.f, nb = 0.f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }
    float denom = std::sqrt(na) * std::sqrt(nb);
    return denom > 0.f ? dot / denom : 0.f;
}

struct SessionMemory::Impl {
    std::unordered_map<std::string, MemoryEntry>   entries;
    std::string                                    persistPath;
    std::function<void(const MemoryEntry&)>        onStore;
    std::function<void(const std::string&)>        onForget;
};

SessionMemory::SessionMemory() : m_impl(new Impl()) {}
SessionMemory::~SessionMemory() { delete m_impl; }

void SessionMemory::Init(const std::string& persistPath) {
    m_impl->persistPath = persistPath;
    Load();
}

void SessionMemory::Shutdown() { Save(); }

void SessionMemory::Store(const std::string& key, const std::string& value,
                          const std::string& sessionTag) {
    auto& e         = m_impl->entries[key];
    e.key           = key;
    e.value         = value;
    e.timestampMs   = NowMs();
    e.sessionTag    = sessionTag;
    e.accessCount++;
    if (m_impl->onStore) m_impl->onStore(e);
}

void SessionMemory::StoreEmbedding(const std::string& key,
                                   const std::vector<float>& embedding,
                                   const std::string& value,
                                   const std::string& sessionTag) {
    auto& e         = m_impl->entries[key];
    e.key           = key;
    e.embedding     = embedding;
    if (!value.empty()) e.value = value;
    e.timestampMs   = NowMs();
    e.sessionTag    = sessionTag;
    e.accessCount++;
    if (m_impl->onStore) m_impl->onStore(e);
}

std::string SessionMemory::Recall(const std::string& key) const {
    auto it = m_impl->entries.find(key);
    if (it == m_impl->entries.end()) return {};
    const_cast<MemoryEntry&>(it->second).accessCount++;
    return it->second.value;
}

bool SessionMemory::Has(const std::string& key) const {
    return m_impl->entries.count(key) > 0;
}

void SessionMemory::Forget(const std::string& key) {
    m_impl->entries.erase(key);
    if (m_impl->onForget) m_impl->onForget(key);
}

std::vector<MemorySimilarityResult>
SessionMemory::FindSimilar(const std::vector<float>& query,
                           uint32_t topK) const {
    std::vector<MemorySimilarityResult> results;
    for (auto& [k, e] : m_impl->entries) {
        if (e.embedding.empty()) continue;
        float score = CosineSimilarity(query, e.embedding);
        results.push_back({e.key, score, e.value});
    }
    std::sort(results.begin(), results.end(),
              [](const MemorySimilarityResult& a, const MemorySimilarityResult& b){
                  return a.score > b.score; });
    if (results.size() > topK) results.resize(topK);
    return results;
}

void SessionMemory::Save() const {
    if (m_impl->persistPath.empty()) return;
    std::ofstream f(m_impl->persistPath);
    if (!f.is_open()) return;
    f << "{\"entries\":[\n";
    bool first = true;
    for (auto& [k, e] : m_impl->entries) {
        if (!first) f << ",\n";
        first = false;
        f << "  {\"key\":\"" << e.key << "\","
          << "\"value\":\"" << e.value << "\","
          << "\"session\":\"" << e.sessionTag << "\","
          << "\"ts\":" << e.timestampMs << "}";
    }
    f << "\n]}\n";
}

void SessionMemory::Load() {
    // Minimal JSON loader — full implementation would use Core/Serialization
}

void SessionMemory::ClearDisk() {
    if (!m_impl->persistPath.empty())
        std::remove(m_impl->persistPath.c_str());
}

std::vector<MemoryEntry> SessionMemory::AllEntries() const {
    std::vector<MemoryEntry> out;
    out.reserve(m_impl->entries.size());
    for (auto& [k, v] : m_impl->entries) out.push_back(v);
    return out;
}

std::vector<MemoryEntry> SessionMemory::EntriesForSession(
    const std::string& tag) const {
    std::vector<MemoryEntry> out;
    for (auto& [k, v] : m_impl->entries)
        if (v.sessionTag == tag) out.push_back(v);
    return out;
}

uint32_t SessionMemory::Count() const {
    return static_cast<uint32_t>(m_impl->entries.size());
}

void SessionMemory::OnStore(std::function<void(const MemoryEntry&)> cb) {
    m_impl->onStore = std::move(cb);
}

void SessionMemory::OnForget(std::function<void(const std::string&)> cb) {
    m_impl->onForget = std::move(cb);
}

} // namespace AI
