#include "Engine/Render/Decal/DecalSystem/DecalSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

struct DecalSystem::Impl {
    std::vector<DecalInstance> decals;
    uint32_t maxDecals{128};
    uint32_t nextId{1};
    std::function<void(uint32_t)> onExpire;

    DecalInstance* Find(uint32_t id) {
        for (auto& d : decals) if (d.id==id) return &d; return nullptr;
    }
    const DecalInstance* Find(uint32_t id) const {
        for (auto& d : decals) if (d.id==id) return &d; return nullptr;
    }
};

DecalSystem::DecalSystem()  : m_impl(new Impl) {}
DecalSystem::~DecalSystem() { Shutdown(); delete m_impl; }

void DecalSystem::Init(uint32_t maxDecals) { m_impl->maxDecals=maxDecals; }
void DecalSystem::Shutdown() { m_impl->decals.clear(); }

uint32_t DecalSystem::Spawn(const DecalDesc& desc)
{
    // Evict oldest if budget exceeded
    while ((uint32_t)m_impl->decals.size() >= m_impl->maxDecals && !m_impl->decals.empty()) {
        if (m_impl->onExpire) m_impl->onExpire(m_impl->decals.front().id);
        m_impl->decals.erase(m_impl->decals.begin());
    }
    DecalInstance di;
    di.id   = m_impl->nextId++;
    di.desc = desc;
    di.age  = 0.f;
    di.alpha= (desc.fadeIn<=0.f) ? 1.f : 0.f;
    di.alive= true;
    m_impl->decals.push_back(di);
    return di.id;
}

void DecalSystem::Destroy(uint32_t id) {
    auto& v=m_impl->decals;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& d){return d.id==id;}),v.end());
}
bool DecalSystem::Has(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void DecalSystem::SetPosition(uint32_t id, const float pos[3]) {
    auto* d=m_impl->Find(id); if(d) for(int i=0;i<3;i++) d->desc.position[i]=pos[i];
}
void DecalSystem::SetOrientation(uint32_t id, const float q[4]) {
    auto* d=m_impl->Find(id); if(d) for(int i=0;i<4;i++) d->desc.orientation[i]=q[i];
}
void DecalSystem::SetColour(uint32_t id, const float rgba[4]) {
    auto* d=m_impl->Find(id); if(d) for(int i=0;i<4;i++) d->desc.colour[i]=rgba[i];
}
void DecalSystem::SetLifetime(uint32_t id, float lt) {
    auto* d=m_impl->Find(id); if(d) d->desc.lifetime=lt;
}

const DecalInstance* DecalSystem::Get(uint32_t id) const { return m_impl->Find(id); }
uint32_t             DecalSystem::Count()            const { return (uint32_t)m_impl->decals.size(); }
float                DecalSystem::GetAlpha(uint32_t id) const {
    const auto* d=m_impl->Find(id); return d?d->alpha:0.f;
}

std::vector<uint32_t> DecalSystem::GetVisibleDecals(const float /*frustumPlanes*/[24]) const {
    return GetAll();  // stub: return all live decals
}

std::vector<uint32_t> DecalSystem::GetAll() const {
    std::vector<uint32_t> out;
    for (auto& d : m_impl->decals) if(d.alive) out.push_back(d.id);
    return out;
}

std::vector<uint32_t> DecalSystem::GetByCategory(const std::string& cat) const {
    std::vector<uint32_t> out;
    for (auto& d : m_impl->decals) if(d.alive&&d.desc.category==cat) out.push_back(d.id);
    return out;
}

void DecalSystem::ClearAll() { m_impl->decals.clear(); }
void DecalSystem::ClearCategory(const std::string& cat) {
    auto& v=m_impl->decals;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& d){return d.desc.category==cat;}),v.end());
}

void DecalSystem::SetOnExpire(std::function<void(uint32_t)> cb) { m_impl->onExpire=cb; }
void DecalSystem::SetMaxDecals(uint32_t n) { m_impl->maxDecals=n; }
uint32_t DecalSystem::MaxDecals() const { return m_impl->maxDecals; }

void DecalSystem::Tick(float dt)
{
    std::vector<uint32_t> expired;
    for (auto& d : m_impl->decals) {
        if (!d.alive) continue;
        d.age += dt;
        // Compute alpha
        if (d.desc.fadeIn > 0.f && d.age < d.desc.fadeIn)
            d.alpha = d.age / d.desc.fadeIn;
        else if (d.desc.lifetime > 0.f) {
            float remaining = d.desc.lifetime - d.age;
            if (remaining <= 0.f) {
                d.alive = false;
                expired.push_back(d.id);
                continue;
            }
            d.alpha = (d.desc.fadeOut > 0.f && remaining < d.desc.fadeOut)
                      ? remaining / d.desc.fadeOut : 1.f;
        } else {
            d.alpha = 1.f;
        }
    }
    for (uint32_t eid : expired) {
        if (m_impl->onExpire) m_impl->onExpire(eid);
        Destroy(eid);
    }
}

} // namespace Engine
