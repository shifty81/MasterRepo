#include "Engine/Render/DecalProjector/DecalProjector/DecalProjector.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct AtlasDef { uint32_t cols{1}, rows{1}; };

struct Decal {
    uint32_t id;
    float pos[3]{};
    float norm[3]{0,1,0};
    uint32_t atlasId{0};
    uint32_t tileIndex{0};
    float w{1}, h{1};
    float life{5};
    float age{0};
    float fadeDur{1};
};

struct DecalProjector::Impl {
    std::unordered_map<uint32_t,AtlasDef> atlases;
    std::vector<Decal>                    decals;
    uint32_t maxDecals{512};
    float    fadeDur  {1.f};
    std::function<void(uint32_t)> onExpire;

    Decal* Find(uint32_t id){
        for(auto& d:decals) if(d.id==id) return &d; return nullptr;
    }
};

DecalProjector::DecalProjector(): m_impl(new Impl){}
DecalProjector::~DecalProjector(){ Shutdown(); delete m_impl; }
void DecalProjector::Init(){}
void DecalProjector::Shutdown(){ m_impl->decals.clear(); m_impl->atlases.clear(); }
void DecalProjector::Reset(){ m_impl->decals.clear(); }

void DecalProjector::SetAtlas(uint32_t id, uint32_t c, uint32_t r){
    m_impl->atlases[id]={c,r};
}

bool DecalProjector::SpawnDecal(uint32_t id, float x, float y, float z,
                                  float nx, float ny, float nz,
                                  uint32_t atlasId, float w, float h, float life){
    if(m_impl->Find(id)) return false;
    if(m_impl->decals.size()>=m_impl->maxDecals&&!m_impl->decals.empty())
        m_impl->decals.erase(m_impl->decals.begin()); // drop oldest
    Decal d; d.id=id;
    d.pos[0]=x; d.pos[1]=y; d.pos[2]=z;
    // normalise normal
    float l=std::sqrt(nx*nx+ny*ny+nz*nz);
    if(l>0){nx/=l;ny/=l;nz/=l;}
    d.norm[0]=nx; d.norm[1]=ny; d.norm[2]=nz;
    d.atlasId=atlasId; d.w=w; d.h=h;
    d.life=life; d.fadeDur=m_impl->fadeDur;
    m_impl->decals.push_back(d);
    return true;
}
void DecalProjector::RemoveDecal(uint32_t id){
    auto& v=m_impl->decals;
    v.erase(std::remove_if(v.begin(),v.end(),[id](const Decal& d){return d.id==id;}),v.end());
}
void DecalProjector::RemoveAll(){ m_impl->decals.clear(); }

void DecalProjector::Tick(float dt){
    std::vector<uint32_t> expired;
    for(auto& d:m_impl->decals){
        d.age+=dt;
        if(d.age>=d.life) expired.push_back(d.id);
    }
    for(auto id:expired){
        if(m_impl->onExpire) m_impl->onExpire(id);
        RemoveDecal(id);
    }
}

void DecalProjector::SetFadeDuration(float s){ m_impl->fadeDur=s; }
void DecalProjector::SetMaxDecals(uint32_t n){ m_impl->maxDecals=std::max(1u,n); }

uint32_t DecalProjector::GetDecalCount() const { return (uint32_t)m_impl->decals.size(); }

void DecalProjector::GetDecalPosition(uint32_t id, float& x, float& y, float& z) const {
    auto* d=m_impl->Find(id); if(!d){x=y=z=0;return;}
    x=d->pos[0]; y=d->pos[1]; z=d->pos[2];
}
void DecalProjector::GetDecalNormal(uint32_t id, float& nx, float& ny, float& nz) const {
    auto* d=m_impl->Find(id); if(!d){nx=0;ny=1;nz=0;return;}
    nx=d->norm[0]; ny=d->norm[1]; nz=d->norm[2];
}
float DecalProjector::GetDecalAlpha(uint32_t id) const {
    auto* d=m_impl->Find(id); if(!d) return 0;
    float remain=d->life-d->age;
    if(remain<=0) return 0;
    if(remain<d->fadeDur) return remain/d->fadeDur;
    return 1.f;
}
void DecalProjector::SetDecalTile(uint32_t id, uint32_t tile){
    auto* d=m_impl->Find(id); if(d) d->tileIndex=tile;
}
void DecalProjector::SetOnExpire(std::function<void(uint32_t)> cb){ m_impl->onExpire=cb; }

} // namespace Engine
