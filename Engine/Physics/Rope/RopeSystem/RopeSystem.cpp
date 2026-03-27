#include "Engine/Physics/Rope/RopeSystem/RopeSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

static float Len3(const float v[3]){ return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])+1e-9f; }

struct RopeSeg { float pos[3]{},prev[3]{},vel[3]{}; bool pinned{false}; float pinnedPos[3]{}; };

struct RopeData {
    uint32_t id; RopeDesc desc;
    std::vector<RopeSeg> segs;
    float wind[3]{};
    float turbAmp{0.f}, turbFreq{1.f}, turbPhase{0.f};
    std::vector<RopeSphere> spheres;
    std::vector<std::pair<float,float>> planes; // normalY, d
    std::vector<float> tangents; // segs.size()*3
};

struct RopeSystem::Impl {
    std::vector<RopeData*> ropes;
    uint32_t nextId{1};
    std::function<void(uint32_t)> onBreak;

    RopeData* Find(uint32_t id){ for(auto* r:ropes) if(r->id==id) return r; return nullptr; }
    const RopeData* Find(uint32_t id) const { for(auto* r:ropes) if(r->id==id) return r; return nullptr; }
};

RopeSystem::RopeSystem()  : m_impl(new Impl){}
RopeSystem::~RopeSystem() { Shutdown(); delete m_impl; }
void RopeSystem::Init()     {}
void RopeSystem::Shutdown() { for(auto* r:m_impl->ropes) delete r; m_impl->ropes.clear(); }

uint32_t RopeSystem::Create(const RopeDesc& d, const float start[3]){
    RopeData* rd=new RopeData; rd->id=m_impl->nextId++; rd->desc=d;
    float ox=start?start[0]:0.f, oy=start?start[1]:0.f, oz=start?start[2]:0.f;
    for(uint32_t i=0;i<d.segmentCount;i++){
        RopeSeg s; s.pos[0]=ox; s.pos[1]=oy-i*d.segmentLength; s.pos[2]=oz;
        for(int k=0;k<3;k++) s.prev[k]=s.pos[k];
        rd->segs.push_back(s);
    }
    rd->tangents.resize(d.segmentCount*3,0.f);
    m_impl->ropes.push_back(rd); return rd->id;
}
void RopeSystem::Destroy(uint32_t id){
    auto& v=m_impl->ropes;
    for(auto it=v.begin();it!=v.end();++it) if((*it)->id==id){delete *it;v.erase(it);return;}
}
bool RopeSystem::Has(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void RopeSystem::Pin(uint32_t id, uint32_t i, const float wp[3]){
    auto* r=m_impl->Find(id); if(r&&i<r->segs.size()){
        auto& s=r->segs[i]; s.pinned=true; for(int k=0;k<3;k++){s.pinnedPos[k]=wp[k];s.pos[k]=wp[k];s.prev[k]=wp[k];}
    }
}
void RopeSystem::Unpin(uint32_t id, uint32_t i){
    auto* r=m_impl->Find(id); if(r&&i<r->segs.size()) r->segs[i].pinned=false;
}
void RopeSystem::SetWind(uint32_t id, const float d[3], float amp, float freq){
    auto* r=m_impl->Find(id); if(r){for(int k=0;k<3;k++) r->wind[k]=d[k]; r->turbAmp=amp; r->turbFreq=freq;}
}
void RopeSystem::AddImpulse(uint32_t id, uint32_t i, const float imp[3]){
    auto* r=m_impl->Find(id); if(r&&i<r->segs.size()&&!r->segs[i].pinned)
        for(int k=0;k<3;k++) r->segs[i].vel[k]+=imp[k];
}
void RopeSystem::AddSphere(uint32_t id, const RopeSphere& s){ auto* r=m_impl->Find(id); if(r) r->spheres.push_back(s); }
void RopeSystem::AddPlane(uint32_t id, float ny, float d){ auto* r=m_impl->Find(id); if(r) r->planes.push_back({ny,d}); }
void RopeSystem::ClearColliders(uint32_t id){ auto* r=m_impl->Find(id); if(r){r->spheres.clear();r->planes.clear();} }

const float* RopeSystem::GetSegmentPos(uint32_t id, uint32_t i) const {
    const auto* r=m_impl->Find(id); return (r&&i<r->segs.size())?r->segs[i].pos:nullptr;
}
const float* RopeSystem::GetTangent(uint32_t id, uint32_t i) const {
    const auto* r=m_impl->Find(id); return (r&&i<r->segs.size())?&r->tangents[i*3]:nullptr;
}
uint32_t RopeSystem::SegmentCount(uint32_t id) const {
    const auto* r=m_impl->Find(id); return r?(uint32_t)r->segs.size():0;
}
float RopeSystem::GetTension(uint32_t id, uint32_t i) const {
    const auto* r=m_impl->Find(id); if(!r||i+1>=(uint32_t)r->segs.size()) return 0;
    float d[3]={r->segs[i].pos[0]-r->segs[i+1].pos[0],r->segs[i].pos[1]-r->segs[i+1].pos[1],r->segs[i].pos[2]-r->segs[i+1].pos[2]};
    return std::abs(Len3(d)-r->desc.segmentLength);
}
void RopeSystem::SetOnBreak(std::function<void(uint32_t)> cb){ m_impl->onBreak=cb; }
std::vector<uint32_t> RopeSystem::GetAll() const {
    std::vector<uint32_t> out; for(auto* r:m_impl->ropes) out.push_back(r->id); return out;
}

void RopeSystem::Tick(float dt)
{
    for(auto* rd:m_impl->ropes){
        rd->turbPhase+=dt*rd->turbFreq;
        float turbForce=rd->turbAmp*std::sin(rd->turbPhase);
        // Verlet
        for(auto& s:rd->segs){
            if(s.pinned){for(int k=0;k<3;k++) s.pos[k]=s.pinnedPos[k]; continue;}
            float acc[3]; for(int k=0;k<3;k++) acc[k]=rd->desc.gravity[k]+rd->wind[k]+turbForce;
            float np[3]; for(int k=0;k<3;k++) np[k]=s.pos[k]+(1.f-rd->desc.damping)*(s.pos[k]-s.prev[k])+acc[k]*dt*dt;
            for(int k=0;k<3;k++){s.prev[k]=s.pos[k]; s.pos[k]=np[k];}
        }
        // Distance constraints
        float restLen=rd->desc.segmentLength;
        for(uint32_t iter=0;iter<rd->desc.solverIter;iter++){
            for(uint32_t i=0;i+1<(uint32_t)rd->segs.size();i++){
                auto& a=rd->segs[i]; auto& b=rd->segs[i+1];
                float d[3]={b.pos[0]-a.pos[0],b.pos[1]-a.pos[1],b.pos[2]-a.pos[2]};
                float l=Len3(d), diff=(l-restLen)/l*rd->desc.stiffness;
                if(!a.pinned) for(int k=0;k<3;k++) a.pos[k]+=diff*d[k]*0.5f;
                if(!b.pinned) for(int k=0;k<3;k++) b.pos[k]-=diff*d[k]*0.5f;
            }
        }
        // Collision
        for(auto& s:rd->segs){if(s.pinned) continue;
            for(auto& sp:rd->spheres){
                float d[3]={s.pos[0]-sp.centre[0],s.pos[1]-sp.centre[1],s.pos[2]-sp.centre[2]};
                float l=Len3(d); if(l<sp.radius) for(int k=0;k<3;k++) s.pos[k]=sp.centre[k]+d[k]/l*sp.radius;
            }
            for(auto& p:rd->planes){
                float dist=s.pos[1]*p.first-p.second; if(dist<0) s.pos[1]+=std::abs(dist)/p.first;
            }
        }
        // Tangents
        for(uint32_t i=0;i<(uint32_t)rd->segs.size();i++){
            uint32_t ni=std::min(i+1,(uint32_t)rd->segs.size()-1);
            float d[3]={rd->segs[ni].pos[0]-rd->segs[i].pos[0],rd->segs[ni].pos[1]-rd->segs[i].pos[1],rd->segs[ni].pos[2]-rd->segs[i].pos[2]};
            float l=Len3(d); for(int k=0;k<3;k++) rd->tangents[i*3+k]=d[k]/l;
        }
        // Break check
        if(rd->desc.breakThreshold>0.f&&m_impl->onBreak){
            for(uint32_t i=0;i+1<(uint32_t)rd->segs.size();i++)
                if(GetTension(rd->id,i)>rd->desc.breakThreshold){ m_impl->onBreak(rd->id); break; }
        }
    }
}

} // namespace Engine
