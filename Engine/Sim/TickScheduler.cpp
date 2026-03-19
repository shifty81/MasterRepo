#include "Engine/Sim/TickScheduler.h"

namespace Engine::Sim {

void TickScheduler::SetTickRate(uint32_t hz) {
    m_tickRate = hz > 0 ? hz : 1;
} // namespace Engine::Sim

uint32_t TickScheduler::TickRate() const {
    return m_tickRate;
} // namespace Engine::Sim

float TickScheduler::FixedDeltaTime() const {
    return 1.0f / static_cast<float>(m_tickRate);
} // namespace Engine::Sim

void TickScheduler::Tick(const std::function<void(float)>& callback) {
    if (callback) {
        callback(FixedDeltaTime());
    }
    m_currentTick++;
} // namespace Engine::Sim

uint64_t TickScheduler::CurrentTick() const {
    return m_currentTick;
} // namespace Engine::Sim

} // namespace Engine::Sim
