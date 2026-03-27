#include "Engine/Physics/Magnet/MagnetSystem/MagnetSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

static float Len3(const float v[3]){ return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])+1e-9f; }

struct MagData { uint32_t id; MagnetDesc desc; };
struct BodyData { uint32_t id; float mass; float pos[3]{}; float force[3]{}; std::unordered_map<uint32_t,float> scales; };

struct MagnetSystem::Impl {
    std::vector<MagData>  magnets;
    std::vector<BodyData> bodies;
    uint32_t nextMagId{1};

    MagData*  FindMag (uint32_t id){ for(auto& m:magnets) if(m.id==id) return &m; return nullptr; }
    BodyData* FindBody(uint32_t id){ for(auto& b:bodies)  if(b.id==id) return &b; return nullptr; }
    const MagData*  FindMag (uint32_t id) const { for(auto& m:magnets) if(m.id==id) return &m; return nullptr; }
    const BodyData* FindBody(uint32_t id) const { for(auto& b:bodies)  if(b.id==id) return &b; return nullptr; }
};

MagnetSystem::MagnetSystem() : m_impl(new Impl){}
MagnetSystem::~MagnetSystem(){ Shutdown(); delete m_impl; }
void MagnetSystem::Init()     {}
void MagnetSystem::Shutdown() { m_impl->magnets.clear(); m_impl->bodies.clear(); }

uint32_t MagnetSystem::Create(const MagnetDesc& d){
    m_impl->magnets.push_back({m_impl->nextMagId++, d});
    return m_impl->magnets.back().id;
}
void MagnetSystem::Destroy(uint32_t id){
    auto& v=m_impl->magnets;
    v.erase(std::remove_if(v.begin(),v.end(),[id](const MagData& m){return m.id==id;}),v.end());
}
bool MagnetSystem::Has(uint32_t id) const { return m_impl->FindMag(id)!=nullptr; }
void MagnetSystem::SetStrength(uint32_t id, float s){ auto* m=m_impl->FindMag(id); if(m) m->desc.strength=s; }
void MagnetSystem::SetEnabled (uint32_t id, bool e) { auto* m=m_impl->FindMag(id); if(m) m->desc.enabled=e; }
void MagnetSystem::MoveMAGNET(uint32_t id, const float pos[3]){ auto* m=m_impl->FindMag(id); if(m) for(int i=0;i<3;i++) m->desc.pos[i]=pos[i]; }
void MagnetSystem::GetMagnetPos(uint32_t id, float out[3]) const { auto* m=m_impl->FindMag(id); if(m) for(int i=0;i<3;i++) out[i]=m->desc.pos[i]; }

void MagnetSystem::RegisterBody(uint32_t id, float mass, const float pos[3]){
    BodyData b; b.id=id; b.mass=mass; for(int i=0;i<3;i++) b.pos[i]=pos[i]; m_impl->bodies.push_back(b);
}
void MagnetSystem::UnregisterBody(uint32_t id){
    auto& v=m_impl->bodies; v.erase(std::remove_if(v.begin(),v.end(),[id](const BodyData& b){return b.id==id;}),v.end());
}
void MagnetSystem::UpdateBodyPos(uint32_t id, const float pos[3]){ auto* b=m_impl->FindBody(id); if(b) for(int i=0;i<3;i++) b->pos[i]=pos[i]; }
const float* MagnetSystem::GetForce(uint32_t id) const { const auto* b=m_impl->FindBody(id); return b?b->force:nullptr; }
void MagnetSystem::SetBodyMagnetScale(uint32_t bodyId, uint32_t magId, float s){ auto* b=m_impl->FindBody(bodyId); if(b) b->scales[magId]=s; }
bool MagnetSystem::IsBodyAffected(uint32_t bodyId, uint32_t magId) const {
    const auto* b=m_impl->FindBody(bodyId); const auto* m=m_impl->FindMag(magId); if(!b||!m) return false;
    float d[3]={b->pos[0]-m->desc.pos[0],b->pos[1]-m->desc.pos[1],b->pos[2]-m->desc.pos[2]};
    return Len3(d)<=m->desc.radius;
}

std::vector<uint32_t> MagnetSystem::GetAllMagnets() const { std::vector<uint32_t> v; for(auto& m:m_impl->magnets) v.push_back(m.id); return v; }
std::vector<uint32_t> MagnetSystem::GetAllBodies()  const { std::vector<uint32_t> v; for(auto& b:m_impl->bodies)  v.push_back(b.id); return v; }

void MagnetSystem::Tick(float dt){
    for(auto& bd:m_impl->bodies){
        for(int i=0;i<3;i++) bd.force[i]=0;
        for(auto& mg:m_impl->magnets){
            if(!mg.desc.enabled) continue;
            float d[3]={bd.pos[0]-mg.desc.pos[0],bd.pos[1]-mg.desc.pos[1],bd.pos[2]-mg.desc.pos[2]};
            float dist=Len3(d);
            if(dist>mg.desc.radius) continue;
            float scale=1.f;
            auto it=bd.scales.find(mg.id); if(it!=bd.scales.end()) scale=it->second;
            float mag=mg.desc.strength*scale/std::pow(dist,mg.desc.falloffExp);
            // attract: force toward magnet (-d), repel: force away (+d)
            float dir= mg.desc.strength>0?-1.f:1.f;
            for(int i=0;i<3;i++) bd.force[i]+=dir*mag*d[i]/dist*dt;
        }
    }
}

} // namespace Engine
