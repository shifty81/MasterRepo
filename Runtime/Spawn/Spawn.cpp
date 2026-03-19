#include "Runtime/Spawn/Spawn.h"
#include "Runtime/Components/Components.h"
#include <algorithm>

namespace Runtime::Spawn {

void SpawnSystem::RegisterSpawn(const std::string& id, const SpawnConfig& cfg) {
    m_configs[id] = cfg;
}

Runtime::ECS::EntityID SpawnSystem::Spawn(const std::string& id,
                                           Runtime::ECS::World& world) {
    auto it = m_configs.find(id);
    if (it == m_configs.end())
        return 0;

    const SpawnConfig& cfg = it->second;

    Runtime::ECS::EntityID entity = world.CreateEntity();

    // Apply transform from the spawn config.
    Runtime::Components::Transform xform;
    xform.position = cfg.position;
    xform.rotation = cfg.rotation;
    xform.scale    = cfg.scale;
    world.AddComponent(entity, xform);

    m_active.push_back(entity);
    return entity;
}

void SpawnSystem::Despawn(Runtime::ECS::EntityID entity,
                           Runtime::ECS::World& world) {
    auto it = std::find(m_active.begin(), m_active.end(), entity);
    if (it == m_active.end())
        return;

    m_active.erase(it);
    world.DestroyEntity(entity);
}

void SpawnSystem::DespawnAll(Runtime::ECS::World& world) {
    for (auto entity : m_active)
        world.DestroyEntity(entity);
    m_active.clear();
}

size_t SpawnSystem::GetActiveCount() const {
    return m_active.size();
}

} // namespace Runtime::Spawn
