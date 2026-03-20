#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Editor {

// ──────────────────────────────────────────────────────────────
// LayerTagSystem — migrated from atlas::tools::LayerTagSystem
// ──────────────────────────────────────────────────────────────

struct LayerInfo {
    std::string name;
    bool        visible = true;
    bool        locked  = false;
    uint32_t    color   = 0;   // RGBA packed
};

class LayerTagSystem {
public:
    // Layers
    bool CreateLayer(const std::string& name);
    bool RemoveLayer(const std::string& name);
    void SetLayerVisible(const std::string& name, bool visible);
    bool IsLayerVisible(const std::string& name) const;
    void SetLayerLocked(const std::string& name, bool locked);
    bool IsLayerLocked(const std::string& name) const;
    const LayerInfo* GetLayer(const std::string& name) const;
    std::vector<std::string> AllLayers() const;

    // Entity-layer assignment
    void AssignLayer(uint32_t entityId, const std::string& layer);
    std::string EntityLayer(uint32_t entityId) const;
    std::vector<uint32_t> EntitiesInLayer(const std::string& layer) const;

    // Tags
    void AddTag(uint32_t entityId, const std::string& tag);
    void RemoveTag(uint32_t entityId, const std::string& tag);
    bool HasTag(uint32_t entityId, const std::string& tag) const;
    std::vector<std::string> TagsOf(uint32_t entityId) const;
    std::vector<uint32_t>   EntitiesWithTag(const std::string& tag) const;

    // Visibility query (layer visibility + tag filter)
    bool IsEntityVisible(uint32_t entityId) const;

    void Clear();

private:
    std::unordered_map<std::string, LayerInfo>            m_layers;
    std::unordered_map<uint32_t, std::string>             m_entityLayer;
    std::unordered_map<uint32_t, std::unordered_set<std::string>> m_entityTags;
};

} // namespace Editor
