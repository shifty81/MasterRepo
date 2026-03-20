#include "AI/AssetLearning/AssetLearning.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace AI {

static uint64_t AssetNowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

static std::string AssetTypeName(AssetType t) {
    switch (t) {
    case AssetType::Mesh:     return "Mesh";
    case AssetType::Texture:  return "Texture";
    case AssetType::Audio:    return "Audio";
    case AssetType::Material: return "Material";
    case AssetType::Prefab:   return "Prefab";
    case AssetType::Script:   return "Script";
    default:                  return "Unknown";
    }
}

static AssetType AssetTypeFromName(const std::string& s) {
    if (s == "Mesh")     return AssetType::Mesh;
    if (s == "Texture")  return AssetType::Texture;
    if (s == "Audio")    return AssetType::Audio;
    if (s == "Material") return AssetType::Material;
    if (s == "Prefab")   return AssetType::Prefab;
    if (s == "Script")   return AssetType::Script;
    return AssetType::Unknown;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

AssetType AssetLearning::DetectType(const std::string& path) const {
    namespace fs = std::filesystem;
    std::string ext = fs::path(path).extension().string();
    // Mesh
    if (ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".obj" ||
        ext == ".dae" || ext == ".3ds")
        return AssetType::Mesh;
    // Texture
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" ||
        ext == ".bmp" || ext == ".hdr" || ext == ".exr")
        return AssetType::Texture;
    // Audio
    if (ext == ".wav" || ext == ".ogg" || ext == ".mp3" || ext == ".flac")
        return AssetType::Audio;
    // Material
    if (ext == ".mat" || ext == ".mtl")
        return AssetType::Material;
    // Prefab
    if (ext == ".prefab" || ext == ".scene")
        return AssetType::Prefab;
    // Script
    if (ext == ".lua" || ext == ".py" || ext == ".js" || ext == ".ts")
        return AssetType::Script;
    return AssetType::Unknown;
}

std::vector<std::string> AssetLearning::AutoTag(const AssetDescriptor& desc) const {
    std::vector<std::string> tags;
    tags.push_back(AssetTypeName(desc.type));

    namespace fs = std::filesystem;
    std::string stem = fs::path(desc.path).stem().string();
    // Common naming conventions → tags
    static const std::vector<std::pair<std::string, std::string>> kHints = {
        {"hull",    "hull"},   {"thruster", "thruster"},
        {"cockpit", "cockpit"},{"weapon",   "weapon"},
        {"ship",    "ship"},   {"station",  "station"},
        {"player",  "player"}, {"enemy",    "enemy"},
        {"ui",      "ui"},     {"hud",      "hud"},
        {"terrain", "terrain"},{"tree",     "vegetation"},
        {"rock",    "rock"},   {"vfx",      "vfx"},
    };
    std::string lower = stem;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (const auto& [kw, tag] : kHints) {
        if (lower.find(kw) != std::string::npos)
            tags.push_back(tag);
    }
    return tags;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void AssetLearning::ScanAssetDir(const std::string& dir) {
    namespace fs = std::filesystem;
    if (!fs::exists(dir)) return;

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;

        AssetDescriptor desc;
        desc.path     = fs::relative(entry.path(), dir).string();
        desc.name     = entry.path().stem().string();
        desc.type     = DetectType(entry.path().string());
        desc.lastSeen = AssetNowMs();
        desc.tags     = AutoTag(desc);
        IndexAsset(std::move(desc));
    }
}

void AssetLearning::IndexAsset(AssetDescriptor desc) {
    if (desc.lastSeen == 0) desc.lastSeen = AssetNowMs();
    if (desc.type == AssetType::Unknown) desc.type = DetectType(desc.path);
    if (desc.tags.empty())               desc.tags = AutoTag(desc);
    m_assets.push_back(std::move(desc));
}

std::vector<AssetDescriptor> AssetLearning::Search(const std::string& query) const {
    std::vector<AssetDescriptor> results;
    for (const auto& a : m_assets) {
        if (a.path.find(query) != std::string::npos ||
            a.name.find(query) != std::string::npos) {
            results.push_back(a);
        }
    }
    return results;
}

std::vector<AssetDescriptor> AssetLearning::ByType(AssetType t) const {
    std::vector<AssetDescriptor> results;
    for (const auto& a : m_assets) {
        if (a.type == t) results.push_back(a);
    }
    return results;
}

std::vector<AssetDescriptor> AssetLearning::ByTag(const std::string& tag) const {
    std::vector<AssetDescriptor> results;
    for (const auto& a : m_assets) {
        for (const auto& t : a.tags) {
            if (t == tag) { results.push_back(a); break; }
        }
    }
    return results;
}

void AssetLearning::ExtractPatterns() {
    // Aggregate by type+tag combination and count frequency
    std::unordered_map<std::string, AssetPattern> byName;

    for (const auto& a : m_assets) {
        std::string key = AssetTypeName(a.type);
        if (!a.tags.empty()) key += "_" + a.tags.front();

        auto& p = byName[key];
        if (p.name.empty()) {
            p.name = key;
            p.type = a.type;
        }
        p.frequency++;
        for (const auto& tag : a.tags) {
            if (std::find(p.commonTags.begin(), p.commonTags.end(), tag) == p.commonTags.end())
                p.commonTags.push_back(tag);
        }
    }

    m_patterns.clear();
    for (auto& [key, pat] : byName)
        m_patterns.push_back(pat);

    std::sort(m_patterns.begin(), m_patterns.end(),
              [](const AssetPattern& a, const AssetPattern& b){
                  return a.frequency > b.frequency;
              });
}

const std::vector<AssetPattern>& AssetLearning::GetPatterns() const {
    return m_patterns;
}

std::string AssetLearning::SuggestAssetName(AssetType type,
                                             const std::vector<std::string>& tags) const {
    std::string base = AssetTypeName(type);
    std::transform(base.begin(), base.end(), base.begin(), ::tolower);
    for (const auto& tag : tags) {
        if (tag != base) base += "_" + tag;
    }
    return base;
}

bool AssetLearning::SaveIndex(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    for (const auto& a : m_assets) {
        f << "ASSET\t" << a.path << "\t" << AssetTypeName(a.type) << "\t"
          << a.name << "\t" << a.lastSeen << "\n";
        for (const auto& tag : a.tags)
            f << "TAG\t" << tag << "\n";
        for (const auto& [k, v] : a.properties)
            f << "PROP\t" << k << "\t" << v << "\n";
    }
    return true;
}

bool AssetLearning::LoadIndex(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    m_assets.clear();
    std::string line;
    AssetDescriptor* cur = nullptr;
    while (std::getline(f, line)) {
        if (line.substr(0, 6) == "ASSET\t") {
            m_assets.push_back({});
            cur = &m_assets.back();
            std::istringstream ss(line.substr(6));
            std::string typeName, ts;
            std::getline(ss, cur->path,  '\t');
            std::getline(ss, typeName,   '\t');
            std::getline(ss, cur->name,  '\t');
            std::getline(ss, ts,         '\t');
            cur->type = AssetTypeFromName(typeName);
            try { cur->lastSeen = std::stoull(ts); } catch (...) {}
        } else if (line.substr(0, 4) == "TAG\t" && cur) {
            cur->tags.push_back(line.substr(4));
        } else if (line.substr(0, 5) == "PROP\t" && cur) {
            std::string rest = line.substr(5);
            auto tab = rest.find('\t');
            if (tab != std::string::npos)
                cur->properties[rest.substr(0, tab)] = rest.substr(tab + 1);
        }
    }
    return true;
}

size_t AssetLearning::Count() const { return m_assets.size(); }
void   AssetLearning::Clear()       { m_assets.clear(); m_patterns.clear(); }

} // namespace AI
