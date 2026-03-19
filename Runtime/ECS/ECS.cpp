#include "Runtime/ECS/ECS.h"
#include <algorithm>

namespace Runtime::ECS {

EntityID World::CreateEntity() {
    EntityID id = m_nextID++;
    m_entities.push_back(id);
    return id;
} // namespace Runtime::ECS

void World::DestroyEntity(EntityID id) {
    m_entities.erase(
        std::remove(m_entities.begin(), m_entities.end(), id),
        m_entities.end()
    );
    m_components.erase(id);
} // namespace Runtime::ECS

bool World::IsAlive(EntityID id) const {
    return std::find(m_entities.begin(), m_entities.end(), id) != m_entities.end();
} // namespace Runtime::ECS

std::vector<EntityID> World::GetEntities() const {
    return m_entities;
} // namespace Runtime::ECS

size_t World::EntityCount() const {
    return m_entities.size();
} // namespace Runtime::ECS

void World::Update(float dt) {
    if (m_tickCallback) {
        m_tickCallback(dt);
    }
} // namespace Runtime::ECS

void World::SetTickCallback(std::function<void(float)> cb) {
    m_tickCallback = std::move(cb);
} // namespace Runtime::ECS

std::vector<std::type_index> World::GetComponentTypes(EntityID id) const {
    std::vector<std::type_index> types;
    auto it = m_components.find(id);
    if (it != m_components.end()) {
        for (const auto& pair : it->second) {
            types.push_back(pair.first);
        }
    }
    return types;
} // namespace Runtime::ECS

} // namespace Runtime::ECS
