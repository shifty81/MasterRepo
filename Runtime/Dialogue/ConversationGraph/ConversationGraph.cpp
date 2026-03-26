#include "Runtime/Dialogue/ConversationGraph/ConversationGraph.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Runtime {

static bool EvalCondition(const DialogueCondition& c,
                           const std::unordered_map<std::string,std::string>& vars) {
    auto it=vars.find(c.variable); if(it==vars.end()) return false;
    auto& actual=it->second;
    if(c.op=="==") return actual==c.value;
    if(c.op=="!=") return actual!=c.value;
    try {
        float a=std::stof(actual), b=std::stof(c.value);
        if(c.op==">")  return a>b;
        if(c.op=="<")  return a<b;
        if(c.op==">=") return a>=b;
        if(c.op=="<=") return a<=b;
    } catch(...) {}
    return false;
}

struct ConversationGraph::Impl {
    std::unordered_map<uint32_t,DialogueNode>   nodes;
    uint32_t nextId{1};

    ConversationState state{ConversationState::Idle};
    uint32_t          currentNodeId{0};
    float             autoAdvanceTimer{0.f};

    std::unordered_map<std::string,std::string> variables;

    std::function<void(const DialogueNode&)>              onNodeEnter;
    std::function<void(const std::vector<DialogueChoice>&)> onChoice;
    std::function<void()>                                 onFinished;
    std::function<void(const DialogueEffect&)>            onEffect;

    void EnterNode(uint32_t id){
        auto it=nodes.find(id); if(it==nodes.end()){ Finish(); return; }
        currentNodeId=id;
        auto& n=it->second;
        // Fire effects
        for(auto& e:n.effects) if(onEffect) onEffect(e);
        if(onNodeEnter) onNodeEnter(n);
        // Check if we have choices
        std::vector<DialogueChoice> avail;
        for(auto& ch:n.choices){
            bool ok=true;
            for(auto& c:ch.prerequisites) if(!EvalCondition(c,variables)){ ok=false; break; }
            if(ok) avail.push_back(ch);
        }
        if(!avail.empty()){
            state=ConversationState::WaitingChoice;
            if(onChoice) onChoice(avail);
        } else if(n.autoNextId){
            state=ConversationState::Active;
            autoAdvanceTimer=n.autoAdvanceDelay;
        } else {
            Finish();
        }
    }

    void Finish(){
        state=ConversationState::Finished;
        if(onFinished) onFinished();
    }
};

ConversationGraph::ConversationGraph() : m_impl(new Impl()) {}
ConversationGraph::~ConversationGraph() { delete m_impl; }

bool ConversationGraph::LoadFromFile(const std::string& path){
    std::ifstream f(path); return f.is_open();
}
bool ConversationGraph::SaveToFile(const std::string& path) const {
    std::ofstream f(path); if(!f.is_open()) return false;
    f<<"{\"nodeCount\":"<<m_impl->nodes.size()<<"}\n"; return true;
}
void ConversationGraph::Clear() { m_impl->nodes.clear(); m_impl->nextId=1; }

uint32_t ConversationGraph::AddNode(const DialogueNode& node){
    uint32_t id=node.id?node.id:m_impl->nextId++;
    m_impl->nodes[id]=node; m_impl->nodes[id].id=id;
    return id;
}
void ConversationGraph::UpdateNode(const DialogueNode& node){
    auto it=m_impl->nodes.find(node.id); if(it!=m_impl->nodes.end()) it->second=node;
}
void ConversationGraph::RemoveNode(uint32_t id){ m_impl->nodes.erase(id); }
DialogueNode ConversationGraph::GetNode(uint32_t id) const{
    auto it=m_impl->nodes.find(id); return it!=m_impl->nodes.end()?it->second:DialogueNode{};
}
std::vector<DialogueNode> ConversationGraph::AllNodes() const{
    std::vector<DialogueNode> out; for(auto&[k,v]:m_impl->nodes) out.push_back(v); return out;
}

void ConversationGraph::Start(uint32_t startId){
    m_impl->state=ConversationState::Active;
    if(startId==0 && !m_impl->nodes.empty()) startId=m_impl->nodes.begin()->second.id;
    m_impl->EnterNode(startId);
}
void ConversationGraph::Stop(){ m_impl->state=ConversationState::Idle; }
bool ConversationGraph::IsActive() const{
    return m_impl->state==ConversationState::Active||m_impl->state==ConversationState::WaitingChoice;
}

DialogueNode ConversationGraph::CurrentNode() const{
    return GetNode(m_impl->currentNodeId);
}

std::vector<DialogueChoice> ConversationGraph::GetChoices() const{
    auto it=m_impl->nodes.find(m_impl->currentNodeId);
    if(it==m_impl->nodes.end()) return {};
    std::vector<DialogueChoice> avail;
    for(auto& ch:it->second.choices){
        bool ok=true;
        for(auto& c:ch.prerequisites) if(!EvalCondition(c,m_impl->variables)){ ok=false; break; }
        if(ok) avail.push_back(ch);
    }
    return avail;
}

bool ConversationGraph::Choose(uint32_t choiceId){
    auto choices=GetChoices();
    for(auto& ch:choices) if(ch.id==choiceId){ m_impl->EnterNode(ch.nextNodeId); return true; }
    return false;
}

void ConversationGraph::Advance(){
    auto it=m_impl->nodes.find(m_impl->currentNodeId);
    if(it!=m_impl->nodes.end()&&it->second.autoNextId) m_impl->EnterNode(it->second.autoNextId);
    else m_impl->Finish();
}

void ConversationGraph::Tick(float dt){
    if(m_impl->state==ConversationState::Active){
        m_impl->autoAdvanceTimer-=dt;
        if(m_impl->autoAdvanceTimer<=0.f) Advance();
    }
}

ConversationState ConversationGraph::GetState() const{ return m_impl->state; }

void ConversationGraph::SetVariable(const std::string& k, const std::string& v){ m_impl->variables[k]=v; }
std::string ConversationGraph::GetVariable(const std::string& k) const{
    auto it=m_impl->variables.find(k); return it!=m_impl->variables.end()?it->second:"";
}

void ConversationGraph::OnNodeEnter(std::function<void(const DialogueNode&)> cb){ m_impl->onNodeEnter=std::move(cb); }
void ConversationGraph::OnChoiceAvailable(std::function<void(const std::vector<DialogueChoice>&)> cb){ m_impl->onChoice=std::move(cb); }
void ConversationGraph::OnFinished(std::function<void()> cb){ m_impl->onFinished=std::move(cb); }
void ConversationGraph::OnEffect(std::function<void(const DialogueEffect&)> cb){ m_impl->onEffect=std::move(cb); }

} // namespace Runtime
