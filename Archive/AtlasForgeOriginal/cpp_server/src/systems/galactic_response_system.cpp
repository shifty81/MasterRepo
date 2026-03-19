#include "systems/galactic_response_system.h"
#include "components/game_components.h"
#include <algorithm>

namespace atlas {

using namespace components;

void GalacticResponseSystem::initialize(ecs::Entity* galaxy) {
    if (!galaxy->hasComponent<GalacticResponse>()) {
        galaxy->addComponent(std::make_unique<GalacticResponse>());
    }
}

void GalacticResponseSystem::update(ecs::Entity* galaxy, float titan_progress, float avg_sector_tension) {
    auto* response = galaxy->getComponent<GalacticResponse>();
    if (!response) return;

    response->perceived_threat = titan_progress * 0.6f + avg_sector_tension * 0.4f;
    response->perceived_threat = std::clamp(response->perceived_threat, 0.0f, 1.0f);

    response->border_reinforcement = response->perceived_threat > 0.25f
        ? (response->perceived_threat - 0.25f) * 1.33f : 0.0f;
    response->border_reinforcement = std::clamp(response->border_reinforcement, 0.0f, 1.0f);

    response->capital_production_priority = response->perceived_threat > 0.4f
        ? (response->perceived_threat - 0.4f) * 1.67f : 0.0f;
    response->capital_production_priority = std::clamp(response->capital_production_priority, 0.0f, 1.0f);

    response->outer_system_abandonment = response->perceived_threat > 0.6f
        ? (response->perceived_threat - 0.6f) * 2.5f : 0.0f;
    response->outer_system_abandonment = std::clamp(response->outer_system_abandonment, 0.0f, 1.0f);

    response->emergency_doctrine_level = response->perceived_threat > 0.75f
        ? (response->perceived_threat - 0.75f) * 4.0f : 0.0f;
    response->emergency_doctrine_level = std::clamp(response->emergency_doctrine_level, 0.0f, 1.0f);
}

float GalacticResponseSystem::getPerceivedThreat(ecs::Entity* galaxy) {
    auto* response = galaxy->getComponent<GalacticResponse>();
    return response ? response->perceived_threat : 0.0f;
}

bool GalacticResponseSystem::isEmergencyActive(ecs::Entity* galaxy) {
    auto* response = galaxy->getComponent<GalacticResponse>();
    return response && response->emergency_doctrine_level > 0.0f;
}

} // namespace atlas
