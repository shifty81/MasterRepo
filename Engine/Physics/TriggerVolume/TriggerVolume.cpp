#include "Engine/Physics/TriggerVolume/TriggerVolume.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Engine {

// ── Shape overlap ────────────────────────────────────────────────────────────

static bool OverlapAABB(const float centre[3], const float half[3], const float p[3])
{
    return std::abs(p[0]-centre[0]) <= half[0] &&
           std::abs(p[1]-centre[1]) <= half[1] &&
           std::abs(p[2]-centre[2]) <= half[2];
}

static bool OverlapSphere(const float centre[3], float radius, const float p[3])
{
    float dx=p[0]-centre[0], dy=p[1]-centre[1], dz=p[2]-centre[2];
    return (dx*dx+dy*dy+dz*dz) <= radius*radius;
}

static bool OverlapCapsule(const float centre[3], float radius, float halfH,
                             const float p[3])
{
    // Axis-aligned vertical capsule: cylinder + hemisphere caps
    float dy = p[1]-centre[1];
    float clampedY = std::max(-halfH, std::min(halfH, dy));
    float dx=p[0]-centre[0], dz=p[2]-centre[2];
    float radDist2 = dx*dx + dz*dz;
    float axialDist2 = (dy-clampedY)*(dy-clampedY);
    return (radDist2 + axialDist2) <= radius*radius;
}

// ── Impl ─────────────────────────────────────────────────────────────────────

struct VolumeEntry {
    uint32_t          id{0};
    TriggerVolumeDesc desc;
    TriggerCb         onEnter;
    TriggerCb         onExit;
    TriggerStayCb     onStay;
    bool              used{false};  // for one-shot
};

struct ActorRecord {
    uint32_t    actorId{0};
    float       position[3]{};
    std::string tag;
    std::unordered_map<uint32_t, float>   inVolumes;   ///< volumeId → time inside
    std::unordered_map<uint32_t, float>   cooldowns;   ///< volumeId → remaining cooldown
};

struct TriggerVolume::Impl {
    std::vector<VolumeEntry>                  volumes;
    std::unordered_map<uint32_t, ActorRecord> actors;
    uint32_t nextVolumeId{1};
    std::unordered_map<std::string, uint32_t> nameToId;

    bool TestOverlap(const VolumeEntry& ve, const float pos[3]) const {
        auto& d = ve.desc;
        switch (d.shape) {
        case TriggerShape::AABB:
            return OverlapAABB(d.centre, d.halfExtents, pos);
        case TriggerShape::Sphere:
            return OverlapSphere(d.centre, d.halfExtents[0], pos);
        case TriggerShape::Capsule:
            return OverlapCapsule(d.centre, d.halfExtents[0], d.halfExtents[1], pos);
        case TriggerShape::OBB:
            // Simplified: treat as AABB for now
            return OverlapAABB(d.centre, d.halfExtents, pos);
        }
        return false;
    }
};

TriggerVolume::TriggerVolume()  : m_impl(new Impl) {}
TriggerVolume::~TriggerVolume() { Shutdown(); delete m_impl; }

void TriggerVolume::Init()     {}
void TriggerVolume::Shutdown() { m_impl->volumes.clear(); }

uint32_t TriggerVolume::Add(const TriggerVolumeDesc& desc)
{
    VolumeEntry ve;
    ve.id   = m_impl->nextVolumeId++;
    ve.desc = desc;
    m_impl->volumes.push_back(ve);
    m_impl->nameToId[desc.id] = ve.id;
    return ve.id;
}

uint32_t TriggerVolume::AddAABB(const std::string& id, const float centre[3],
                                  const float half[3])
{
    TriggerVolumeDesc d; d.id=id; d.shape=TriggerShape::AABB;
    for(int i=0;i<3;i++){d.centre[i]=centre[i]; d.halfExtents[i]=half[i];}
    return Add(d);
}

uint32_t TriggerVolume::AddSphere(const std::string& id, const float centre[3],
                                    float radius)
{
    TriggerVolumeDesc d; d.id=id; d.shape=TriggerShape::Sphere;
    for(int i=0;i<3;i++) d.centre[i]=centre[i];
    d.halfExtents[0]=radius;
    return Add(d);
}

uint32_t TriggerVolume::AddCapsule(const std::string& id, const float centre[3],
                                     float radius, float halfHeight,
                                     const float /*upAxis*/[3])
{
    TriggerVolumeDesc d; d.id=id; d.shape=TriggerShape::Capsule;
    for(int i=0;i<3;i++) d.centre[i]=centre[i];
    d.halfExtents[0]=radius; d.halfExtents[1]=halfHeight;
    return Add(d);
}

void TriggerVolume::Remove(uint32_t volumeId)
{
    auto& v = m_impl->volumes;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& ve){ return ve.id==volumeId; }), v.end());
}

void TriggerVolume::Remove(const std::string& id)
{
    auto it = m_impl->nameToId.find(id);
    if (it != m_impl->nameToId.end()) Remove(it->second);
}

void TriggerVolume::Enable(uint32_t volumeId, bool enabled)
{
    for (auto& ve : m_impl->volumes) if (ve.id==volumeId) { ve.desc.enabled=enabled; return; }
}

bool TriggerVolume::HasVolume(const std::string& id) const
{ return m_impl->nameToId.count(id)>0; }

uint32_t TriggerVolume::GetVolumeId(const std::string& id) const
{
    auto it = m_impl->nameToId.find(id);
    return it != m_impl->nameToId.end() ? it->second : 0;
}

void TriggerVolume::SetCentre(uint32_t volumeId, const float centre[3])
{
    for (auto& ve : m_impl->volumes)
        if (ve.id==volumeId) { for(int i=0;i<3;i++) ve.desc.centre[i]=centre[i]; return; }
}

void TriggerVolume::SetOnEnter(uint32_t volumeId, TriggerCb cb)
{ for(auto& ve:m_impl->volumes) if(ve.id==volumeId){ve.onEnter=cb;return;} }
void TriggerVolume::SetOnExit (uint32_t volumeId, TriggerCb cb)
{ for(auto& ve:m_impl->volumes) if(ve.id==volumeId){ve.onExit=cb;return;} }
void TriggerVolume::SetOnStay (uint32_t volumeId, TriggerStayCb cb)
{ for(auto& ve:m_impl->volumes) if(ve.id==volumeId){ve.onStay=cb;return;} }

void TriggerVolume::SetActorPosition(uint32_t actorId, const float pos[3])
{
    auto& ar = m_impl->actors[actorId];
    ar.actorId = actorId;
    ar.position[0]=pos[0]; ar.position[1]=pos[1]; ar.position[2]=pos[2];
}

void TriggerVolume::SetActorTag(uint32_t actorId, const std::string& tag)
{ m_impl->actors[actorId].tag = tag; }

void TriggerVolume::RemoveActor(uint32_t actorId) { m_impl->actors.erase(actorId); }

void TriggerVolume::Tick()
{
    std::vector<VolumeEntry*> toDisable;

    for (auto& ve : m_impl->volumes) {
        if (!ve.desc.enabled) continue;

        for (auto& [aid, ar] : m_impl->actors) {
            // Tag filter
            if (!ve.desc.filterTag.empty() && ar.tag != ve.desc.filterTag) continue;

            // Cooldown
            float& cool = ar.cooldowns[ve.id];
            if (cool > 0.f) { cool -= 0.016f; continue; } // approx dt; refined below

            bool nowIn  = m_impl->TestOverlap(ve, ar.position);
            bool wasIn  = ar.inVolumes.count(ve.id) > 0;

            if (nowIn && !wasIn) {
                ar.inVolumes[ve.id] = 0.f;
                if (ve.onEnter) ve.onEnter(aid);
                if (ve.desc.oneShot) { toDisable.push_back(&ve); }
                if (ve.desc.cooldown > 0.f) cool = ve.desc.cooldown;
            } else if (nowIn && wasIn) {
                ar.inVolumes[ve.id] += 0.016f;
                if (ve.onStay) ve.onStay(aid, ar.inVolumes[ve.id]);
            } else if (!nowIn && wasIn) {
                float t = ar.inVolumes[ve.id];
                ar.inVolumes.erase(ve.id);
                if (ve.onExit) ve.onExit(aid);
            }
        }
    }
    for (auto* ve : toDisable) ve->desc.enabled = false;
}

bool TriggerVolume::IsActorInVolume(uint32_t actorId, uint32_t volumeId) const
{
    auto it = m_impl->actors.find(actorId);
    return it != m_impl->actors.end() && it->second.inVolumes.count(volumeId)>0;
}

std::vector<uint32_t> TriggerVolume::GetActorsInVolume(uint32_t volumeId) const
{
    std::vector<uint32_t> out;
    for (auto& [aid,ar] : m_impl->actors)
        if (ar.inVolumes.count(volumeId)) out.push_back(aid);
    return out;
}

std::vector<uint32_t> TriggerVolume::GetVolumesForActor(uint32_t actorId) const
{
    std::vector<uint32_t> out;
    auto it = m_impl->actors.find(actorId);
    if (it == m_impl->actors.end()) return out;
    for (auto& [vid,_] : it->second.inVolumes) out.push_back(vid);
    return out;
}

} // namespace Engine
