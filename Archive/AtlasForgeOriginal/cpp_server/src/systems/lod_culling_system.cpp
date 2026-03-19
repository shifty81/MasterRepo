#include "systems/lod_culling_system.h"
#include "components/game_components.h"
#include <cmath>

namespace atlas {

using namespace components;

void LODCullingSystem::updatePriorities(ecs::World* world,
                                        float observerX, float observerY, float observerZ,
                                        float cull_distance) {
    auto entities = world->getAllEntities();
    for (auto* entity : entities) {
        auto* pos = entity->getComponent<Position>();
        auto* lod = entity->getComponent<LODPriority>();
        if (!pos || !lod) continue;

        // Never cull force_visible entities
        if (lod->force_visible) {
            lod->priority = 2.0f;
            continue;
        }

        float dx = pos->x - observerX;
        float dy = pos->y - observerY;
        float dz = pos->z - observerZ;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (dist >= cull_distance) {
            // Beyond culling distance — mark as culled
            lod->priority = 0.0f;
        } else if (lod->impostor_distance <= 0.0f) {
            // No impostor distance set — use linear falloff
            float t = dist / cull_distance;
            lod->priority = 2.0f * (1.0f - t);
        } else if (dist >= lod->impostor_distance) {
            // In impostor range — low priority
            float t = (dist - lod->impostor_distance) / (cull_distance - lod->impostor_distance);
            lod->priority = 0.5f * (1.0f - t);
        } else {
            // Close to observer — full priority (scaled 1.0–2.0)
            float t = dist / lod->impostor_distance;
            lod->priority = 2.0f - t;
        }
    }
}

std::vector<ecs::Entity*> LODCullingSystem::getCulledEntities(ecs::World* world) {
    std::vector<ecs::Entity*> result;
    auto entities = world->getAllEntities();
    for (auto* entity : entities) {
        auto* lod = entity->getComponent<LODPriority>();
        if (!lod) continue;
        if (lod->priority <= 0.0f && !lod->force_visible) {
            result.push_back(entity);
        }
    }
    return result;
}

std::vector<ecs::Entity*> LODCullingSystem::getVisibleEntities(ecs::World* world) {
    std::vector<ecs::Entity*> result;
    auto entities = world->getAllEntities();
    for (auto* entity : entities) {
        auto* lod = entity->getComponent<LODPriority>();
        if (!lod) continue;
        if (lod->priority > 0.0f || lod->force_visible) {
            result.push_back(entity);
        }
    }
    return result;
}

int LODCullingSystem::getCulledCount(ecs::World* world) {
    return static_cast<int>(getCulledEntities(world).size());
}

int LODCullingSystem::getVisibleCount(ecs::World* world) {
    return static_cast<int>(getVisibleEntities(world).size());
}

} // namespace atlas
