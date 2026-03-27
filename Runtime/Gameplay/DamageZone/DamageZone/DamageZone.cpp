#include "Runtime/Gameplay/DamageZone/DamageZone/DamageZone.h"
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace Runtime {

struct Zone {
    DamageZoneShape shape;
    float cx{}, cy{}, cz{};
    float hw{1}, hh{1}, hd{1};
    float dps{10};
    uint32_t damageType{0};
    bool enabled{true};
    std::unordered_set<uint32_t> inside;
};

struct DamageZone::Impl {
    std::unordered_map<uint32_t, Zone> zones;
    std::function<void(uint32_t,uint32_t)> onEnter, onExit;
    std::function<void(uint32_t,float,uint32_t)> onDamage;
};

DamageZone::DamageZone()  { m_impl = new Impl; }
DamageZone::~DamageZone() { delete m_impl; }
void DamageZone::Init    () {}
void DamageZone::Shutdown() { Reset(); }
void DamageZone::Reset   () { m_impl->zones.clear(); }

bool DamageZone::AddZone(uint32_t id, DamageZoneShape shape,
                          float cx, float cy, float cz,
                          float hw, float hh, float hd,
                          float dps, uint32_t damageType) {
    if (m_impl->zones.count(id)) return false;
    Zone z; z.shape = shape; z.cx = cx; z.cy = cy; z.cz = cz;
    z.hw = hw; z.hh = hh; z.hd = hd; z.dps = dps; z.damageType = damageType;
    m_impl->zones[id] = z;
    return true;
}
void DamageZone::RemoveZone(uint32_t id) { m_impl->zones.erase(id); }
void DamageZone::RemoveAll () { m_impl->zones.clear(); }
void DamageZone::SetZoneEnabled(uint32_t id, bool on) {
    auto it = m_impl->zones.find(id); if (it != m_impl->zones.end()) it->second.enabled = on;
}
void DamageZone::SetDamagePerSecond(uint32_t id, float dps) {
    auto it = m_impl->zones.find(id); if (it != m_impl->zones.end()) it->second.dps = dps;
}
void DamageZone::SetDamageType(uint32_t id, uint32_t t) {
    auto it = m_impl->zones.find(id); if (it != m_impl->zones.end()) it->second.damageType = t;
}
void DamageZone::MoveZone(uint32_t id, float cx, float cy, float cz) {
    auto it = m_impl->zones.find(id);
    if (it != m_impl->zones.end()) { it->second.cx = cx; it->second.cy = cy; it->second.cz = cz; }
}

void DamageZone::Tick(float dt, QueryFn queryFn) {
    if (!queryFn) return;
    for (auto& [zid, z] : m_impl->zones) {
        if (!z.enabled) continue;
        std::vector<uint32_t> current;
        queryFn(z.cx, z.cy, z.cz, z.hw, z.hh, z.hd, current);
        std::unordered_set<uint32_t> curSet(current.begin(), current.end());
        // enter
        for (uint32_t eid : current)
            if (!z.inside.count(eid)) {
                z.inside.insert(eid);
                if (m_impl->onEnter) m_impl->onEnter(zid, eid);
            }
        // exit
        std::vector<uint32_t> exited;
        for (uint32_t eid : z.inside) if (!curSet.count(eid)) exited.push_back(eid);
        for (uint32_t eid : exited) {
            z.inside.erase(eid);
            if (m_impl->onExit) m_impl->onExit(zid, eid);
        }
        // damage
        float dmg = z.dps * dt;
        for (uint32_t eid : current)
            if (m_impl->onDamage) m_impl->onDamage(eid, dmg, z.damageType);
    }
}

uint32_t DamageZone::GetZoneCount() const { return (uint32_t)m_impl->zones.size(); }
uint32_t DamageZone::GetEntitiesInZone(uint32_t id, std::vector<uint32_t>& out) const {
    auto it = m_impl->zones.find(id); if (it == m_impl->zones.end()) return 0;
    out.assign(it->second.inside.begin(), it->second.inside.end());
    return (uint32_t)out.size();
}
void DamageZone::SetOnEnter (std::function<void(uint32_t,uint32_t)> cb) { m_impl->onEnter  = std::move(cb); }
void DamageZone::SetOnExit  (std::function<void(uint32_t,uint32_t)> cb) { m_impl->onExit   = std::move(cb); }
void DamageZone::SetOnDamage(std::function<void(uint32_t,float,uint32_t)> cb) { m_impl->onDamage = std::move(cb); }

} // namespace Runtime
