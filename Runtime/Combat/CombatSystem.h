#pragma once
/**
 * @file CombatSystem.h
 * @brief Ship-to-ship combat: targeting, weapon arcs, shield facings, missiles.
 *
 * Manages:
 *   - CombatEntity registration (ships with health, shields, weapons, position)
 *   - Weapon firing with arc/range checks
 *   - Shield-facing damage model (front/rear/left/right, each facing has own HP)
 *   - Missile tracking with intercept prediction
 *   - Kill/damage events for integration with FactionSystem and ProgressionSystem
 *
 * Usage:
 * @code
 *   CombatSystem combat;
 *   combat.Init();
 *   uint32_t playerShip = combat.Register({"Player Ship", {0,0,0}, 1000, 500});
 *   uint32_t enemyShip  = combat.Register({"Pirate", {100,0,0}, 400, 200});
 *   combat.FireWeapon(playerShip, 0, enemyShip);  // weapon slot 0
 *   combat.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>

namespace Runtime {

// ── Shield facings ────────────────────────────────────────────────────────────

enum class ShieldFacing : uint8_t { Front = 0, Rear, Left, Right, COUNT };

struct ShieldFacings {
    std::array<float, 4> hp{};       ///< per-facing HP
    std::array<float, 4> maxHp{};
    std::array<float, 4> regenRate{}; ///< HP per second per facing

    float Total()    const { float t = 0; for (auto v : hp) t += v; return t; }
    float TotalMax() const { float t = 0; for (auto v : maxHp) t += v; return t; }
    void  RegenTick(float dt);
};

// ── Weapon definition ─────────────────────────────────────────────────────────

enum class WeaponType : uint8_t { Laser, Railgun, Missile, Torpedo, PDS };

struct WeaponDef {
    std::string  name;
    WeaponType   type{WeaponType::Laser};
    float        damage{10.0f};
    float        range{500.0f};
    float        cooldown{1.0f};    ///< seconds between shots
    float        arcDeg{90.0f};     ///< fire arc in degrees (0 = forward only)
    float        projectileSpeed{0.0f}; ///< 0 = instant-hit
    bool         homing{false};     ///< for missiles/torpedoes
};

// ── Combat entity ─────────────────────────────────────────────────────────────

struct CombatEntityDesc {
    std::string  name;
    float        posX{0}, posY{0}, posZ{0};
    float        rotYawDeg{0};
    float        hullHP{1000.0f};
    float        shieldHP{500.0f};   ///< distributed equally across all facings
    float        shieldRegenRate{5.0f};
    std::vector<WeaponDef> weapons;
    uint32_t     factionId{0};
};

struct CombatEntity {
    uint32_t     id{0};
    std::string  name;
    float        posX{0}, posY{0}, posZ{0};
    float        rotYawDeg{0};
    float        hullHP{0}, maxHullHP{0};
    ShieldFacings shields{};
    std::vector<WeaponDef>  weapons;
    std::vector<float>      weaponCooldowns; ///< parallel to weapons
    uint32_t     factionId{0};
    bool         destroyed{false};
};

// ── In-flight projectile / missile ────────────────────────────────────────────

struct Projectile {
    uint32_t id{0};
    uint32_t sourceEntityId{0};
    uint32_t targetEntityId{0};   ///< homing target (0 = not homing)
    float    posX{0}, posY{0}, posZ{0};
    float    dirX{0}, dirY{0}, dirZ{0};
    float    speed{0};
    float    damage{0};
    float    maxRange{0};
    float    travelledRange{0};
    WeaponType type{WeaponType::Laser};
};

// ── Events ────────────────────────────────────────────────────────────────────

struct DamageResult {
    uint32_t    targetId{0};
    float       hullDamage{0};
    float       shieldDamage{0};
    ShieldFacing facing{ShieldFacing::Front};
    bool        destroyed{false};
};

using DamageEventFn  = std::function<void(const DamageResult&)>;
using DestroyEventFn = std::function<void(uint32_t destroyedId, uint32_t killerId)>;

// ── CombatSystem ──────────────────────────────────────────────────────────────

class CombatSystem {
public:
    void Init();

    // ── Registration ──────────────────────────────────────────────────────
    uint32_t     Register(const CombatEntityDesc& desc);
    void         Deregister(uint32_t entityId);
    CombatEntity* GetEntity(uint32_t entityId);
    std::vector<uint32_t> AllEntityIds() const;

    // ── Combat actions ────────────────────────────────────────────────────
    /// Fire weapon slot weaponIndex at target. Returns true if within arc/range.
    bool FireWeapon(uint32_t attackerId, uint32_t weaponIndex, uint32_t targetId);

    /// Directly apply damage to target (bypasses arc/range check).
    DamageResult ApplyDamage(uint32_t targetId, float rawDamage,
                              uint32_t attackerId = 0);

    // ── Position updates ──────────────────────────────────────────────────
    void SetPosition(uint32_t entityId, float x, float y, float z);
    void SetYaw(uint32_t entityId, float yawDeg);

    // ── Tick ──────────────────────────────────────────────────────────────
    void Tick(float dt);

    // ── Queries ───────────────────────────────────────────────────────────
    float Distance(uint32_t entityA, uint32_t entityB) const;
    bool  IsInWeaponArc(uint32_t attackerId, uint32_t weaponIndex,
                         uint32_t targetId) const;
    std::vector<uint32_t> GetEntitiesInRange(uint32_t entityId,
                                              float range) const;

    // ── Callbacks ─────────────────────────────────────────────────────────
    void SetDamageCallback(DamageEventFn fn);
    void SetDestroyCallback(DestroyEventFn fn);

    // ── Stats ─────────────────────────────────────────────────────────────
    size_t EntityCount()     const;
    size_t ProjectileCount() const;

private:
    void TickProjectiles(float dt);
    void TickShieldRegen(float dt);
    ShieldFacing FacingFrom(const CombatEntity& target,
                             float attackerX, float attackerZ) const;
    float AngleBetween(float ax, float az, float bx, float bz) const;

    uint32_t m_nextEntityId{1};
    uint32_t m_nextProjId{1};
    std::unordered_map<uint32_t, CombatEntity> m_entities;
    std::vector<Projectile>                    m_projectiles;
    DamageEventFn  m_onDamage;
    DestroyEventFn m_onDestroy;
};

} // namespace Runtime
