#include "systems/operational_wear_system.h"
#include "components/game_components.h"
#include <algorithm>

namespace atlas {

using namespace components;

void OperationalWearSystem::initialize(ecs::Entity* ship) {
    if (!ship->hasComponent<OperationalWear>()) {
        ship->addComponent(std::make_unique<OperationalWear>());
    }
}

void OperationalWearSystem::update(ecs::Entity* ship, float dt) {
    auto* wear = ship->getComponent<OperationalWear>();
    if (!wear) return;

    wear->deployment_time += dt;
    wear->fuel_inefficiency = std::min(wear->deployment_time / 36000.0f, 0.3f);
    wear->repair_debt += 0.0001f * dt;
    wear->crew_stress = std::min(wear->deployment_time / 72000.0f, 1.0f);
}

void OperationalWearSystem::dock(ecs::Entity* ship) {
    auto* wear = ship->getComponent<OperationalWear>();
    if (!wear) return;

    wear->deployment_time = 0.0f;
    wear->fuel_inefficiency = 0.0f;
    wear->crew_stress = 0.0f;

    if (wear->field_repaired) {
        wear->repair_debt *= 0.2f;
    } else {
        wear->repair_debt = 0.0f;
    }
    wear->field_repaired = false;
}

void OperationalWearSystem::fieldRepair(ecs::Entity* ship) {
    auto* wear = ship->getComponent<OperationalWear>();
    if (!wear) return;

    wear->repair_debt *= 0.5f;
    wear->field_repaired = true;
}

float OperationalWearSystem::getEfficiencyPenalty(ecs::Entity* ship) {
    auto* wear = ship->getComponent<OperationalWear>();
    if (!wear) return 0.0f;

    float penalty = wear->fuel_inefficiency + wear->repair_debt * 0.5f;
    return std::min(penalty, 0.5f);
}

bool OperationalWearSystem::needsDocking(ecs::Entity* ship) {
    auto* wear = ship->getComponent<OperationalWear>();
    if (!wear) return false;

    return wear->crew_stress > 0.7f || wear->repair_debt > 0.5f;
}

} // namespace atlas
