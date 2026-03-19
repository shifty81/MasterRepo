#include "systems/npc_intent_system.h"
#include "components/game_components.h"

namespace atlas {

using namespace components;

void NPCIntentSystem::initialize(ecs::Entity* npc) {
    if (!npc->hasComponent<NPCIntent>()) {
        auto intent = std::make_unique<NPCIntent>();
        intent->current_intent = NPCIntent::Intent::Idle;
        npc->addComponent(std::move(intent));
    }
}

void NPCIntentSystem::selectIntent(ecs::Entity* npc, float security, float resource_availability, float threat_level) {
    auto* intent = npc->getComponent<NPCIntent>();
    if (!intent) return;

    intent->previous_intent = intent->current_intent;

    bool has_weapons = npc->hasComponent<Weapon>();
    bool is_miner = npc->hasComponent<MiningLaser>();

    if (threat_level > 0.7f) {
        intent->current_intent = NPCIntent::Intent::Flee;
    } else if (threat_level > 0.5f && has_weapons) {
        intent->current_intent = NPCIntent::Intent::Defend;
    } else if (resource_availability > 0.5f && is_miner) {
        intent->current_intent = NPCIntent::Intent::Mine;
    } else if (security < 0.3f) {
        intent->current_intent = NPCIntent::Intent::Patrol;
    } else {
        intent->current_intent = NPCIntent::Intent::Idle;
    }

    if (intent->current_intent != intent->previous_intent) {
        intent->intent_timer = 0.0f;
    }
}

std::string NPCIntentSystem::getIntentName(ecs::Entity* npc) {
    auto* intent = npc->getComponent<NPCIntent>();
    if (!intent) return "None";

    switch (intent->current_intent) {
        case NPCIntent::Intent::Idle:    return "Idle";
        case NPCIntent::Intent::Mine:    return "Mine";
        case NPCIntent::Intent::Haul:    return "Haul";
        case NPCIntent::Intent::Trade:   return "Trade";
        case NPCIntent::Intent::Patrol:  return "Patrol";
        case NPCIntent::Intent::Raid:    return "Raid";
        case NPCIntent::Intent::Flee:    return "Flee";
        case NPCIntent::Intent::Defend:  return "Defend";
        case NPCIntent::Intent::Escort:  return "Escort";
        case NPCIntent::Intent::Produce: return "Produce";
        default:                         return "Unknown";
    }
}

float NPCIntentSystem::getIntentDuration(ecs::Entity* npc) {
    auto* intent = npc->getComponent<NPCIntent>();
    return intent ? intent->intent_timer : 0.0f;
}

} // namespace atlas
