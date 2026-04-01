#include "Engine/ECS/World.h"
#include "Core/Logging/Log.h"
#include <algorithm>
#include <string>

namespace NF {

EntityId World::CreateEntity() {
    const EntityId id = m_NextId++;
    m_LiveEntities.push_back(id);
    Logger::Log(LogLevel::Trace, "ECS", "Created entity " + std::to_string(id));
    return id;
}

void World::DestroyEntity(EntityId id) {
    auto it = std::find(m_LiveEntities.begin(), m_LiveEntities.end(), id);
    if (it == m_LiveEntities.end()) return;
    m_LiveEntities.erase(it);

    // Remove every component associated with this entity via the registered
    // type-erased erasure functions.
    for (auto& [typeKey, eraser] : m_Erasers)
        eraser(id);

    Logger::Log(LogLevel::Trace, "ECS", "Destroyed entity " + std::to_string(id));
}

} // namespace NF
