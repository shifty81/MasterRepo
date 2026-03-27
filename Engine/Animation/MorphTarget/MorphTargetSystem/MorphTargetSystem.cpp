#include "Engine/Animation/MorphTarget/MorphTargetSystem/MorphTargetSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct AnimKey { float time; float weight; };

struct MorphTarget {
    uint32_t            id;
    std::string         name;
    std::vector<MTVec3> deltaPos;
    std::vector<MTVec3> deltaNorm;
    float               weight{0};
    std::vector<AnimKey> curve;
    float               animTime{0};
    bool                loop{true};
};

struct MorphMesh {
    uint32_t                id;
    uint32_t                vertexCount{0};
    std::vector<MTVec3>     basePos;
    std::vector<MTVec3>     baseNorm;
    std::vector<MorphTarget> targets;
    uint32_t                nextTargetId{1};
};

struct MorphTargetSystem::Impl {
    std::vector<MorphMesh> meshes;

    MorphMesh* FindMesh(uint32_t id){
        for(auto& m:meshes) if(m.id==id) return &m; return nullptr;
    }
    const MorphMesh* FindMesh(uint32_t id) const {
        for(auto& m:meshes) if(m.id==id) return &m; return nullptr;
    }
    MorphTarget* FindTarget(MorphMesh* m, uint32_t tid){
        for(auto& t:m->targets) if(t.id==tid) return &t; return nullptr;
    }
};

MorphTargetSystem::MorphTargetSystem(): m_impl(new Impl){}
MorphTargetSystem::~MorphTargetSystem(){ Shutdown(); delete m_impl; }
void MorphTargetSystem::Init(){}
void MorphTargetSystem::Shutdown(){ m_impl->meshes.clear(); }
void MorphTargetSystem::Reset(){ m_impl->meshes.clear(); }

void MorphTargetSystem::CreateMesh(uint32_t id, const MTVec3* bp, const MTVec3* bn, uint32_t n){
    // Replace if exists
    for(auto& m:m_impl->meshes) if(m.id==id){ m.vertexCount=n;
        m.basePos.assign(bp,bp+n); m.baseNorm.assign(bn,bn+n); return; }
    MorphMesh mm; mm.id=id; mm.vertexCount=n;
    mm.basePos.assign(bp,bp+n); mm.baseNorm.assign(bn,bn+n);
    m_impl->meshes.push_back(mm);
}
void MorphTargetSystem::DestroyMesh(uint32_t id){
    m_impl->meshes.erase(std::remove_if(m_impl->meshes.begin(),m_impl->meshes.end(),
        [id](const MorphMesh& m){return m.id==id;}),m_impl->meshes.end());
}

uint32_t MorphTargetSystem::AddTarget(uint32_t meshId, const std::string& name,
                                       const MTVec3* dp, const MTVec3* dn){
    auto* mm=m_impl->FindMesh(meshId); if(!mm) return 0;
    MorphTarget t; t.id=mm->nextTargetId++; t.name=name;
    t.deltaPos.assign(dp,dp+mm->vertexCount);
    if(dn) t.deltaNorm.assign(dn,dn+mm->vertexCount);
    else   t.deltaNorm.assign(mm->vertexCount,{0,0,0});
    mm->targets.push_back(t); return t.id;
}
void MorphTargetSystem::RemoveTarget(uint32_t meshId, uint32_t tid){
    auto* mm=m_impl->FindMesh(meshId); if(!mm) return;
    mm->targets.erase(std::remove_if(mm->targets.begin(),mm->targets.end(),
        [tid](const MorphTarget& t){return t.id==tid;}),mm->targets.end());
}
uint32_t MorphTargetSystem::GetTargetId(uint32_t meshId, const std::string& name) const {
    auto* mm=m_impl->FindMesh(meshId); if(!mm) return 0;
    for(auto& t:mm->targets) if(t.name==name) return t.id;
    return 0;
}

void  MorphTargetSystem::SetWeight(uint32_t meshId, uint32_t tid, float w){
    auto* mm=m_impl->FindMesh(meshId); if(!mm) return;
    auto* t=m_impl->FindTarget(mm,tid); if(t) t->weight=w;
}
float MorphTargetSystem::GetWeight(uint32_t meshId, uint32_t tid) const {
    auto* mm=m_impl->FindMesh(meshId); if(!mm) return 0;
    auto* t=const_cast<Impl*>(m_impl)->FindTarget(const_cast<MorphMesh*>(mm),tid);
    return t?t->weight:0;
}
void MorphTargetSystem::ResetWeights(uint32_t meshId){
    auto* mm=m_impl->FindMesh(meshId); if(!mm) return;
    for(auto& t:mm->targets) t.weight=0;
}

void MorphTargetSystem::Evaluate(uint32_t meshId, MTVec3* outPos, MTVec3* outNorm) const {
    auto* mm=m_impl->FindMesh(meshId); if(!mm) return;
    uint32_t n=mm->vertexCount;
    for(uint32_t i=0;i<n;i++){ outPos[i]=mm->basePos[i]; outNorm[i]=mm->baseNorm[i]; }
    for(auto& t:mm->targets){
        if(t.weight==0) continue;
        for(uint32_t i=0;i<n;i++){
            outPos [i].x+=t.deltaPos [i].x*t.weight;
            outPos [i].y+=t.deltaPos [i].y*t.weight;
            outPos [i].z+=t.deltaPos [i].z*t.weight;
            outNorm[i].x+=t.deltaNorm[i].x*t.weight;
            outNorm[i].y+=t.deltaNorm[i].y*t.weight;
            outNorm[i].z+=t.deltaNorm[i].z*t.weight;
        }
    }
}

void MorphTargetSystem::SetAnimCurve(uint32_t meshId, uint32_t tid,
                                      const float* times, const float* weights, uint32_t count){
    auto* mm=m_impl->FindMesh(meshId); if(!mm) return;
    auto* t=m_impl->FindTarget(mm,tid); if(!t) return;
    t->curve.resize(count);
    for(uint32_t i=0;i<count;i++){ t->curve[i].time=times[i]; t->curve[i].weight=weights[i]; }
}
void MorphTargetSystem::SetAnimLooping(uint32_t meshId, uint32_t tid, bool loop){
    auto* mm=m_impl->FindMesh(meshId); if(!mm) return;
    auto* t=m_impl->FindTarget(mm,tid); if(t) t->loop=loop;
}
void MorphTargetSystem::TickAnimation(uint32_t meshId, float dt){
    auto* mm=m_impl->FindMesh(meshId); if(!mm) return;
    for(auto& t:mm->targets){
        if(t.curve.empty()) continue;
        float dur=t.curve.back().time;
        if(dur<=0) continue;
        t.animTime+=dt;
        if(t.loop) t.animTime=std::fmod(t.animTime,dur);
        else t.animTime=std::min(t.animTime,dur);
        // Evaluate
        for(size_t i=0;i+1<t.curve.size();i++){
            if(t.animTime>=t.curve[i].time&&t.animTime<=t.curve[i+1].time){
                float seg=t.curve[i+1].time-t.curve[i].time;
                float u=(seg>0)?(t.animTime-t.curve[i].time)/seg:0;
                t.weight=t.curve[i].weight+(t.curve[i+1].weight-t.curve[i].weight)*u;
                break;
            }
        }
    }
}

uint32_t MorphTargetSystem::GetTargetCount(uint32_t meshId) const {
    auto* mm=m_impl->FindMesh(meshId); return mm?(uint32_t)mm->targets.size():0;
}
uint32_t MorphTargetSystem::GetVertexCount(uint32_t meshId) const {
    auto* mm=m_impl->FindMesh(meshId); return mm?mm->vertexCount:0;
}

} // namespace Engine
