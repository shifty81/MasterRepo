#include "Engine/Physics/SoftBody/SoftBodySystem/SoftBodySystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

static float Len3(const float v[3]){ return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])+1e-9f; }
static float Dot3(const float a[3],const float b[3]){ return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }

struct SBVertex { float pos[3]{}, prev[3]{}, vel[3]{}; float invMass{1.f}; bool pinned{false}; float pinnedPos[3]{}; };
struct SBSpring  { uint32_t a, b; float restLen; };

struct SoftBodyData {
    uint32_t id{0};
    SoftBodyDesc desc;
    std::vector<SBVertex>  verts;
    std::vector<SBSpring>  springs;
    std::vector<SBSphereCollider> spheres;
    std::vector<SBPlaneCollider>  planes;
    float restVolume{1.f};
};

struct SoftBodySystem::Impl {
    std::vector<SoftBodyData*> bodies;
    uint32_t nextId{1};
    std::function<void(uint32_t,uint32_t)> onCollision;

    SoftBodyData* Find(uint32_t id){ for(auto* b:bodies) if(b->id==id) return b; return nullptr; }
    const SoftBodyData* Find(uint32_t id) const { for(auto* b:bodies) if(b->id==id) return b; return nullptr; }
};

SoftBodySystem::SoftBodySystem()  : m_impl(new Impl){}
SoftBodySystem::~SoftBodySystem() { Shutdown(); delete m_impl; }
void SoftBodySystem::Init()     {}
void SoftBodySystem::Shutdown() { for(auto* b:m_impl->bodies) delete b; m_impl->bodies.clear(); }

uint32_t SoftBodySystem::Create(const SoftBodyDesc& d, const float origin[3]){
    SoftBodyData* sb=new SoftBodyData; sb->id=m_impl->nextId++; sb->desc=d;
    sb->verts.resize(d.vertexCount);
    if(origin) for(auto& v:sb->verts) for(int i=0;i<3;i++) v.pos[i]=origin[i];
    for(auto& v:sb->verts){ for(int i=0;i<3;i++) v.prev[i]=v.pos[i]; v.invMass=1.f/d.mass; }
    m_impl->bodies.push_back(sb); return sb->id;
}
void SoftBodySystem::Destroy(uint32_t id){
    auto& v=m_impl->bodies;
    for(auto it=v.begin();it!=v.end();++it) if((*it)->id==id){delete *it;v.erase(it);return;}
}
bool SoftBodySystem::Has(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void SoftBodySystem::SetRestPositions(uint32_t id, const float* posArr, uint32_t n){
    auto* sb=m_impl->Find(id); if(!sb) return;
    if(n>sb->verts.size()) n=(uint32_t)sb->verts.size();
    for(uint32_t i=0;i<n;i++){
        for(int k=0;k<3;k++){sb->verts[i].pos[k]=posArr[i*3+k]; sb->verts[i].prev[k]=posArr[i*3+k];}
    }
}

void SoftBodySystem::AddSpring(uint32_t id, uint32_t a, uint32_t b, float rl){
    auto* sb=m_impl->Find(id); if(!sb) return;
    if(rl<0.f){float d[3]={sb->verts[a].pos[0]-sb->verts[b].pos[0],sb->verts[a].pos[1]-sb->verts[b].pos[1],sb->verts[a].pos[2]-sb->verts[b].pos[2]}; rl=Len3(d);}
    sb->springs.push_back({a,b,rl});
}
void SoftBodySystem::AutoSprings(uint32_t id){
    auto* sb=m_impl->Find(id); if(!sb||sb->verts.empty()) return;
    sb->springs.clear();
    uint32_t n=(uint32_t)sb->verts.size();
    // Connect neighbours within radius
    float radius=0.3f;
    for(uint32_t a=0;a<n;a++) for(uint32_t b=a+1;b<n;b++){
        float d[3]={sb->verts[a].pos[0]-sb->verts[b].pos[0],sb->verts[a].pos[1]-sb->verts[b].pos[1],sb->verts[a].pos[2]-sb->verts[b].pos[2]};
        float l=Len3(d);
        if(l<radius) sb->springs.push_back({a,b,l});
    }
}

void SoftBodySystem::Pin(uint32_t id, uint32_t vi, const float wp[3]){
    auto* sb=m_impl->Find(id); if(sb&&vi<sb->verts.size()){
        auto& v=sb->verts[vi]; v.pinned=true; for(int i=0;i<3;i++){v.pinnedPos[i]=wp[i];v.pos[i]=wp[i];v.prev[i]=wp[i];}
    }
}
void SoftBodySystem::Unpin(uint32_t id, uint32_t vi){
    auto* sb=m_impl->Find(id); if(sb&&vi<sb->verts.size()) sb->verts[vi].pinned=false;
}
void SoftBodySystem::AddImpulse(uint32_t id, uint32_t vi, const float imp[3]){
    auto* sb=m_impl->Find(id); if(sb&&vi<sb->verts.size()&&!sb->verts[vi].pinned)
        for(int i=0;i<3;i++) sb->verts[vi].vel[i]+=imp[i]*sb->verts[vi].invMass;
}

void SoftBodySystem::AddSphere(uint32_t id, const SBSphereCollider& s){ auto* sb=m_impl->Find(id); if(sb) sb->spheres.push_back(s); }
void SoftBodySystem::AddPlane (uint32_t id, const SBPlaneCollider&  p){ auto* sb=m_impl->Find(id); if(sb) sb->planes.push_back(p); }
void SoftBodySystem::ClearColliders(uint32_t id){ auto* sb=m_impl->Find(id); if(sb){sb->spheres.clear();sb->planes.clear();} }

const float* SoftBodySystem::GetVertexPos(uint32_t id, uint32_t vi) const {
    const auto* sb=m_impl->Find(id); return (sb&&vi<sb->verts.size())?sb->verts[vi].pos:nullptr;
}
uint32_t SoftBodySystem::VertexCount(uint32_t id) const {
    const auto* sb=m_impl->Find(id); return sb?(uint32_t)sb->verts.size():0;
}
void SoftBodySystem::SetOnCollision(std::function<void(uint32_t,uint32_t)> cb){ m_impl->onCollision=cb; }
std::vector<uint32_t> SoftBodySystem::GetAll() const {
    std::vector<uint32_t> out; for(auto* sb:m_impl->bodies) out.push_back(sb->id); return out;
}

void SoftBodySystem::Tick(float dt)
{
    for(auto* sb:m_impl->bodies){
        // Verlet
        for(auto& v:sb->verts){
            if(v.pinned){for(int i=0;i<3;i++) v.pos[i]=v.pinnedPos[i]; continue;}
            float acc[3]; for(int i=0;i<3;i++) acc[i]=sb->desc.gravity[i]+v.vel[i];
            float np[3]; for(int i=0;i<3;i++) np[i]=v.pos[i]+(1.f-sb->desc.damping)*(v.pos[i]-v.prev[i])+acc[i]*dt*dt;
            for(int i=0;i<3;i++){v.prev[i]=v.pos[i]; v.pos[i]=np[i];}
        }
        // Springs
        for(int iter=0;iter<5;iter++){
            for(auto& sp:sb->springs){
                auto& va=sb->verts[sp.a]; auto& vb=sb->verts[sp.b];
                float d[3]={vb.pos[0]-va.pos[0],vb.pos[1]-va.pos[1],vb.pos[2]-va.pos[2]};
                float l=Len3(d); float diff=(l-sp.restLen)/l*sb->desc.stiffness;
                float wSum=va.invMass+vb.invMass+1e-9f;
                for(int i=0;i<3;i++){
                    if(!va.pinned) va.pos[i]+=va.invMass/wSum*diff*d[i];
                    if(!vb.pinned) vb.pos[i]-=vb.invMass/wSum*diff*d[i];
                }
            }
        }
        // Pressure (simple outward force from centroid)
        if(sb->desc.pressure>0.f&&!sb->verts.empty()){
            float c[3]={}; for(auto& v:sb->verts) for(int i=0;i<3;i++) c[i]+=v.pos[i];
            float n=(float)sb->verts.size(); for(int i=0;i<3;i++) c[i]/=n;
            for(auto& v:sb->verts){ if(v.pinned) continue;
                float d[3]={v.pos[0]-c[0],v.pos[1]-c[1],v.pos[2]-c[2]};
                float l=Len3(d); for(int i=0;i<3;i++) v.pos[i]+=d[i]/l*sb->desc.pressure*dt;
            }
        }
        // Collisions
        for(auto& v:sb->verts){ if(v.pinned) continue;
            for(auto& s:sb->spheres){
                float d[3]={v.pos[0]-s.centre[0],v.pos[1]-s.centre[1],v.pos[2]-s.centre[2]};
                float l=Len3(d); if(l<s.radius){ float f=(s.radius-l)/l; for(int i=0;i<3;i++) v.pos[i]+=d[i]*f; if(m_impl->onCollision) m_impl->onCollision(sb->id,0); }
            }
            for(auto& p:sb->planes){
                float dist=Dot3(v.pos,p.normal)-p.d;
                if(dist<0) for(int i=0;i<3;i++) v.pos[i]-=p.normal[i]*dist;
            }
        }
    }
}

} // namespace Engine
