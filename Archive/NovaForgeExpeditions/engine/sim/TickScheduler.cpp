#include "TickScheduler.h"
#include <thread>

namespace atlas::sim {

void TickScheduler::SetTickRate(uint32_t hz) {
    if (m_tickRateLocked) return;
    m_tickRate = hz > 0 ? hz : 1;
}

void TickScheduler::LockTickRate() {
    m_tickRateLocked = true;
}

bool TickScheduler::IsTickRateLocked() const {
    return m_tickRateLocked;
}

uint32_t TickScheduler::TickRate() const {
    return m_tickRate;
}

float TickScheduler::FixedDeltaTime() const {
    return 1.0f / static_cast<float>(m_tickRate);
}

void TickScheduler::Tick(const std::function<void(float)>& callback) {
    if (m_framePacing) {
        auto now = std::chrono::steady_clock::now();
        if (m_firstTick) {
            m_nextTick = now;
            m_firstTick = false;
        }

        if (now < m_nextTick) {
            std::this_thread::sleep_until(m_nextTick);
        }

        auto tickInterval = std::chrono::microseconds(1000000 / m_tickRate);
        m_nextTick += tickInterval;

        // Prevent spiral of death: if we fell behind more than one full tick,
        // reset the target to now + one interval
        now = std::chrono::steady_clock::now();
        if (m_nextTick < now) {
            m_nextTick = now + tickInterval;
        }
    }

    if (callback) {
        ATLAS_SIM_TICK_BEGIN();
        callback(FixedDeltaTime());
        ATLAS_SIM_TICK_END();
    }
    m_currentTick++;
}

uint64_t TickScheduler::CurrentTick() const {
    return m_currentTick;
}

void TickScheduler::SetFramePacing(bool enabled) {
    m_framePacing = enabled;
    if (enabled) {
        m_firstTick = true;
    }
}

bool TickScheduler::FramePacingEnabled() const {
    return m_framePacing;
}

}
