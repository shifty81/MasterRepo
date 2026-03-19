#include "systems/background_simulation_system.h"
#include "components/game_components.h"

namespace atlas {

using namespace components;

void BackgroundSimulationSystem::initialize(ecs::Entity* galaxy) {
    if (!galaxy->hasComponent<BackgroundSimState>()) {
        galaxy->addComponent(std::make_unique<BackgroundSimState>());
    }
}

void BackgroundSimulationSystem::update(ecs::Entity* galaxy, float dt) {
    auto* state = galaxy->getComponent<BackgroundSimState>();
    if (!state || state->paused) return;

    state->sim_time += dt;
    state->tick_accumulator += dt;
    while (state->tick_accumulator >= state->sim_tick_rate) {
        state->tick_accumulator -= state->sim_tick_rate;
        state->total_ticks++;
    }
}

float BackgroundSimulationSystem::getSimTime(ecs::Entity* galaxy) {
    auto* state = galaxy->getComponent<BackgroundSimState>();
    return state ? state->sim_time : 0.0f;
}

int BackgroundSimulationSystem::getTotalTicks(ecs::Entity* galaxy) {
    auto* state = galaxy->getComponent<BackgroundSimState>();
    return state ? state->total_ticks : 0;
}

void BackgroundSimulationSystem::pause(ecs::Entity* galaxy) {
    auto* state = galaxy->getComponent<BackgroundSimState>();
    if (state) state->paused = true;
}

void BackgroundSimulationSystem::resume(ecs::Entity* galaxy) {
    auto* state = galaxy->getComponent<BackgroundSimState>();
    if (state) state->paused = false;
}

} // namespace atlas
