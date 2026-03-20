#include "Editor/Panels/SceneOutliner/LayerTagSystem.h"
#include <algorithm>

namespace Editor {

bool LayerTagSystem::CreateLayer(const std::string& name) {
    if (m_layers.count(name)) return false;
    m_layers[name] = LayerInfo{name, true, false, 0};
    return true;
}

bool LayerTagSystem::RemoveLayer(const std::string& name) {
    auto it = m_layers.find(name);
    if (it == m_layers.end()) return false;
    m_layers.erase(it);
    // Move all entities from this layer to default
    for (auto& kv : m_entityLayer)
        if (kv.second == name) kv.second = "";
    return true;
}

void LayerTagSystem::SetLayerVisible(const std::string& name, bool visible) {
    auto it = m_layers.find(name);
    if (it != m_layers.end()) it->second.visible = visible;
}

bool LayerTagSystem::IsLayerVisible(const std::string& name) const {
    auto it = m_layers.find(name);
    return (it == m_layers.end()) ? true : it->second.visible;
}

void LayerTagSystem::SetLayerLocked(const std::string& name, bool locked) {
    auto it = m_layers.find(name);
    if (it != m_layers.end()) it->second.locked = locked;
}

bool LayerTagSystem::IsLayerLocked(const std::string& name) const {
    auto it = m_layers.find(name);
    return (it == m_layers.end()) ? false : it->second.locked;
}

const LayerInfo* LayerTagSystem::GetLayer(const std::string& name) const {
    auto it = m_layers.find(name);
    return (it == m_layers.end()) ? nullptr : &it->second;
}

std::vector<std::string> LayerTagSystem::AllLayers() const {
    std::vector<std::string> out;
    out.reserve(m_layers.size());
    for (const auto& kv : m_layers) out.push_back(kv.first);
    return out;
}

void LayerTagSystem::AssignLayer(uint32_t entityId, const std::string& layer) {
    if (!layer.empty() && !m_layers.count(layer))
        m_layers[layer] = LayerInfo{layer, true, false, 0};
    m_entityLayer[entityId] = layer;
}

std::string LayerTagSystem::EntityLayer(uint32_t entityId) const {
    auto it = m_entityLayer.find(entityId);
    return (it == m_entityLayer.end()) ? "" : it->second;
}

std::vector<uint32_t> LayerTagSystem::EntitiesInLayer(const std::string& layer) const {
    std::vector<uint32_t> out;
    for (const auto& kv : m_entityLayer)
        if (kv.second == layer) out.push_back(kv.first);
    return out;
}

void LayerTagSystem::AddTag(uint32_t entityId, const std::string& tag) {
    m_entityTags[entityId].insert(tag);
}

void LayerTagSystem::RemoveTag(uint32_t entityId, const std::string& tag) {
    auto it = m_entityTags.find(entityId);
    if (it != m_entityTags.end()) it->second.erase(tag);
}

bool LayerTagSystem::HasTag(uint32_t entityId, const std::string& tag) const {
    auto it = m_entityTags.find(entityId);
    return (it != m_entityTags.end()) && it->second.count(tag);
}

std::vector<std::string> LayerTagSystem::TagsOf(uint32_t entityId) const {
    std::vector<std::string> out;
    auto it = m_entityTags.find(entityId);
    if (it != m_entityTags.end())
        for (const auto& t : it->second) out.push_back(t);
    return out;
}

std::vector<uint32_t> LayerTagSystem::EntitiesWithTag(const std::string& tag) const {
    std::vector<uint32_t> out;
    for (const auto& kv : m_entityTags)
        if (kv.second.count(tag)) out.push_back(kv.first);
    return out;
}

bool LayerTagSystem::IsEntityVisible(uint32_t entityId) const {
    return IsLayerVisible(EntityLayer(entityId));
}

void LayerTagSystem::Clear() {
    m_layers.clear();
    m_entityLayer.clear();
    m_entityTags.clear();
}

} // namespace Editor
