#include "Runtime/Combat/CombatSystem.h"
#include <algorithm>
#include <cmath>

namespace Runtime {

// ── ShieldFacings ─────────────────────────────────────────────────────────────

void ShieldFacings::RegenTick(float dt) {
    for (size_t i = 0; i < 4; ++i)
        hp[i] = std::min(maxHp[i], hp[i] + regenRate[i] * dt);
}

// ── CombatSystem ──────────────────────────────────────────────────────────────

void CombatSystem::Init() {
    m_entities.clear();
    m_projectiles.clear();
    m_nextEntityId = 1;
    m_nextProjId   = 1;
}

uint32_t CombatSystem::Register(const CombatEntityDesc& desc) {
    CombatEntity e;
    e.id        = m_nextEntityId++;
    e.name      = desc.name;
    e.posX      = desc.posX;
    e.posY      = desc.posY;
    e.posZ      = desc.posZ;
    e.rotYawDeg = desc.rotYawDeg;
    e.hullHP    = desc.hullHP;
    e.maxHullHP = desc.hullHP;
    e.weapons   = desc.weapons;
    e.weaponCooldowns.assign(desc.weapons.size(), 0.0f);
    e.factionId = desc.factionId;

    float shieldPerFacing = desc.shieldHP / 4.0f;
    for (int i = 0; i < 4; ++i) {
        e.shields.hp[i]        = shieldPerFacing;
        e.shields.maxHp[i]     = shieldPerFacing;
        e.shields.regenRate[i] = desc.shieldRegenRate / 4.0f;
    }

    uint32_t id = e.id;
    m_entities[id] = std::move(e);
    return id;
}

void CombatSystem::Deregister(uint32_t entityId) { m_entities.erase(entityId); }

CombatEntity* CombatSystem::GetEntity(uint32_t entityId) {
    auto it = m_entities.find(entityId);
    return it != m_entities.end() ? &it->second : nullptr;
}

std::vector<uint32_t> CombatSystem::AllEntityIds() const {
    std::vector<uint32_t> ids;
    for (const auto& [id, _] : m_entities) ids.push_back(id);
    return ids;
}

// ── Combat actions ────────────────────────────────────────────────────────────

bool CombatSystem::FireWeapon(uint32_t attackerId, uint32_t weaponIndex,
                               uint32_t targetId) {
    CombatEntity* atk = GetEntity(attackerId);
    CombatEntity* tgt = GetEntity(targetId);
    if (!atk || !tgt || atk->destroyed || tgt->destroyed) return false;
    if (weaponIndex >= atk->weapons.size()) return false;
    if (atk->weaponCooldowns[weaponIndex] > 0.0f) return false;

    const WeaponDef& wpn = atk->weapons[weaponIndex];
    float dist = Distance(attackerId, targetId);
    if (dist > wpn.range) return false;
    if (!IsInWeaponArc(attackerId, weaponIndex, targetId)) return false;

    atk->weaponCooldowns[weaponIndex] = wpn.cooldown;

    if (wpn.projectileSpeed > 0.0f) {
        // Spawn projectile
        Projectile p;
        p.id             = m_nextProjId++;
        p.sourceEntityId = attackerId;
        p.targetEntityId = wpn.homing ? targetId : 0;
        p.posX = atk->posX; p.posY = atk->posY; p.posZ = atk->posZ;
        float dx = tgt->posX - atk->posX;
        float dz = tgt->posZ - atk->posZ;
        float len = std::sqrt(dx*dx + dz*dz);
        if (len > 0.f) { dx /= len; dz /= len; }
        p.dirX  = dx; p.dirY = 0; p.dirZ = dz;
        p.speed  = wpn.projectileSpeed;
        p.damage = wpn.damage;
        p.maxRange    = wpn.range;
        p.type = wpn.type;
        m_projectiles.push_back(p);
    } else {
        // Instant-hit
        ApplyDamage(targetId, wpn.damage, attackerId);
    }
    return true;
}

DamageResult CombatSystem::ApplyDamage(uint32_t targetId, float rawDamage,
                                        uint32_t attackerId) {
    DamageResult result;
    CombatEntity* tgt = GetEntity(targetId);
    if (!tgt || tgt->destroyed) return result;
    result.targetId = targetId;

    // Determine which shield facing takes the hit
    float atkX = 0, atkZ = 0;
    if (attackerId != 0) {
        CombatEntity* atk = GetEntity(attackerId);
        if (atk) { atkX = atk->posX; atkZ = atk->posZ; }
    }
    ShieldFacing facing = FacingFrom(*tgt, atkX, atkZ);
    result.facing = facing;

    // Absorb with shields first
    float& shieldHP = tgt->shields.hp[(int)facing];
    float absorbed  = std::min(shieldHP, rawDamage);
    shieldHP -= absorbed;
    float remainder = rawDamage - absorbed;
    result.shieldDamage = absorbed;

    // Remaining hits hull
    tgt->hullHP -= remainder;
    result.hullDamage = remainder;

    if (tgt->hullHP <= 0.0f) {
        tgt->hullHP   = 0.0f;
        tgt->destroyed = true;
        result.destroyed = true;
        if (m_onDestroy) m_onDestroy(targetId, attackerId);
    }

    if (m_onDamage) m_onDamage(result);
    return result;
}

// ── Position ─────────────────────────────────────────────────────────────────

void CombatSystem::SetPosition(uint32_t id, float x, float y, float z) {
    if (auto* e = GetEntity(id)) { e->posX = x; e->posY = y; e->posZ = z; }
}
void CombatSystem::SetYaw(uint32_t id, float yawDeg) {
    if (auto* e = GetEntity(id)) e->rotYawDeg = yawDeg;
}

// ── Tick ─────────────────────────────────────────────────────────────────────

void CombatSystem::Tick(float dt) {
    TickShieldRegen(dt);
    TickProjectiles(dt);
    // Reduce weapon cooldowns
    for (auto& [id, e] : m_entities)
        for (float& cd : e.weaponCooldowns)
            cd = std::max(0.0f, cd - dt);
}

void CombatSystem::TickShieldRegen(float dt) {
    for (auto& [id, e] : m_entities)
        if (!e.destroyed) e.shields.RegenTick(dt);
}

void CombatSystem::TickProjectiles(float dt) {
    for (auto it = m_projectiles.begin(); it != m_projectiles.end(); ) {
        Projectile& p = *it;
        // Homing: steer towards target
        if (p.targetEntityId != 0) {
            CombatEntity* tgt = GetEntity(p.targetEntityId);
            if (tgt && !tgt->destroyed) {
                float dx = tgt->posX - p.posX;
                float dz = tgt->posZ - p.posZ;
                float len = std::sqrt(dx*dx + dz*dz);
                if (len > 0.f) { p.dirX = dx/len; p.dirZ = dz/len; }
            }
        }
        float move = p.speed * dt;
        p.posX += p.dirX * move;
        p.posZ += p.dirZ * move;
        p.travelledRange += move;

        bool remove = false;
        if (p.travelledRange >= p.maxRange) {
            remove = true;
        } else {
            // Check collision with any enemy entity
            for (auto& [id, e] : m_entities) {
                if (id == p.sourceEntityId || e.destroyed) continue;
                float dx = e.posX - p.posX;
                float dz = e.posZ - p.posZ;
                float dist = std::sqrt(dx*dx + dz*dz);
                if (dist < 5.0f) { // 5-unit hit radius
                    ApplyDamage(id, p.damage, p.sourceEntityId);
                    remove = true;
                    break;
                }
            }
        }

        it = remove ? m_projectiles.erase(it) : ++it;
    }
}

// ── Queries ───────────────────────────────────────────────────────────────────

float CombatSystem::Distance(uint32_t a, uint32_t b) const {
    const CombatEntity* ea = const_cast<CombatSystem*>(this)->GetEntity(a);
    const CombatEntity* eb = const_cast<CombatSystem*>(this)->GetEntity(b);
    if (!ea || !eb) return 1e9f;
    float dx = ea->posX - eb->posX;
    float dy = ea->posY - eb->posY;
    float dz = ea->posZ - eb->posZ;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

bool CombatSystem::IsInWeaponArc(uint32_t attackerId, uint32_t weaponIndex,
                                  uint32_t targetId) const {
    const CombatEntity* atk = const_cast<CombatSystem*>(this)->GetEntity(attackerId);
    const CombatEntity* tgt = const_cast<CombatSystem*>(this)->GetEntity(targetId);
    if (!atk || !tgt || weaponIndex >= atk->weapons.size()) return false;
    float arcDeg = atk->weapons[weaponIndex].arcDeg;
    if (arcDeg >= 360.f) return true;
    float dx = tgt->posX - atk->posX;
    float dz = tgt->posZ - atk->posZ;
    float angleToTarget = std::atan2(dx, dz) * (180.f / 3.14159265f);
    float diff = angleToTarget - atk->rotYawDeg;
    // Normalise to [−180, 180]
    while (diff >  180.f) diff -= 360.f;
    while (diff < -180.f) diff += 360.f;
    return std::abs(diff) <= arcDeg * 0.5f;
}

std::vector<uint32_t> CombatSystem::GetEntitiesInRange(uint32_t entityId,
                                                         float range) const {
    std::vector<uint32_t> result;
    for (const auto& [id, _] : m_entities)
        if (id != entityId && const_cast<CombatSystem*>(this)->Distance(entityId, id) <= range)
            result.push_back(id);
    return result;
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void CombatSystem::SetDamageCallback(DamageEventFn fn)  { m_onDamage  = std::move(fn); }
void CombatSystem::SetDestroyCallback(DestroyEventFn fn){ m_onDestroy = std::move(fn); }

// ── Stats ─────────────────────────────────────────────────────────────────────

size_t CombatSystem::EntityCount()     const { return m_entities.size(); }
size_t CombatSystem::ProjectileCount() const { return m_projectiles.size(); }

// ── Private helpers ───────────────────────────────────────────────────────────

ShieldFacing CombatSystem::FacingFrom(const CombatEntity& target,
                                       float attackerX, float attackerZ) const {
    float dx = attackerX - target.posX;
    float dz = attackerZ - target.posZ;
    float angle = std::atan2(dx, dz) * (180.f / 3.14159265f) - target.rotYawDeg;
    while (angle >  180.f) angle -= 360.f;
    while (angle < -180.f) angle += 360.f;
    if (angle >= -45.f  && angle <  45.f)  return ShieldFacing::Front;
    if (angle >=  45.f  && angle < 135.f)  return ShieldFacing::Right;
    if (angle >= -135.f && angle < -45.f)  return ShieldFacing::Left;
    return ShieldFacing::Rear;
}

} // namespace Runtime
