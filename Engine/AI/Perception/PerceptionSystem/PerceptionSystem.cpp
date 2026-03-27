#include "Engine/AI/Perception/PerceptionSystem/PerceptionSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

static float Dist3(PSVec3 a, PSVec3 b){
    float dx=a.x-b.x, dy=a.y-b.y, dz=a.z-b.z;
    return std::sqrt(dx*dx+dy*dy+dz*dz);
}
static float Dot3(PSVec3 a, PSVec3 b){ return a.x*b.x+a.y*b.y+a.z*b.z; }
static float Len3(PSVec3 v){ return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }

struct StimulusData {
    uint32_t     stimId;
    StimulusType type;
    PSVec3       pos{};
    float        strength{1};
};

struct AgentData {
    uint32_t    id;
    float       sightRange{20};
    float       sightHalfAngleCos{0}; // cos(halfAngle)
    float       hearingRange{15};
    uint32_t    team{0};
    PSVec3      pos{};
    PSVec3      forward{0,0,1};
    std::vector<Percept> percepts;
    std::function<void(const Percept&)> onPerceived;
    std::function<void(uint32_t)>       onLost;
};

struct PerceptionSystem::Impl {
    float forgetTime{3.f};
    std::vector<AgentData>    agents;
    std::vector<StimulusData> stimuli;
    std::function<bool(PSVec3,PSVec3)> sightBlocker;

    AgentData* FindAgent(uint32_t id){
        for(auto& a:agents) if(a.id==id) return &a; return nullptr;
    }
    StimulusData* FindStim(uint32_t id){
        for(auto& s:stimuli) if(s.stimId==id) return &s; return nullptr;
    }
};

PerceptionSystem::PerceptionSystem(): m_impl(new Impl){}
PerceptionSystem::~PerceptionSystem(){ Shutdown(); delete m_impl; }
void PerceptionSystem::Init(){}
void PerceptionSystem::Shutdown(){ m_impl->agents.clear(); m_impl->stimuli.clear(); }
void PerceptionSystem::Reset(){ Shutdown(); }

void PerceptionSystem::RegisterAgent(uint32_t id, float sr, float sa, float hr){
    AgentData a; a.id=id; a.sightRange=sr; a.hearingRange=hr;
    float halfRad=sa*0.5f*3.14159265f/180.f;
    a.sightHalfAngleCos=std::cos(halfRad);
    m_impl->agents.push_back(a);
}
void PerceptionSystem::UnregisterAgent(uint32_t id){
    m_impl->agents.erase(std::remove_if(m_impl->agents.begin(),m_impl->agents.end(),
        [id](const AgentData& a){return a.id==id;}),m_impl->agents.end());
}
void PerceptionSystem::SetAgentTeam(uint32_t id, uint32_t team){
    auto* a=m_impl->FindAgent(id); if(a) a->team=team;
}
void PerceptionSystem::SetAgentPos(uint32_t id, PSVec3 pos, PSVec3 fwd){
    auto* a=m_impl->FindAgent(id);
    if(a){ a->pos=pos; float l=Len3(fwd); if(l>0) a->forward={fwd.x/l,fwd.y/l,fwd.z/l}; }
}

void PerceptionSystem::RegisterStimulus(uint32_t sid, StimulusType t, PSVec3 pos, float str){
    for(auto& s:m_impl->stimuli) if(s.stimId==sid){ s.type=t; s.pos=pos; s.strength=str; return; }
    StimulusData sd; sd.stimId=sid; sd.type=t; sd.pos=pos; sd.strength=str;
    m_impl->stimuli.push_back(sd);
}
void PerceptionSystem::UpdateStimulus(uint32_t sid, PSVec3 pos, float str){
    auto* s=m_impl->FindStim(sid); if(s){ s->pos=pos; s->strength=str; }
}
void PerceptionSystem::RemoveStimulus(uint32_t sid){
    m_impl->stimuli.erase(std::remove_if(m_impl->stimuli.begin(),m_impl->stimuli.end(),
        [sid](const StimulusData& s){return s.stimId==sid;}),m_impl->stimuli.end());
}
void PerceptionSystem::SetSightBlocker(std::function<bool(PSVec3,PSVec3)> fn){
    m_impl->sightBlocker=fn;
}
void PerceptionSystem::SetForgetTime(float sec){ m_impl->forgetTime=sec; }

void PerceptionSystem::Update(float dt){
    for(auto& agent:m_impl->agents){
        // Age existing percepts
        for(auto& p:agent.percepts){ p.age+=dt; p.isFresh=false; }

        for(auto& stim:m_impl->stimuli){
            float dist=Dist3(agent.pos, stim.pos);
            bool detected=false;

            if(stim.type==StimulusType::Visual){
                if(dist<=agent.sightRange){
                    PSVec3 dir={(stim.pos.x-agent.pos.x)/dist,
                                (stim.pos.y-agent.pos.y)/dist,
                                (stim.pos.z-agent.pos.z)/dist};
                    float cosA=Dot3(agent.forward,dir);
                    if(cosA>=agent.sightHalfAngleCos){
                        if(!m_impl->sightBlocker||!m_impl->sightBlocker(agent.pos,stim.pos))
                            detected=true;
                    }
                }
            } else if(stim.type==StimulusType::Audio){
                if(dist<=agent.hearingRange*stim.strength) detected=true;
            } else {
                if(dist<=1.5f) detected=true; // Touch/Custom: close range
            }

            if(detected){
                // Find existing or create new percept
                Percept* existing=nullptr;
                for(auto& p:agent.percepts) if(p.stimId==stim.stimId){ existing=&p; break; }
                bool isNew=false;
                if(!existing){
                    Percept np{}; np.stimId=stim.stimId; np.type=stim.type;
                    agent.percepts.push_back(np);
                    existing=&agent.percepts.back();
                    isNew=true;
                }
                existing->lastKnownPos=stim.pos;
                existing->strength=stim.strength;
                existing->age=0;
                existing->isFresh=true;
                if(isNew&&agent.onPerceived) agent.onPerceived(*existing);
            }
        }

        // Expire forgotten percepts
        for(auto it=agent.percepts.begin();it!=agent.percepts.end();){
            if(it->age>m_impl->forgetTime){
                if(agent.onLost) agent.onLost(it->stimId);
                it=agent.percepts.erase(it);
            } else ++it;
        }
    }
}

std::vector<Percept> PerceptionSystem::GetPercepts(uint32_t agentId) const {
    auto* a=m_impl->FindAgent(agentId); return a?a->percepts:std::vector<Percept>{};
}
bool PerceptionSystem::IsAware(uint32_t agentId, uint32_t stimId) const {
    auto* a=m_impl->FindAgent(agentId); if(!a) return false;
    for(auto& p:a->percepts) if(p.stimId==stimId) return true;
    return false;
}

void PerceptionSystem::SetOnPerceived(uint32_t id, std::function<void(const Percept&)> cb){
    auto* a=m_impl->FindAgent(id); if(a) a->onPerceived=cb;
}
void PerceptionSystem::SetOnLost(uint32_t id, std::function<void(uint32_t)> cb){
    auto* a=m_impl->FindAgent(id); if(a) a->onLost=cb;
}

} // namespace Engine
