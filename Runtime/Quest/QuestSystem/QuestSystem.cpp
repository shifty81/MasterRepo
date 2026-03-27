#include "Runtime/Quest/QuestSystem/QuestSystem.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct QuestSystem::Impl {
    std::vector<QuestDef>   defs;
    std::vector<QuestState_> states;

    std::function<void(const std::string&)> onActivate;
    std::function<void(const std::string&)> onComplete;
    std::function<void(const std::string&)> onFail;
    std::function<void(const std::string&, const std::string&, uint32_t)> onProgress;

    const QuestDef* FindDef(const std::string& id) const {
        for (auto& d:defs) if(d.id==id) return &d; return nullptr;
    }
    QuestState_* FindState(const std::string& id) {
        for (auto& s:states) if(s.questId==id) return &s; return nullptr;
    }
    const QuestState_* FindState(const std::string& id) const {
        for (auto& s:states) if(s.questId==id) return &s; return nullptr;
    }
};

QuestSystem::QuestSystem()  : m_impl(new Impl) {}
QuestSystem::~QuestSystem() { Shutdown(); delete m_impl; }

void QuestSystem::Init()     {}
void QuestSystem::Shutdown() { m_impl->defs.clear(); m_impl->states.clear(); }

void QuestSystem::Register(const QuestDef& def)
{
    m_impl->defs.push_back(def);
    QuestState_ qs;
    qs.questId = def.id;
    qs.state   = QuestState::Inactive;
    qs.timeRemaining = def.timeLimit;
    for (auto& obj : def.objectives) {
        ObjectiveState os; os.id=obj.id;
        qs.objectives.push_back(os);
    }
    m_impl->states.push_back(qs);
}

bool QuestSystem::Has(const std::string& id) const { return m_impl->FindDef(id)!=nullptr; }
const QuestDef* QuestSystem::Get(const std::string& id) const { return m_impl->FindDef(id); }
std::vector<QuestDef> QuestSystem::GetAll() const { return m_impl->defs; }
std::vector<QuestDef> QuestSystem::GetByCategory(const std::string& cat) const {
    std::vector<QuestDef> out;
    for (auto& d:m_impl->defs) if(d.category==cat) out.push_back(d);
    return out;
}

bool QuestSystem::CanActivate(const std::string& id) const {
    const auto* def = m_impl->FindDef(id); if(!def) return false;
    for (auto& prereq : def->prerequisites) {
        const auto* s=m_impl->FindState(prereq);
        if (!s||s->state!=QuestState::Completed) return false;
    }
    const auto* st=m_impl->FindState(id);
    return st&&(st->state==QuestState::Inactive||def->repeatable);
}

bool QuestSystem::Activate(const std::string& id) {
    if (!CanActivate(id)) return false;
    auto* s=m_impl->FindState(id);
    if (!s) return false;
    s->state=QuestState::Active;
    const auto* def=m_impl->FindDef(id);
    if (def&&def->timeLimit>0.f) s->timeRemaining=def->timeLimit;
    // Reset objectives
    for (auto& obj:s->objectives) { obj.progress=0; obj.complete=false; }
    if (m_impl->onActivate) m_impl->onActivate(id);
    return true;
}

bool QuestSystem::Abandon(const std::string& id) {
    auto* s=m_impl->FindState(id); if(!s||s->state!=QuestState::Active) return false;
    s->state=QuestState::Abandoned; return true;
}
bool QuestSystem::Fail(const std::string& id) {
    auto* s=m_impl->FindState(id); if(!s) return false;
    s->state=QuestState::Failed;
    if (m_impl->onFail) m_impl->onFail(id);
    return true;
}
bool QuestSystem::Complete(const std::string& id) {
    auto* s=m_impl->FindState(id); if(!s) return false;
    s->state=QuestState::Completed;
    if (m_impl->onComplete) m_impl->onComplete(id);
    return true;
}

void QuestSystem::AddObjectiveProgress(const std::string& qid, const std::string& oid, uint32_t delta) {
    auto* s=m_impl->FindState(qid); if(!s||s->state!=QuestState::Active) return;
    const auto* def=m_impl->FindDef(qid); if(!def) return;
    for (auto& obj:s->objectives) {
        if (obj.id!=oid) continue;
        // Find goal
        uint32_t goal=1;
        for (auto& od:def->objectives) if(od.id==oid){goal=od.goal;break;}
        obj.progress=std::min(obj.progress+delta, goal);
        obj.complete=(obj.progress>=goal);
        if (m_impl->onProgress) m_impl->onProgress(qid,oid,obj.progress);
        break;
    }
    // Check all non-optional objectives complete
    bool allDone=true;
    for (auto& obj:s->objectives) {
        const ObjectiveDef* od=nullptr;
        for (auto& d:def->objectives) if(d.id==obj.id){od=&d;break;}
        if (od&&!od->optional&&!obj.complete) { allDone=false; break; }
    }
    if (allDone) Complete(qid);
}

void QuestSystem::SetObjectiveProgress(const std::string& qid, const std::string& oid, uint32_t v) {
    auto* s=m_impl->FindState(qid); if(!s) return;
    const auto* def=m_impl->FindDef(qid); if(!def) return;
    for (auto& obj:s->objectives) {
        if (obj.id!=oid) continue;
        uint32_t goal=1;
        for (auto& od:def->objectives) if(od.id==oid){goal=od.goal;break;}
        obj.progress=std::min(v,goal); obj.complete=(obj.progress>=goal);
        break;
    }
}

uint32_t QuestSystem::GetObjectiveProgress(const std::string& qid, const std::string& oid) const {
    const auto* s=m_impl->FindState(qid); if(!s) return 0;
    for (auto& obj:s->objectives) if(obj.id==oid) return obj.progress;
    return 0;
}
bool QuestSystem::IsObjectiveComplete(const std::string& qid, const std::string& oid) const {
    const auto* s=m_impl->FindState(qid); if(!s) return false;
    for (auto& obj:s->objectives) if(obj.id==oid) return obj.complete;
    return false;
}

QuestState QuestSystem::GetState(const std::string& id) const {
    const auto* s=m_impl->FindState(id); return s?s->state:QuestState::Inactive;
}
const QuestState_* QuestSystem::GetQState(const std::string& id) const {
    return m_impl->FindState(id);
}

std::vector<std::string> QuestSystem::GetActive() const {
    std::vector<std::string> out;
    for (auto& s:m_impl->states) if(s.state==QuestState::Active) out.push_back(s.questId);
    return out;
}
std::vector<std::string> QuestSystem::GetCompleted() const {
    std::vector<std::string> out;
    for (auto& s:m_impl->states) if(s.state==QuestState::Completed) out.push_back(s.questId);
    return out;
}
float QuestSystem::CompletionPct() const {
    if (m_impl->defs.empty()) return 0.f;
    uint32_t n=0; for(auto& s:m_impl->states) if(s.state==QuestState::Completed) n++;
    return 100.f*n/(float)m_impl->defs.size();
}

void QuestSystem::SetOnActivate (std::function<void(const std::string&)> cb) { m_impl->onActivate=cb; }
void QuestSystem::SetOnComplete (std::function<void(const std::string&)> cb) { m_impl->onComplete=cb; }
void QuestSystem::SetOnFail     (std::function<void(const std::string&)> cb) { m_impl->onFail=cb; }
void QuestSystem::SetOnProgress (std::function<void(const std::string&,const std::string&,uint32_t)> cb) { m_impl->onProgress=cb; }

std::string QuestSystem::Serialize() const {
    std::ostringstream ss;
    ss << "{\"quests\":[";
    bool first=true;
    for (auto& s:m_impl->states) {
        if(!first) ss<<","; first=false;
        ss << "{\"id\":\"" << s.questId << "\",\"state\":" << (int)s.state << "}";
    }
    ss << "]}";
    return ss.str();
}
bool QuestSystem::Deserialize(const std::string& /*json*/) { return true; }

void QuestSystem::Tick(float dt) {
    for (auto& s:m_impl->states) {
        if (s.state!=QuestState::Active) continue;
        const auto* def=m_impl->FindDef(s.questId);
        if (!def||def->timeLimit<=0.f) continue;
        s.timeRemaining-=dt;
        if (s.timeRemaining<=0.f) Fail(s.questId);
    }
}

} // namespace Runtime
