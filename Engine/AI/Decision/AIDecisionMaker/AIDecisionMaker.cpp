#include "Engine/AI/Decision/AIDecisionMaker/AIDecisionMaker.h"
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Engine {

struct GoalData {
    std::string name;
    float priority{1.f};
};

struct ActionData {
    std::string name;
    std::string goalName;
    std::function<float(uint32_t,const AIBlackboard&)> scorer;
    std::function<void(uint32_t)> onExecute;
    float cooldownSec{0};
    std::unordered_map<uint32_t,float> cooldownRemaining; // agentId → remaining
};

struct AgentRecord {
    std::string lastAction;
    void*       context{nullptr};
};

struct AIDecisionMaker::Impl {
    std::vector<GoalData>   goals;
    std::vector<ActionData> actions;
    AIBlackboard            board;
    std::unordered_map<uint32_t,AgentRecord> agents;

    GoalData* FindGoal(const std::string& name){
        for(auto& g:goals) if(g.name==name) return &g;
        return nullptr;
    }
    ActionData* FindAction(const std::string& name){
        for(auto& a:actions) if(a.name==name) return &a;
        return nullptr;
    }
    float GoalPriority(const std::string& name) const {
        for(auto& g:goals) if(g.name==name) return g.priority;
        return 1.f;
    }
    bool OnCooldown(const ActionData& a, uint32_t agentId) const {
        auto it=a.cooldownRemaining.find(agentId);
        return it!=a.cooldownRemaining.end()&&it->second>0;
    }
};

AIDecisionMaker::AIDecisionMaker(): m_impl(new Impl){}
AIDecisionMaker::~AIDecisionMaker(){ Shutdown(); delete m_impl; }
void AIDecisionMaker::Init(){}
void AIDecisionMaker::Shutdown(){ m_impl->goals.clear(); m_impl->actions.clear(); m_impl->agents.clear(); }
void AIDecisionMaker::Reset(){ Shutdown(); m_impl->board.values.clear(); }

void AIDecisionMaker::Tick(float dt){
    for(auto& a:m_impl->actions){
        for(auto& [id,rem]:a.cooldownRemaining)
            rem=std::max(0.f,rem-dt);
    }
}

void AIDecisionMaker::RegisterGoal(const std::string& name, float priority){
    m_impl->goals.push_back({name,priority});
}
void AIDecisionMaker::RegisterAction(const std::string& name, const std::string& goal,
                                      std::function<float(uint32_t,const AIBlackboard&)> scorer){
    ActionData a; a.name=name; a.goalName=goal; a.scorer=scorer;
    m_impl->actions.push_back(a);
}

void  AIDecisionMaker::SetValue(const std::string& k, float v){ m_impl->board.Set(k,v); }
float AIDecisionMaker::GetValue(const std::string& k, float def) const { return m_impl->board.Get(k,def); }

std::string AIDecisionMaker::SelectBestAction(uint32_t agentId) const {
    std::string best; float bestScore=-1e30f;
    for(auto& a:m_impl->actions){
        if(m_impl->OnCooldown(a,agentId)) continue;
        if(!a.scorer) continue;
        float score=a.scorer(agentId,m_impl->board)*m_impl->GoalPriority(a.goalName);
        if(score>bestScore){ bestScore=score; best=a.name; }
    }
    return best;
}

void AIDecisionMaker::ExecuteAction(uint32_t agentId, const std::string& name){
    auto* a=m_impl->FindAction(name);
    if(!a) return;
    m_impl->agents[agentId].lastAction=name;
    if(a->cooldownSec>0) a->cooldownRemaining[agentId]=a->cooldownSec;
    if(a->onExecute) a->onExecute(agentId);
}

std::vector<std::string> AIDecisionMaker::Plan(uint32_t agentId, uint32_t depth) const {
    std::vector<std::string> plan;
    for(uint32_t i=0;i<depth;i++){
        std::string act=SelectBestAction(agentId);
        if(act.empty()) break;
        plan.push_back(act);
    }
    return plan;
}

void AIDecisionMaker::SetOnExecute(const std::string& name, std::function<void(uint32_t)> cb){
    auto* a=m_impl->FindAction(name); if(a) a->onExecute=cb;
}
void AIDecisionMaker::SetCooldown(const std::string& name, float seconds){
    auto* a=m_impl->FindAction(name); if(a) a->cooldownSec=seconds;
}
void  AIDecisionMaker::SetAgentContext(uint32_t id, void* ctx){ m_impl->agents[id].context=ctx; }
void* AIDecisionMaker::GetAgentContext(uint32_t id) const {
    auto it=m_impl->agents.find(id); return it!=m_impl->agents.end()?it->second.context:nullptr;
}
std::string AIDecisionMaker::GetLastAction(uint32_t id) const {
    auto it=m_impl->agents.find(id); return it!=m_impl->agents.end()?it->second.lastAction:"";
}

std::string AIDecisionMaker::DebugDump(uint32_t agentId) const {
    std::ostringstream os; os<<"Agent "<<agentId<<" scores:\n";
    for(auto& a:m_impl->actions){
        float s=a.scorer?a.scorer(agentId,m_impl->board)*m_impl->GoalPriority(a.goalName):0.f;
        bool cd=m_impl->OnCooldown(a,agentId);
        os<<"  "<<a.name<<"  score="<<s<<(cd?" [cooldown]":"")<<"\n";
    }
    return os.str();
}

} // namespace Engine
