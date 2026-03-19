#include "systems/fleet_norm_system.h"
#include "components/game_components.h"
#include <algorithm>

namespace atlas {

using namespace components;

void FleetNormSystem::initialize(ecs::Entity* fleet) {
    if (!fleet->hasComponent<FleetNorm>()) {
        fleet->addComponent(std::make_unique<FleetNorm>());
    }
}

void FleetNormSystem::recordNormViolation(ecs::Entity* fleet) {
    auto* norm = fleet->getComponent<FleetNorm>();
    if (norm) norm->violations++;
}

void FleetNormSystem::recordNormReinforcement(ecs::Entity* fleet) {
    auto* norm = fleet->getComponent<FleetNorm>();
    if (norm) norm->reinforcements++;
}

float FleetNormSystem::getNormStability(ecs::Entity* fleet) {
    auto* norm = fleet->getComponent<FleetNorm>();
    if (!norm) return 0.0f;

    int total = norm->violations + norm->reinforcements;
    if (total == 0) return 1.0f;
    return static_cast<float>(norm->reinforcements) / static_cast<float>(total);
}

void FleetNormSystem::updateRiskAppetite(ecs::Entity* fleet, float recent_outcomes) {
    auto* norm = fleet->getComponent<FleetNorm>();
    if (!norm) return;

    norm->risk_appetite += (recent_outcomes - norm->risk_appetite) * 0.1f;
    norm->risk_appetite = std::clamp(norm->risk_appetite, 0.0f, 1.0f);
}

} // namespace atlas
