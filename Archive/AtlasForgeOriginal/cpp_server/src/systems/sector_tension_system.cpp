#include "systems/sector_tension_system.h"
#include "components/game_components.h"
#include <algorithm>

namespace atlas {

using namespace components;

void SectorTensionSystem::initialize(ecs::Entity* system) {
    if (!system->hasComponent<SectorTension>()) {
        system->addComponent(std::make_unique<SectorTension>());
    }
}

void SectorTensionSystem::update(ecs::Entity* system, float dt, float pirate_activity, float trade_activity) {
    auto* tension = system->getComponent<SectorTension>();
    if (!tension) return;

    // Pirate pressure
    if (pirate_activity > 0.5f) {
        tension->pirate_pressure += (pirate_activity - 0.5f) * dt * 0.1f;
    } else {
        tension->pirate_pressure -= 0.05f * dt;
    }
    tension->pirate_pressure = std::clamp(tension->pirate_pressure, 0.0f, 1.0f);

    // Resource stress
    if (tension->industrial_output < 0.5f) {
        tension->resource_stress += (0.5f - tension->industrial_output) * dt * 0.1f;
    } else {
        tension->resource_stress -= 0.02f * dt;
    }
    tension->resource_stress = std::clamp(tension->resource_stress, 0.0f, 1.0f);

    // Security confidence
    tension->security_confidence = 1.0f - tension->pirate_pressure * 0.7f;
    tension->security_confidence = std::clamp(tension->security_confidence, 0.0f, 1.0f);

    // Population stability
    if (tension->resource_stress > 0.6f) {
        tension->population_stability -= (tension->resource_stress - 0.6f) * dt * 0.05f;
    }
    tension->population_stability = std::clamp(tension->population_stability, 0.0f, 1.0f);
}

float SectorTensionSystem::getThreatLevel(ecs::Entity* system) {
    auto* tension = system->getComponent<SectorTension>();
    if (!tension) return 0.0f;
    return (tension->pirate_pressure + tension->resource_stress) / 2.0f;
}

bool SectorTensionSystem::isUnderStress(ecs::Entity* system) {
    return getThreatLevel(system) > 0.5f;
}

} // namespace atlas
