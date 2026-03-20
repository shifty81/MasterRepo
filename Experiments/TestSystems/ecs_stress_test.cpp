// ecs_stress_test.cpp — Standalone ECS stress test prototype
// Tests: 10 000 entities, 3 components each, 100 update ticks
// No engine includes — fully self-contained.
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

namespace Experiments {

// ──────────────────────────────────────────────────────────────
// Entity
// ──────────────────────────────────────────────────────────────

struct Entity {
    uint32_t id = 0;
};

// ──────────────────────────────────────────────────────────────
// Components
// ──────────────────────────────────────────────────────────────

struct PositionComponent {
    float x = 0.f, y = 0.f, z = 0.f;
};

struct VelocityComponent {
    float vx = 0.f, vy = 0.f, vz = 0.f;
};

struct HealthComponent {
    float hp    = 100.f;
    float maxHp = 100.f;
};

// ──────────────────────────────────────────────────────────────
// ComponentStore<T> — dense storage indexed by entity ID
// ──────────────────────────────────────────────────────────────

template<typename T>
class ComponentStore {
public:
    void Add(uint32_t entityId, T component) {
        if (entityId >= m_data.size())
            m_data.resize(entityId + 1);
        m_data[entityId] = std::move(component);
    }

    T& Get(uint32_t entityId) { return m_data[entityId]; }
    const T& Get(uint32_t entityId) const { return m_data[entityId]; }

    size_t Size() const { return m_data.size(); }

private:
    std::vector<T> m_data;
};

// ──────────────────────────────────────────────────────────────
// MovementSystem — position += velocity each tick
// ──────────────────────────────────────────────────────────────

class MovementSystem {
public:
    void Update(ComponentStore<PositionComponent>& positions,
                const ComponentStore<VelocityComponent>& velocities,
                uint32_t entityCount) {
        for (uint32_t id = 0; id < entityCount; ++id) {
            auto& pos       = positions.Get(id);
            const auto& vel = velocities.Get(id);
            pos.x += vel.vx;
            pos.y += vel.vy;
            pos.z += vel.vz;
        }
    }
};

// ──────────────────────────────────────────────────────────────
// DamageSystem — reduces hp by 0.1 per tick
// ──────────────────────────────────────────────────────────────

class DamageSystem {
public:
    static constexpr float kDamagePerTick = 0.1f;

    void Update(ComponentStore<HealthComponent>& healths, uint32_t entityCount) {
        for (uint32_t id = 0; id < entityCount; ++id) {
            auto& h = healths.Get(id);
            h.hp -= kDamagePerTick;
            if (h.hp < 0.f) h.hp = 0.f;
        }
    }
};

} // namespace Experiments

// ──────────────────────────────────────────────────────────────
// Standalone main
// ──────────────────────────────────────────────────────────────

int main() {
    using namespace Experiments;
    constexpr uint32_t kEntities = 10000;
    constexpr int      kTicks    = 100;

    std::cout << "[ecs_stress_test] Spawning " << kEntities << " entities...\n";

    ComponentStore<PositionComponent> positions;
    ComponentStore<VelocityComponent> velocities;
    ComponentStore<HealthComponent>   healths;

    for (uint32_t id = 0; id < kEntities; ++id) {
        float f = static_cast<float>(id);
        positions.Add(id, { f * 0.01f, f * 0.02f, f * 0.005f });
        velocities.Add(id, { 0.1f + f * 0.0001f,
                              0.05f - f * 0.00005f,
                              0.02f + f * 0.00002f });
        healths.Add(id, { 100.f, 100.f });
    }

    MovementSystem moveSys;
    DamageSystem   dmgSys;

    auto t0 = std::chrono::high_resolution_clock::now();

    for (int tick = 0; tick < kTicks; ++tick) {
        moveSys.Update(positions, velocities, kEntities);
        dmgSys.Update(healths, kEntities);
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "[ecs_stress_test] " << kTicks << " ticks completed in "
              << ms << " ms  (" << (ms / kTicks) << " ms/tick)\n";

    // Print final state of entity #42 as a sample
    constexpr uint32_t kSample = 42;
    const auto& pos  = positions.Get(kSample);
    const auto& vel  = velocities.Get(kSample);
    const auto& hp   = healths.Get(kSample);
    std::cout << "[ecs_stress_test] Entity #" << kSample << " final state:\n";
    std::cout << "  pos  = (" << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
    std::cout << "  vel  = (" << vel.vx << ", " << vel.vy << ", " << vel.vz << ")\n";
    std::cout << "  hp   = " << hp.hp << " / " << hp.maxHp << "\n";

    std::cout << "[ecs_stress_test] Done.\n";
    return 0;
}
