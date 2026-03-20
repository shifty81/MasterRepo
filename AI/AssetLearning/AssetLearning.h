#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace AI {

enum class AssetType { Mesh, Texture, Audio, Material, Prefab, Script, Unknown };

struct AssetDescriptor {
    std::string path;
    AssetType   type = AssetType::Unknown;
    std::string name;
    std::vector<std::string> tags;
    std::unordered_map<std::string, std::string> properties;
    uint64_t lastSeen = 0;
};

struct AssetPattern {
    std::string name;
    AssetType   type      = AssetType::Unknown;
    std::vector<std::string> commonTags;
    uint32_t    frequency = 0;
};

class AssetLearning {
public:
    void ScanAssetDir(const std::string& dir);
    void IndexAsset(AssetDescriptor desc);

    std::vector<AssetDescriptor> Search(const std::string& query) const;
    std::vector<AssetDescriptor> ByType(AssetType t) const;
    std::vector<AssetDescriptor> ByTag(const std::string& tag) const;

    void ExtractPatterns();
    const std::vector<AssetPattern>& GetPatterns() const;

    std::string SuggestAssetName(AssetType type, const std::vector<std::string>& tags) const;

    bool   SaveIndex(const std::string& path) const;
    bool   LoadIndex(const std::string& path);
    size_t Count() const;
    void   Clear();

private:
    AssetType                DetectType(const std::string& path) const;
    std::vector<std::string> AutoTag(const AssetDescriptor& desc) const;

    std::vector<AssetDescriptor> m_assets;
    std::vector<AssetPattern>    m_patterns;
};

} // namespace AI
