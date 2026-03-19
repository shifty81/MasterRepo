#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "Engine/Math/Math.h"
#include "Runtime/ECS/ECS.h"

namespace Runtime::Spawn {

struct SpawnConfig {
    std::string              prefabId;
    Engine::Math::Vec3       position{0.0f, 0.0f, 0.0f};
    Engine::Math::Quaternion rotation{0.0f, 0.0f, 0.0f, 1.0f};
    Engine::Math::Vec3       scale{1.0f, 1.0f, 1.0f};
    int                      poolSize = 10;
};

class SpawnSystem {
public:
    void RegisterSpawn(const std::string& id, const SpawnConfig& cfg);

    // Creates a new entity in `world` using the named config and records it.
    // Returns 0 (invalid) if the config id is unknown.
    Runtime::ECS::EntityID Spawn(const std::string& id,
                                  Runtime::ECS::World& world);

    // Destroys the entity in `world` and removes it from the active list.
    void Despawn(Runtime::ECS::EntityID entity, Runtime::ECS::World& world);

    // Destroys all tracked entities.
    void DespawnAll(Runtime::ECS::World& world);

    size_t GetActiveCount() const;

private:
    std::unordered_map<std::string, SpawnConfig>  m_configs;
    std::vector<Runtime::ECS::EntityID>           m_active;
};

} // namespace Runtime::Spawn
