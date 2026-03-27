#include "Engine/AI/Formation/FormationSystem/FormationSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

static constexpr float kPi = 3.14159265f;

struct FormationData {
    uint32_t         id;
    FormationShape   shape;
    uint32_t         slotCount;
    float            spacing;
    FVec3            lastLeaderPos{0,0,0};
    float            lastLeaderYaw{0};
    std::vector<uint32_t> slotToAgent; // 0 = empty
    std::vector<FVec3>    slotPositions;
};

struct FormationSystem::Impl {
    uint32_t nextFormId{1};
    std::vector<FormationData> formations;
    std::unordered_map<uint32_t,uint32_t> agentToForm; // agentId → formId
    std::unordered_map<uint32_t,uint32_t> agentToSlot; // agentId → slotIndex

    FormationData* Find(uint32_t id){
        for(auto& f:formations) if(f.id==id) return &f;
        return nullptr;
    }

    FVec3 ComputeSlot(FormationShape shape, uint32_t slotIdx, uint32_t total,
                      FVec3 leader, float yaw, float spacing) const {
        float cy=std::cos(yaw), sy=std::sin(yaw);
        float lx=0,lz=0;
        switch(shape){
            case FormationShape::Line:{
                float offset=(float)slotIdx-(total-1)*0.5f;
                lx=offset*spacing; lz=0; break;
            }
            case FormationShape::Column:{
                lx=0; lz=(float)(slotIdx+1)*spacing; break;
            }
            case FormationShape::Wedge:{
                uint32_t row=slotIdx/2, col=slotIdx%2;
                lx=(col==0?-1.f:1.f)*(row+1)*spacing*0.7f;
                lz=(float)(row+1)*spacing; break;
            }
            case FormationShape::Echelon:{
                lx=(float)slotIdx*spacing*0.7f;
                lz=(float)slotIdx*spacing; break;
            }
            case FormationShape::Circle:{
                float ang=2*kPi*slotIdx/std::max(1u,total);
                lx=std::cos(ang)*spacing; lz=std::sin(ang)*spacing; break;
            }
            case FormationShape::Box:{
                uint32_t side=(uint32_t)std::sqrt((float)total)+1;
                uint32_t row=slotIdx/side, col=slotIdx%side;
                lx=((float)col-(side-1)*0.5f)*spacing;
                lz=(float)row*spacing; break;
            }
        }
        // Rotate by leader yaw
        float wx=cy*lx-sy*lz, wz=sy*lx+cy*lz;
        return {leader.x+wx, leader.y, leader.z+wz};
    }
};

FormationSystem::FormationSystem(): m_impl(new Impl){}
FormationSystem::~FormationSystem(){ Shutdown(); delete m_impl; }
void FormationSystem::Init(){}
void FormationSystem::Shutdown(){ m_impl->formations.clear(); m_impl->agentToForm.clear(); m_impl->agentToSlot.clear(); }
void FormationSystem::Reset(){ Shutdown(); m_impl->nextFormId=1; }

uint32_t FormationSystem::CreateFormation(FormationShape shape, uint32_t slots, float spacing){
    FormationData fd;
    fd.id=m_impl->nextFormId++;
    fd.shape=shape; fd.slotCount=slots; fd.spacing=spacing;
    fd.slotToAgent.assign(slots,0);
    fd.slotPositions.resize(slots);
    m_impl->formations.push_back(fd);
    return fd.id;
}

void FormationSystem::DisbandFormation(uint32_t id){
    BreakFormation(id);
    m_impl->formations.erase(
        std::remove_if(m_impl->formations.begin(),m_impl->formations.end(),
            [id](const FormationData& f){return f.id==id;}),
        m_impl->formations.end());
}

void FormationSystem::BreakFormation(uint32_t id){
    auto* fd=m_impl->Find(id); if(!fd) return;
    for(auto agent : fd->slotToAgent){
        if(agent){ m_impl->agentToForm.erase(agent); m_impl->agentToSlot.erase(agent); }
    }
    std::fill(fd->slotToAgent.begin(),fd->slotToAgent.end(),0u);
}

bool FormationSystem::AssignAgent(uint32_t formId, uint32_t agentId, uint32_t slotIdx){
    auto* fd=m_impl->Find(formId); if(!fd||slotIdx>=fd->slotCount) return false;
    fd->slotToAgent[slotIdx]=agentId;
    m_impl->agentToForm[agentId]=formId;
    m_impl->agentToSlot[agentId]=slotIdx;
    return true;
}

void FormationSystem::AutoAssign(uint32_t formId, const std::vector<uint32_t>& agents){
    auto* fd=m_impl->Find(formId); if(!fd) return;
    uint32_t n=std::min((uint32_t)agents.size(), fd->slotCount);
    for(uint32_t i=0;i<n;i++) AssignAgent(formId,agents[i],i);
}

void FormationSystem::UnassignAgent(uint32_t agentId){
    auto it=m_impl->agentToForm.find(agentId);
    if(it==m_impl->agentToForm.end()) return;
    auto* fd=m_impl->Find(it->second);
    if(fd){
        auto sit=m_impl->agentToSlot.find(agentId);
        if(sit!=m_impl->agentToSlot.end() && sit->second<fd->slotCount)
            fd->slotToAgent[sit->second]=0;
    }
    m_impl->agentToForm.erase(agentId);
    m_impl->agentToSlot.erase(agentId);
}

void FormationSystem::Update(uint32_t formId, FVec3 lpos, float lyaw){
    auto* fd=m_impl->Find(formId); if(!fd) return;
    fd->lastLeaderPos=lpos; fd->lastLeaderYaw=lyaw;
    for(uint32_t i=0;i<fd->slotCount;i++)
        fd->slotPositions[i]=m_impl->ComputeSlot(fd->shape,i,fd->slotCount,lpos,lyaw,fd->spacing);
}

FVec3 FormationSystem::GetSlotWorldPos(uint32_t formId, uint32_t slotIdx, FVec3 lpos, float lyaw) const {
    auto* fd=m_impl->Find(formId);
    if(!fd) return lpos;
    return m_impl->ComputeSlot(fd->shape,slotIdx,fd->slotCount,lpos,lyaw,fd->spacing);
}

FVec3 FormationSystem::GetTargetPos(uint32_t agentId) const {
    auto it=m_impl->agentToForm.find(agentId);
    if(it==m_impl->agentToForm.end()) return {0,0,0};
    auto sit=m_impl->agentToSlot.find(agentId);
    if(sit==m_impl->agentToSlot.end()) return {0,0,0};
    auto* fd=m_impl->Find(it->second);
    if(!fd||sit->second>=fd->slotPositions.size()) return {0,0,0};
    return fd->slotPositions[sit->second];
}

FVec3 FormationSystem::GetFormationCentre(uint32_t formId) const {
    auto* fd=m_impl->Find(formId); return fd?fd->lastLeaderPos:FVec3{0,0,0};
}
uint32_t FormationSystem::GetSlotCount(uint32_t formId) const {
    auto* fd=m_impl->Find(formId); return fd?fd->slotCount:0;
}
int32_t FormationSystem::GetAgentSlot(uint32_t agentId) const {
    auto it=m_impl->agentToSlot.find(agentId);
    return (it!=m_impl->agentToSlot.end())?(int32_t)it->second:-1;
}
void FormationSystem::SetSpacing(uint32_t formId, float s){
    auto* fd=m_impl->Find(formId); if(fd) fd->spacing=s;
}
void FormationSystem::SetFormationShape(uint32_t formId, FormationShape shape){
    auto* fd=m_impl->Find(formId); if(fd) fd->shape=shape;
}

} // namespace Engine
