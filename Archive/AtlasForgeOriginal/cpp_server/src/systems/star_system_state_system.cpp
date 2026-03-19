#include "systems/star_system_state_system.h"
#include "components/game_components.h"
#include <algorithm>

namespace atlas {

using namespace components;

// Tuning constants for star system state transitions
static constexpr float TRAFFIC_SMOOTHING_RATE         = 0.1f;
static constexpr float MINING_ECONOMY_BOOST           = 0.1f;
static constexpr float PIRATE_ECONOMY_DRAIN           = 0.15f;
static constexpr float PIRATE_SECURITY_IMPACT         = 0.8f;
static constexpr float THREAT_PIRATE_WEIGHT           = 0.6f;
static constexpr float THREAT_SECURITY_WEIGHT         = 0.4f;
static constexpr float PIRATE_PRESSURE_THRESHOLD      = 0.5f;
static constexpr float PIRATE_PRESSURE_INCREASE_RATE  = 0.1f;
static constexpr float PIRATE_PRESSURE_DECAY_RATE     = 0.05f;
static constexpr float SHORTAGE_ECONOMY_THRESHOLD     = 0.3f;
static constexpr float SHORTAGE_INCREASE_RATE         = 0.2f;
static constexpr float SHORTAGE_RECOVERY_RATE         = 0.03f;
static constexpr float LOCKDOWN_THREAT_THRESHOLD      = 0.8f;
static constexpr float FACTION_EROSION_THRESHOLD      = 0.6f;
static constexpr float FACTION_EROSION_RATE           = 0.05f;
static constexpr float FACTION_RECOVERY_RATE          = 0.01f;

void StarSystemStateSystem::initialize(ecs::Entity* system) {
    if (!system->hasComponent<StarSystemState>()) {
        system->addComponent(std::make_unique<StarSystemState>());
    }
}

void StarSystemStateSystem::update(ecs::Entity* system, float dt,
                                    float npc_traffic, float mining_output,
                                    float pirate_activity) {
    auto* state = system->getComponent<StarSystemState>();
    if (!state) return;

    // Traffic tracks NPC/player ship activity with smoothing
    state->traffic += (npc_traffic - state->traffic) * TRAFFIC_SMOOTHING_RATE * dt;
    state->traffic = std::clamp(state->traffic, 0.0f, 1.0f);

    // Economy improves with mining output, degrades with pirate activity
    float economy_delta = (mining_output * MINING_ECONOMY_BOOST - pirate_activity * PIRATE_ECONOMY_DRAIN) * dt;
    state->economy += economy_delta;
    state->economy = std::clamp(state->economy, 0.0f, 1.0f);

    // Security inversely related to pirate activity
    state->security = std::clamp(1.0f - pirate_activity * PIRATE_SECURITY_IMPACT, 0.0f, 1.0f);

    // Threat is a combination of pirate activity and low security
    state->threat = std::clamp(pirate_activity * THREAT_PIRATE_WEIGHT + (1.0f - state->security) * THREAT_SECURITY_WEIGHT, 0.0f, 1.0f);

    // Pirate spawn pressure rises when threat is high
    if (state->threat > PIRATE_PRESSURE_THRESHOLD) {
        state->pirate_spawn_pressure += (state->threat - PIRATE_PRESSURE_THRESHOLD) * dt * PIRATE_PRESSURE_INCREASE_RATE;
    } else {
        state->pirate_spawn_pressure -= PIRATE_PRESSURE_DECAY_RATE * dt;
    }
    state->pirate_spawn_pressure = std::clamp(state->pirate_spawn_pressure, 0.0f, 1.0f);

    // Shortage severity rises when economy is low
    if (state->economy < SHORTAGE_ECONOMY_THRESHOLD) {
        state->shortage_severity += (SHORTAGE_ECONOMY_THRESHOLD - state->economy) * dt * SHORTAGE_INCREASE_RATE;
    } else {
        state->shortage_severity -= SHORTAGE_RECOVERY_RATE * dt;
    }
    state->shortage_severity = std::clamp(state->shortage_severity, 0.0f, 1.0f);

    // Lockdown triggers when threat exceeds threshold
    state->lockdown = state->threat > LOCKDOWN_THREAT_THRESHOLD;

    // Faction influence erodes under sustained pirate pressure
    if (pirate_activity > FACTION_EROSION_THRESHOLD) {
        state->faction_influence -= (pirate_activity - FACTION_EROSION_THRESHOLD) * dt * FACTION_EROSION_RATE;
    } else {
        state->faction_influence += FACTION_RECOVERY_RATE * dt;
    }
    state->faction_influence = std::clamp(state->faction_influence, 0.0f, 1.0f);
}

float StarSystemStateSystem::getSecurity(ecs::Entity* system) {
    auto* state = system->getComponent<StarSystemState>();
    return state ? state->security : 0.5f;
}

float StarSystemStateSystem::getEconomy(ecs::Entity* system) {
    auto* state = system->getComponent<StarSystemState>();
    return state ? state->economy : 0.5f;
}

float StarSystemStateSystem::getThreat(ecs::Entity* system) {
    auto* state = system->getComponent<StarSystemState>();
    return state ? state->threat : 0.0f;
}

bool StarSystemStateSystem::isLockdown(ecs::Entity* system) {
    auto* state = system->getComponent<StarSystemState>();
    return state ? state->lockdown : false;
}

} // namespace atlas
