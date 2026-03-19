#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace atlas::tools {

/// Describes a layer that can contain entities.
struct LayerInfo {
    std::string name;
    bool visible = true;
    bool locked = false;   ///< Locked layers can't be edited.
    uint32_t color = 0;    ///< Optional display color (RGBA packed).
};

/// Entity categorization and visibility control via layers and tags.
///
/// Layers are named groups.  Each entity can belong to exactly one layer
/// and have zero or more string tags.  Visibility can be toggled per-layer
/// or by filtering on tags.
class LayerTagSystem {
public:
    // ----- Layers -----

    /// Create a new layer.  Returns false if a layer with that name exists.
    bool CreateLayer(const std::string& name);

    /// Remove a layer.  All entities in the layer are moved to "" (default).
    bool RemoveLayer(const std::string& name);

    /// Set layer visibility.
    void SetLayerVisible(const std::string& name, bool visible);

    /// Query visibility.  Returns true for non-existent layers (unknown
    /// layers are treated as visible by default).
    bool IsLayerVisible(const std::string& name) const;

    /// Set layer locked state.
    void SetLayerLocked(const std::string& name, bool locked);

    /// Query locked state.
    bool IsLayerLocked(const std::string& name) const;

    /// Get info for a layer (nullptr if not found).
    const LayerInfo* GetLayer(const std::string& name) const;

    /// All layer names.
    std::vector<std::string> AllLayers() const;

    // ----- Entity-to-layer assignment -----

    /// Assign an entity to a layer (creates layer if needed).
    void AssignLayer(uint32_t entityId, const std::string& layer);

    /// Get the layer an entity belongs to ("" if unassigned).
    std::string EntityLayer(uint32_t entityId) const;

    /// Get all entities in a given layer.
    std::vector<uint32_t> EntitiesInLayer(const std::string& layer) const;

    // ----- Tags -----

    /// Add a tag to an entity.
    void AddTag(uint32_t entityId, const std::string& tag);

    /// Remove a tag from an entity.
    void RemoveTag(uint32_t entityId, const std::string& tag);

    /// Check if an entity has a tag.
    bool HasTag(uint32_t entityId, const std::string& tag) const;

    /// All tags on an entity.
    std::vector<std::string> EntityTags(uint32_t entityId) const;

    /// All entities with a given tag.
    std::vector<uint32_t> EntitiesWithTag(const std::string& tag) const;

    // ----- Visibility queries -----

    /// Is the entity visible (based on its layer's visibility)?
    bool IsEntityVisible(uint32_t entityId) const;

    // ----- Housekeeping -----

    /// Remove all entity assignments for a given entity id.
    void RemoveEntity(uint32_t entityId);

    /// Clear everything.
    void Clear();

private:
    std::unordered_map<std::string, LayerInfo> m_layers;
    std::unordered_map<uint32_t, std::string> m_entityLayer;
    std::unordered_map<uint32_t, std::unordered_set<std::string>> m_entityTags;
};

} // namespace atlas::tools
