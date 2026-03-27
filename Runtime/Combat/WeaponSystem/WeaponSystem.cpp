#include "Runtime/Combat/WeaponSystem/WeaponSystem.h"
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Runtime {

using SlotKey = uint64_t;
static SlotKey MakeKey(uint32_t actorId, uint32_t slot) {
    return ((uint64_t)actorId << 32) | slot;
}

struct WeaponSystem::Impl {
    std::unordered_map<std::string, WeaponTypeDesc>   types;
    std::unordered_map<SlotKey, WeaponState>          weapons;

    ShotCb   shotCb;
    ReloadCb reloadCb;
    DryCb    dryCb;

    const WeaponTypeDesc* GetType(const std::string& id) const {
        auto it = types.find(id);
        return it != types.end() ? &it->second : nullptr;
    }

    void FireShot(WeaponState& ws, const WeaponTypeDesc& td,
                   const float* origin, const float* dir) {
        if (ws.magazine == 0) {
            if (dryCb) dryCb(ws.actorId, ws.slotId);
            return;
        }
        ws.magazine--;
        ws.fireCooldown = td.fireRate;
        ws.spread = std::min(ws.spread + td.spreadPerShot, td.spreadBase * 10.f);
        ws.heat   = std::min(ws.heat + td.heatPerShot, 100.f);
        if (td.heatPerShot > 0.f && ws.heat >= 100.f) ws.overheated = true;

        if (shotCb) {
            static const float fwd[3] = {0,0,1};
            ShotEvent e;
            e.actorId      = ws.actorId;
            e.weaponTypeId = ws.typeId;
            e.damage       = td.damage;
            e.range        = td.range;
            e.spread       = ws.spread;
            e.muzzleEffect = td.muzzleEffect;
            e.shotSound    = td.shotSound;
            const float* o = origin ? origin : fwd;
            const float* d = dir    ? dir    : fwd;
            for (int i=0;i<3;i++) { e.origin[i]=o[i]; e.direction[i]=d[i]; }
            shotCb(e);
        }
    }
};

WeaponSystem::WeaponSystem()  : m_impl(new Impl) {}
WeaponSystem::~WeaponSystem() { Shutdown(); delete m_impl; }

void WeaponSystem::Init()     {}
void WeaponSystem::Shutdown() { m_impl->weapons.clear(); }

void WeaponSystem::RegisterType(const WeaponTypeDesc& desc)   { m_impl->types[desc.typeId]=desc; }
void WeaponSystem::UnregisterType(const std::string& id)       { m_impl->types.erase(id); }
bool WeaponSystem::HasType(const std::string& id) const        { return m_impl->types.count(id)>0; }

uint32_t WeaponSystem::EquipWeapon(uint32_t actorId, const std::string& typeId, uint32_t slot)
{
    const auto* td = m_impl->GetType(typeId);
    WeaponState ws;
    ws.actorId   = actorId;
    ws.slotId    = slot;
    ws.typeId    = typeId;
    ws.magazine  = td ? td->magazineSize : 0;
    ws.reserve   = td ? td->reserveMax   : 0;
    m_impl->weapons[MakeKey(actorId,slot)] = ws;
    return slot;
}

void WeaponSystem::UnequipWeapon(uint32_t actorId, uint32_t slot)
{
    m_impl->weapons.erase(MakeKey(actorId,slot));
}

void WeaponSystem::UnequipAll(uint32_t actorId)
{
    std::vector<SlotKey> rem;
    for (auto& [k,_] : m_impl->weapons)
        if ((uint32_t)(k>>32) == actorId) rem.push_back(k);
    for (auto k : rem) m_impl->weapons.erase(k);
}

bool WeaponSystem::HasWeapon(uint32_t actorId, uint32_t slot) const
{
    return m_impl->weapons.count(MakeKey(actorId,slot))>0;
}

void WeaponSystem::PullTrigger(uint32_t actorId, uint32_t slot,
                                 const float* origin, const float* direction)
{
    auto it = m_impl->weapons.find(MakeKey(actorId,slot));
    if (it == m_impl->weapons.end()) return;
    auto& ws = it->second;
    if (ws.overheated || ws.reloadPhase != ReloadPhase::None) return;
    const auto* td = m_impl->GetType(ws.typeId);
    if (!td) return;

    ws.triggerHeld = true;

    if (ws.fireCooldown > 0.f) return;

    switch (td->fireMode) {
    case FireMode::SemiAuto:
        if (ws.fireCooldown <= 0.f) m_impl->FireShot(ws, *td, origin, direction);
        break;
    case FireMode::FullAuto:
        m_impl->FireShot(ws, *td, origin, direction);
        break;
    case FireMode::Burst:
        if (ws.burstShotsLeft == 0) {
            ws.burstShotsLeft = td->burstCount;
            ws.burstTimer = 0.f;
        }
        break;
    case FireMode::Charge:
        ws.chargeLevel = 0.f; // start charging in Tick
        break;
    }
}

void WeaponSystem::ReleaseTrigger(uint32_t actorId, uint32_t slot)
{
    auto it = m_impl->weapons.find(MakeKey(actorId,slot));
    if (it == m_impl->weapons.end()) return;
    auto& ws = it->second;
    const auto* td = m_impl->GetType(ws.typeId);
    ws.triggerHeld = false;

    if (td && td->fireMode == FireMode::Charge && ws.chargeLevel >= 1.f) {
        m_impl->FireShot(ws, *td, nullptr, nullptr);
        ws.chargeLevel = 0.f;
    }
}

bool WeaponSystem::StartReload(uint32_t actorId, uint32_t slot)
{
    auto it = m_impl->weapons.find(MakeKey(actorId,slot));
    if (it == m_impl->weapons.end()) return false;
    auto& ws = it->second;
    if (ws.reloadPhase != ReloadPhase::None) return false;
    const auto* td = m_impl->GetType(ws.typeId);
    if (!td || ws.reserve == 0) return false;
    ws.reloadPhase = ReloadPhase::Started;
    ws.reloadTimer = td->reloadTime;
    return true;
}

void WeaponSystem::CancelReload(uint32_t actorId, uint32_t slot)
{
    auto it = m_impl->weapons.find(MakeKey(actorId,slot));
    if (it != m_impl->weapons.end()) {
        it->second.reloadPhase = ReloadPhase::None;
        it->second.reloadTimer = 0.f;
    }
}

bool WeaponSystem::IsReloading(uint32_t actorId, uint32_t slot) const
{
    auto it = m_impl->weapons.find(MakeKey(actorId,slot));
    return it != m_impl->weapons.end() && it->second.reloadPhase != ReloadPhase::None;
}

void WeaponSystem::AddAmmo(uint32_t actorId, const std::string& typeId, uint32_t amount)
{
    for (auto& [k, ws] : m_impl->weapons) {
        if ((uint32_t)(k>>32)==actorId && ws.typeId==typeId) {
            const auto* td = m_impl->GetType(typeId);
            uint32_t cap = td ? td->reserveMax : 999;
            ws.reserve = std::min(ws.reserve+amount, cap);
        }
    }
}

uint32_t WeaponSystem::GetMagazine(uint32_t actorId, uint32_t slot) const
{
    auto it = m_impl->weapons.find(MakeKey(actorId,slot));
    return it != m_impl->weapons.end() ? it->second.magazine : 0;
}

uint32_t WeaponSystem::GetReserve(uint32_t actorId, uint32_t slot) const
{
    auto it = m_impl->weapons.find(MakeKey(actorId,slot));
    return it != m_impl->weapons.end() ? it->second.reserve : 0;
}

const WeaponState* WeaponSystem::GetWeaponState(uint32_t actorId, uint32_t slot) const
{
    auto it = m_impl->weapons.find(MakeKey(actorId,slot));
    return it != m_impl->weapons.end() ? &it->second : nullptr;
}

void WeaponSystem::Tick(float dt)
{
    for (auto& [k, ws] : m_impl->weapons) {
        const auto* td = m_impl->GetType(ws.typeId);
        if (!td) continue;

        // Cooldown
        ws.fireCooldown = std::max(0.f, ws.fireCooldown - dt);

        // Heat cool-down
        if (!ws.triggerHeld) {
            ws.heat = std::max(0.f, ws.heat - td->heatCoolRate*dt);
            if (ws.overheated && ws.heat <= 0.f) ws.overheated = false;
        }

        // Spread recovery
        ws.spread = std::max(td->spreadBase, ws.spread - td->spreadRecovery*dt);

        // Burst
        if (td->fireMode == FireMode::Burst && ws.burstShotsLeft > 0) {
            ws.burstTimer -= dt;
            if (ws.burstTimer <= 0.f) {
                m_impl->FireShot(ws, *td, nullptr, nullptr);
                ws.burstShotsLeft--;
                ws.burstTimer = td->burstInterval;
            }
        }

        // Charge
        if (td->fireMode == FireMode::Charge && ws.triggerHeld)
            ws.chargeLevel = std::min(1.f, ws.chargeLevel + dt/td->chargeTime);

        // Full auto
        if (td->fireMode == FireMode::FullAuto && ws.triggerHeld && ws.fireCooldown<=0.f)
            m_impl->FireShot(ws, *td, nullptr, nullptr);

        // Reload
        if (ws.reloadPhase != ReloadPhase::None) {
            ws.reloadTimer -= dt;
            if (ws.reloadPhase == ReloadPhase::Started)
                ws.reloadPhase = ReloadPhase::InProgress;
            if (ws.reloadTimer <= 0.f) {
                ws.reloadPhase = ReloadPhase::Chambered;
                uint32_t need = td->magazineSize - ws.magazine;
                uint32_t fill = std::min(need, ws.reserve);
                ws.magazine += fill;
                ws.reserve  -= fill;
                ws.reloadPhase = ReloadPhase::None;
                if (m_impl->reloadCb) {
                    ReloadEvent re;
                    re.actorId=ws.actorId; re.weaponTypeId=ws.typeId;
                    re.newMagazine=ws.magazine; re.newReserve=ws.reserve;
                    m_impl->reloadCb(re);
                }
            }
        }
    }
}

void WeaponSystem::SetProjectileDispatch(ShotCb cb)  { m_impl->shotCb   = cb; }
void WeaponSystem::SetOnReloadComplete(ReloadCb cb)   { m_impl->reloadCb = cb; }
void WeaponSystem::SetOnDryFire(DryCb cb)             { m_impl->dryCb    = cb; }

bool WeaponSystem::LoadTypesFromJSON(const std::string& path)
{
    std::ifstream f(path);
    return f.good();
}

} // namespace Runtime
