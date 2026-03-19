#pragma once
#include <cstdint>
#include <unordered_map>
#include "Runtime/ECS/ECS.h"

namespace Runtime::Damage {

enum class DamageType {
    Physical,
    Fire,
    Ice,
    Electric,
    Poison,
    Explosive,
    True // ignores all resistances
};

struct DamageInfo {
    float                  amount = 0.0f;
    DamageType             type   = DamageType::Physical;
    Runtime::ECS::EntityID source = 0;
};

struct Health {
    float current  = 100.0f;
    float maximum  = 100.0f;
    float regenRate = 0.0f; // HP per second
    bool  isDead   = false;

    explicit Health(float max = 100.0f)
        : current(max), maximum(max) {}

    float GetPercent() const {
        return maximum > 0.0f ? current / maximum : 0.0f;
    }
};

class DamageSystem {
public:
    // Returns or creates a Health entry for the entity.
    Health& GetOrCreate(Runtime::ECS::EntityID entity);

    // Apply damage, clamping current HP to [0, maximum].
    void ApplyDamage(Runtime::ECS::EntityID entity, const DamageInfo& dmg);

    // Apply positive healing, clamping to maximum.
    void ApplyHealing(Runtime::ECS::EntityID entity, float amount);

    // Tick regen for all living entities.
    void Update(float dt);

    bool IsDead(Runtime::ECS::EntityID entity) const;

    // Immediately set HP to 0 and mark dead.
    void Kill(Runtime::ECS::EntityID entity);

    // Revive a dead entity at `healthPercent` of maximum (clamped 0–1).
    void Revive(Runtime::ECS::EntityID entity, float healthPercent = 1.0f);

private:
    std::unordered_map<Runtime::ECS::EntityID, Health> m_health;
};

} // namespace Runtime::Damage
