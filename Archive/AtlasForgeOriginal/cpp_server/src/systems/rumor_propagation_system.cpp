#include "systems/rumor_propagation_system.h"
#include "components/game_components.h"
#include <algorithm>

namespace atlas {

using namespace components;

void RumorPropagationSystem::initialize(ecs::Entity* captain) {
    if (!captain->hasComponent<RumorLog>()) {
        captain->addComponent(std::make_unique<RumorLog>());
    }
}

void RumorPropagationSystem::propagateRumor(ecs::Entity* source, ecs::Entity* target, const std::string& rumor_id) {
    auto* source_log = source->getComponent<RumorLog>();
    if (!source_log) return;

    // Find the rumor in the source
    const RumorLog::Rumor* source_rumor = nullptr;
    for (const auto& r : source_log->rumors) {
        if (r.rumor_id == rumor_id) {
            source_rumor = &r;
            break;
        }
    }
    if (!source_rumor) return;

    auto* target_log = target->getComponent<RumorLog>();
    if (!target_log) {
        target->addComponent(std::make_unique<RumorLog>());
        target_log = target->getComponent<RumorLog>();
    }

    // Check if target already has this rumor
    for (auto& r : target_log->rumors) {
        if (r.rumor_id == rumor_id) {
            r.belief_strength = std::min(r.belief_strength + 0.1f, 1.0f);
            r.times_heard++;
            return;
        }
    }

    // Add weakened rumor to target
    RumorLog::Rumor new_rumor;
    new_rumor.rumor_id = rumor_id;
    new_rumor.text = source_rumor->text;
    new_rumor.belief_strength = source_rumor->belief_strength * 0.7f;
    new_rumor.personally_witnessed = false;
    new_rumor.times_heard = 1;
    target_log->rumors.push_back(new_rumor);
}

int RumorPropagationSystem::getRumorCount(ecs::Entity* captain) {
    auto* log = captain->getComponent<RumorLog>();
    return log ? static_cast<int>(log->rumors.size()) : 0;
}

float RumorPropagationSystem::getRumorBelief(ecs::Entity* captain, const std::string& rumor_id) {
    auto* log = captain->getComponent<RumorLog>();
    if (!log) return 0.0f;

    for (const auto& r : log->rumors) {
        if (r.rumor_id == rumor_id) {
            return r.belief_strength;
        }
    }
    return 0.0f;
}

} // namespace atlas
