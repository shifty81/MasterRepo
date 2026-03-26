#include "Core/AssetMetadata/AssetMetadata.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace Core {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct AssetMetadata::Impl {
    std::unordered_map<std::string, AssetRecord> records;
    std::string                                  persistPath;
};

AssetMetadata::AssetMetadata() : m_impl(new Impl()) {}
AssetMetadata::~AssetMetadata() { delete m_impl; }

void AssetMetadata::Init(const std::string& persistPath) {
    m_impl->persistPath = persistPath;
    Load();
}

void AssetMetadata::Shutdown() { Save(); }

void AssetMetadata::Register(const std::string& path,
                             const std::string& assetType) {
    if (m_impl->records.count(path)) return;
    AssetRecord& r = m_impl->records[path];
    r.path       = path;
    r.assetType  = assetType;
    r.createdMs  = NowMs();
    r.modifiedMs = r.createdMs;
    // seed version 1
    r.history.push_back({1u, "Human", "Initial registration", "", r.createdMs});
}

void AssetMetadata::Unregister(const std::string& path) {
    m_impl->records.erase(path);
}

bool AssetMetadata::IsRegistered(const std::string& path) const {
    return m_impl->records.count(path) > 0;
}

void AssetMetadata::Annotate(const std::string& path,
                             const std::string& aiNote) {
    auto it = m_impl->records.find(path);
    if (it != m_impl->records.end()) {
        it->second.aiAnnotation = aiNote;
        it->second.modifiedMs   = NowMs();
    }
}

void AssetMetadata::AddTag(const std::string& path, const std::string& tag) {
    auto it = m_impl->records.find(path);
    if (it == m_impl->records.end()) return;
    auto& tags = it->second.tags;
    if (std::find(tags.begin(), tags.end(), tag) == tags.end())
        tags.push_back(tag);
}

void AssetMetadata::RemoveTag(const std::string& path, const std::string& tag) {
    auto it = m_impl->records.find(path);
    if (it == m_impl->records.end()) return;
    auto& tags = it->second.tags;
    tags.erase(std::remove(tags.begin(), tags.end(), tag), tags.end());
}

void AssetMetadata::Commit(const std::string& path,
                           const std::string& changeSummary,
                           const std::string& author,
                           const std::string& aiReasoning) {
    auto it = m_impl->records.find(path);
    if (it == m_impl->records.end()) return;
    AssetRecord& r = it->second;
    uint32_t nextVer = r.history.empty() ? 1u
                       : r.history.back().version + 1u;
    r.history.push_back({nextVer, author, changeSummary, aiReasoning, NowMs()});
    r.modifiedMs = NowMs();
}

AssetRecord AssetMetadata::Get(const std::string& path) const {
    auto it = m_impl->records.find(path);
    if (it == m_impl->records.end()) return {};
    return it->second;
}

std::vector<AssetRecord> AssetMetadata::All() const {
    std::vector<AssetRecord> out;
    out.reserve(m_impl->records.size());
    for (auto& [k, v] : m_impl->records) out.push_back(v);
    return out;
}

std::vector<AssetRecord> AssetMetadata::ByTag(const std::string& tag) const {
    std::vector<AssetRecord> out;
    for (auto& [k, r] : m_impl->records)
        if (std::find(r.tags.begin(), r.tags.end(), tag) != r.tags.end())
            out.push_back(r);
    return out;
}

std::vector<AssetRecord> AssetMetadata::ByType(const std::string& type) const {
    std::vector<AssetRecord> out;
    for (auto& [k, r] : m_impl->records)
        if (r.assetType == type) out.push_back(r);
    return out;
}

std::vector<AssetVersion> AssetMetadata::History(const std::string& path) const {
    auto it = m_impl->records.find(path);
    if (it == m_impl->records.end()) return {};
    return it->second.history;
}

void AssetMetadata::Save() const {
    if (m_impl->persistPath.empty()) return;
    std::ofstream f(m_impl->persistPath);
    if (!f.is_open()) return;
    f << "{\"assets\":[\n";
    bool first = true;
    for (auto& [path, r] : m_impl->records) {
        if (!first) f << ",\n";
        first = false;
        f << "  {\"path\":\"" << r.path << "\","
          << "\"type\":\"" << r.assetType << "\","
          << "\"annotation\":\"" << r.aiAnnotation << "\","
          << "\"versions\":" << r.history.size() << "}";
    }
    f << "\n]}\n";
}

void AssetMetadata::Load() {
    // Minimal stub; full implementation delegates to Core/Serialization
}

} // namespace Core
