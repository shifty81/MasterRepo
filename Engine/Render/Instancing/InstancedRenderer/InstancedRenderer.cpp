#include "Engine/Render/Instancing/InstancedRenderer/InstancedRenderer.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace Engine {

static inline float Dot4(const float* p, float x,float y,float z){
    return p[0]*x+p[1]*y+p[2]*z+p[3];
}

struct InstEntry {
    uint32_t  handle;
    uint32_t  meshId;
    InstanceData data;
    bool      active{true};
};

struct MeshInfo {
    std::vector<float> lodDist; // distance thresholds, sorted asc
};

struct InstancedRenderer::Impl {
    uint32_t maxInst{65536};
    uint32_t nextHandle{1};
    mutable uint32_t visCount{0};

    std::unordered_map<uint32_t,MeshInfo>  meshes;
    std::unordered_map<uint32_t,InstEntry> entries; // handle → entry

    float ExtractTranslation(const InstMat4& m, float& ox,float& oy,float& oz){
        ox=m.m[12]; oy=m.m[13]; oz=m.m[14]; return 0;
    }
    bool FrustumTest(float x,float y,float z, const float* planes) const {
        for(int i=0;i<6;i++) if(Dot4(planes+i*4,x,y,z)<0) return false;
        return true;
    }
    uint32_t SelectLOD(uint32_t meshId, float dist) const {
        auto it=meshes.find(meshId);
        if(it==meshes.end()) return 0;
        uint32_t lod=0;
        for(float d : it->second.lodDist){ if(dist>d) lod++; else break; }
        return lod;
    }
};

InstancedRenderer::InstancedRenderer(): m_impl(new Impl){}
InstancedRenderer::~InstancedRenderer(){ Shutdown(); delete m_impl; }
void InstancedRenderer::Init(uint32_t max){ m_impl->maxInst=max; }
void InstancedRenderer::Shutdown(){ m_impl->entries.clear(); m_impl->meshes.clear(); }

void InstancedRenderer::RegisterMesh(uint32_t id, std::vector<float> lod){
    std::sort(lod.begin(),lod.end());
    m_impl->meshes[id]={lod};
}
void InstancedRenderer::UnregisterMesh(uint32_t id){ m_impl->meshes.erase(id); }

uint32_t InstancedRenderer::AddInstance(uint32_t meshId, const InstMat4& xf, InstColor col, const float* cust){
    if(m_impl->entries.size()>=m_impl->maxInst) return 0;
    uint32_t h=m_impl->nextHandle++;
    InstEntry e; e.handle=h; e.meshId=meshId; e.data.transform=xf; e.data.colour=col;
    if(cust) for(int i=0;i<4;i++) e.data.custom[i]=cust[i];
    m_impl->entries[h]=e;
    return h;
}
void InstancedRenderer::RemoveInstance(uint32_t h){ m_impl->entries.erase(h); }
void InstancedRenderer::UpdateTransform(uint32_t h, const InstMat4& xf){
    auto it=m_impl->entries.find(h); if(it!=m_impl->entries.end()) it->second.data.transform=xf;
}
void InstancedRenderer::UpdateColour(uint32_t h, InstColor c){
    auto it=m_impl->entries.find(h); if(it!=m_impl->entries.end()) it->second.data.colour=c;
}

std::vector<DrawBatch> InstancedRenderer::CullAndBatch(float cx,float cy,float cz, const float planes[24]) const {
    std::unordered_map<uint64_t, DrawBatch> batchMap;
    m_impl->visCount=0;
    for(auto& [h,e] : m_impl->entries){
        float ox,oy,oz; m_impl->ExtractTranslation(e.data.transform,ox,oy,oz);
        if(!m_impl->FrustumTest(ox,oy,oz,planes)) continue;
        float dx=ox-cx,dy=oy-cy,dz=oz-cz;
        float dist=std::sqrt(dx*dx+dy*dy+dz*dz);
        uint32_t lod=m_impl->SelectLOD(e.meshId,dist);
        uint64_t key=((uint64_t)e.meshId<<32)|(uint64_t)lod;
        auto& batch=batchMap[key];
        batch.meshId=e.meshId; batch.lodLevel=lod;
        batch.instances.push_back(e.data);
        m_impl->visCount++;
    }
    std::vector<DrawBatch> out;
    out.reserve(batchMap.size());
    for(auto& [k,b] : batchMap) out.push_back(std::move(b));
    return out;
}

uint32_t InstancedRenderer::GetInstanceCount(uint32_t meshId) const {
    uint32_t c=0; for(auto& [h,e]:m_impl->entries) if(e.meshId==meshId) c++; return c;
}
uint32_t InstancedRenderer::TotalInstances  () const { return (uint32_t)m_impl->entries.size(); }
uint32_t InstancedRenderer::VisibleInstances() const { return m_impl->visCount; }
uint32_t InstancedRenderer::DrawCallCount   () const {
    // Each unique (meshId,lod) pair = 1 draw call; recompute cheaply
    // (or just return visCount as upper bound stub)
    return m_impl->visCount;
}
void InstancedRenderer::Clear(uint32_t meshId){
    for(auto it=m_impl->entries.begin();it!=m_impl->entries.end();){
        if(it->second.meshId==meshId) it=m_impl->entries.erase(it); else ++it;
    }
}
void InstancedRenderer::ClearAll(){ m_impl->entries.clear(); }

} // namespace Engine
