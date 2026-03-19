#include "systems/npc_archetype_system.h"
#include "components/game_components.h"
#include <algorithm>

namespace atlas {

using namespace components;

void NPCArchetypeSystem::initialize(ecs::Entity* npc,
                                     const std::string& archetype) {
    if (!npc->hasComponent<NPCArchetype>()) {
        auto comp = std::make_unique<NPCArchetype>();

        if (archetype == "Trader") {
            comp->archetype       = NPCArchetype::Archetype::Trader;
            comp->risk_tolerance  = 0.3f;
            comp->default_intent  = NPCIntent::Intent::Trade;
        } else if (archetype == "Pirate") {
            comp->archetype       = NPCArchetype::Archetype::Pirate;
            comp->risk_tolerance  = 0.8f;
            comp->default_intent  = NPCIntent::Intent::Raid;
        } else if (archetype == "Patrol") {
            comp->archetype       = NPCArchetype::Archetype::Patrol;
            comp->risk_tolerance  = 0.6f;
            comp->default_intent  = NPCIntent::Intent::Patrol;
        } else if (archetype == "Miner") {
            comp->archetype       = NPCArchetype::Archetype::Miner;
            comp->risk_tolerance  = 0.2f;
            comp->default_intent  = NPCIntent::Intent::Mine;
        } else if (archetype == "Hauler") {
            comp->archetype       = NPCArchetype::Archetype::Hauler;
            comp->risk_tolerance  = 0.25f;
            comp->default_intent  = NPCIntent::Intent::Haul;
        } else if (archetype == "Industrialist") {
            comp->archetype       = NPCArchetype::Archetype::Industrialist;
            comp->risk_tolerance  = 0.15f;
            comp->default_intent  = NPCIntent::Intent::Produce;
        } else {
            // Unknown archetype — default to Trader
            comp->archetype       = NPCArchetype::Archetype::Trader;
            comp->risk_tolerance  = 0.3f;
            comp->default_intent  = NPCIntent::Intent::Trade;
        }

        npc->addComponent(std::move(comp));
    }

    // Ensure the NPC also has an NPCIntent component
    if (!npc->hasComponent<NPCIntent>()) {
        auto intent = std::make_unique<NPCIntent>();
        auto* arch  = npc->getComponent<NPCArchetype>();
        if (arch) {
            intent->current_intent = arch->default_intent;
        }
        npc->addComponent(std::move(intent));
    }
}

void NPCArchetypeSystem::selectArchetypeIntent(ecs::Entity* npc,
                                                float security,
                                                float resource_availability,
                                                float threat_level) {
    auto* arch   = npc->getComponent<NPCArchetype>();
    auto* intent = npc->getComponent<NPCIntent>();
    if (!arch || !intent) return;

    intent->previous_intent = intent->current_intent;

    // Universal override: extreme threat causes everyone to flee
    if (threat_level > 0.9f) {
        intent->current_intent = NPCIntent::Intent::Flee;
    }
    // Threat exceeds archetype risk tolerance — flee
    else if (threat_level > arch->risk_tolerance) {
        intent->current_intent = NPCIntent::Intent::Flee;
    }
    // Archetype-specific behavior selection
    else {
        switch (arch->archetype) {
        case NPCArchetype::Archetype::Trader:
            if (security < 0.3f) {
                intent->current_intent = NPCIntent::Intent::Flee;
            } else {
                intent->current_intent = NPCIntent::Intent::Trade;
            }
            break;

        case NPCArchetype::Archetype::Pirate:
            if (security > 0.7f) {
                intent->current_intent = NPCIntent::Intent::Idle;  // lay low in high-sec
            } else {
                intent->current_intent = NPCIntent::Intent::Raid;
            }
            break;

        case NPCArchetype::Archetype::Patrol:
            if (threat_level > 0.3f) {
                intent->current_intent = NPCIntent::Intent::Defend;
            } else {
                intent->current_intent = NPCIntent::Intent::Patrol;
            }
            break;

        case NPCArchetype::Archetype::Miner:
            if (resource_availability > 0.3f && security > 0.3f) {
                intent->current_intent = NPCIntent::Intent::Mine;
            } else {
                intent->current_intent = NPCIntent::Intent::Flee;
            }
            break;

        case NPCArchetype::Archetype::Hauler:
            if (security < 0.2f) {
                intent->current_intent = NPCIntent::Intent::Flee;
            } else {
                intent->current_intent = NPCIntent::Intent::Haul;
            }
            break;

        case NPCArchetype::Archetype::Industrialist:
            if (security < 0.3f) {
                intent->current_intent = NPCIntent::Intent::Flee;
            } else {
                intent->current_intent = NPCIntent::Intent::Produce;
            }
            break;
        }
    }

    // Reset timer on intent change
    if (intent->current_intent != intent->previous_intent) {
        intent->intent_timer = 0.0f;
    }
}

std::string NPCArchetypeSystem::getArchetypeName(ecs::Entity* npc) {
    auto* arch = npc->getComponent<NPCArchetype>();
    if (!arch) return "None";

    switch (arch->archetype) {
    case NPCArchetype::Archetype::Trader:         return "Trader";
    case NPCArchetype::Archetype::Pirate:         return "Pirate";
    case NPCArchetype::Archetype::Patrol:         return "Patrol";
    case NPCArchetype::Archetype::Miner:          return "Miner";
    case NPCArchetype::Archetype::Hauler:         return "Hauler";
    case NPCArchetype::Archetype::Industrialist:  return "Industrialist";
    default:                                      return "Unknown";
    }
}

float NPCArchetypeSystem::getRiskTolerance(ecs::Entity* npc) {
    auto* arch = npc->getComponent<NPCArchetype>();
    return arch ? arch->risk_tolerance : 0.5f;
}

} // namespace atlas
