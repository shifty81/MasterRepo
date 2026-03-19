#include "systems/local_reputation_system.h"
#include "components/game_components.h"
#include <algorithm>
#include <cmath>

namespace atlas {

using namespace components;

// Reputation bounds
static constexpr float REP_MIN = -10.0f;
static constexpr float REP_MAX =  10.0f;

void LocalReputationSystem::initialize(ecs::Entity* entity) {
    if (!entity->hasComponent<LocalReputation>()) {
        entity->addComponent(std::make_unique<LocalReputation>());
    }
}

void LocalReputationSystem::modifyReputation(ecs::Entity* entity,
                                              const std::string& system_name,
                                              float change) {
    auto* rep = entity->getComponent<LocalReputation>();
    if (!rep) return;

    float current = 0.0f;
    auto it = rep->system_reputations.find(system_name);
    if (it != rep->system_reputations.end()) {
        current = it->second;
    }

    float updated = std::clamp(current + change, REP_MIN, REP_MAX);
    rep->system_reputations[system_name] = updated;
}

float LocalReputationSystem::getReputation(ecs::Entity* entity,
                                            const std::string& system_name) {
    auto* rep = entity->getComponent<LocalReputation>();
    if (!rep) return 0.0f;

    auto it = rep->system_reputations.find(system_name);
    if (it != rep->system_reputations.end()) {
        return it->second;
    }
    return 0.0f;
}

void LocalReputationSystem::decayReputations(ecs::Entity* entity, float dt) {
    auto* rep = entity->getComponent<LocalReputation>();
    if (!rep) return;

    for (auto& [system_name, value] : rep->system_reputations) {
        if (value > 0.0f) {
            value = std::max(0.0f, value - rep->decay_rate * dt);
        } else if (value < 0.0f) {
            value = std::min(0.0f, value + rep->decay_rate * dt);
        }
    }
}

bool LocalReputationSystem::isWelcome(ecs::Entity* entity,
                                       const std::string& system_name) {
    return getReputation(entity, system_name) >= 0.0f;
}

} // namespace atlas
