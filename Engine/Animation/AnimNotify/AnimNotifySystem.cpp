#include "Engine/Animation/AnimNotify/AnimNotifySystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Engine {

struct NotifyEntry {
    NotifyDef def;
    float     resolvedTime{0.f}; ///< in seconds (after normalisation)
};

struct StateNotifyEntry {
    NotifyStateDef def;
    float          resolvedStart{0.f};
    float          resolvedEnd{0.f};
};

struct InstanceRecord {
    uint32_t    instanceId{0};
    uint32_t    actorId{0};
    std::string clipName;
    float       clipLength{0.f};
    float       currentTime{0.f};
    float       lastTime{0.f};
    float       speed{1.f};
    bool        loop{false};
    bool        active{true};
    // Active state-notify tracking
    std::unordered_set<std::string> activeStates;
};

struct SubRecord {
    uint32_t subId{0};
    std::string notifyName;
    NotifyCb    cb;
};

struct StateSubRecord {
    uint32_t    subId{0};
    std::string notifyName;
    NotifyStateCb cb;
};

struct AnimNotifySystem::Impl {
    // clip → list of notifies
    std::unordered_map<std::string, std::vector<NotifyEntry>>      notifies;
    std::unordered_map<std::string, std::vector<StateNotifyEntry>> stateNotifies;
    std::unordered_set<std::string> disabledGroups;

    std::unordered_map<uint32_t, InstanceRecord> instances;
    std::vector<SubRecord>      subs;
    std::vector<StateSubRecord> stateSubs;
    uint32_t nextInstanceId{1};
    uint32_t nextSubId{1};

    void FireNotify(const InstanceRecord& inst, const NotifyEntry& ne) {
        NotifyEvent evt;
        evt.actorId    = inst.actorId;
        evt.instanceId = inst.instanceId;
        evt.clipName   = inst.clipName;
        evt.notifyName = ne.def.notifyName;
        evt.clipTime   = inst.currentTime;
        evt.clipLength = inst.clipLength;
        for (auto& s : subs) {
            if (s.notifyName == ne.def.notifyName && s.cb) s.cb(evt);
        }
    }

    void FireStateNotify(const InstanceRecord& inst, const StateNotifyEntry& sne,
                          NotifyStatePhase phase) {
        NotifyStateEvent evt;
        evt.actorId    = inst.actorId;
        evt.instanceId = inst.instanceId;
        evt.notifyName = sne.def.notifyName;
        evt.elapsedInState = inst.currentTime - sne.resolvedStart;
        for (auto& s : stateSubs) {
            if (s.notifyName == sne.def.notifyName && s.cb) s.cb(evt, phase);
        }
    }
};

AnimNotifySystem::AnimNotifySystem()  : m_impl(new Impl) {}
AnimNotifySystem::~AnimNotifySystem() { Shutdown(); delete m_impl; }

void AnimNotifySystem::Init()     {}
void AnimNotifySystem::Shutdown() { m_impl->instances.clear(); }

static float Resolve(float t, float len, bool norm) {
    return norm ? t * len : t;
}

void AnimNotifySystem::RegisterNotify(const NotifyDef& def)
{
    NotifyEntry ne;
    ne.def = def;
    // resolvedTime set per-instance; store raw
    m_impl->notifies[def.clipName].push_back(ne);
}

void AnimNotifySystem::RegisterStateNotify(const NotifyStateDef& def)
{
    StateNotifyEntry sne;
    sne.def = def;
    m_impl->stateNotifies[def.clipName].push_back(sne);
}

void AnimNotifySystem::UnregisterNotify(const std::string& clipName,
                                         const std::string& notifyName)
{
    auto it = m_impl->notifies.find(clipName);
    if (it == m_impl->notifies.end()) return;
    auto& v = it->second;
    v.erase(std::remove_if(v.begin(),v.end(),
        [&](auto& ne){ return ne.def.notifyName==notifyName; }), v.end());
}

void AnimNotifySystem::ClearClip(const std::string& clipName)
{
    m_impl->notifies.erase(clipName);
    m_impl->stateNotifies.erase(clipName);
}

void AnimNotifySystem::EnableGroup(const std::string& group)  { m_impl->disabledGroups.erase(group); }
void AnimNotifySystem::DisableGroup(const std::string& group) { m_impl->disabledGroups.insert(group); }

uint32_t AnimNotifySystem::Subscribe(const std::string& notifyName, NotifyCb cb)
{
    uint32_t id = m_impl->nextSubId++;
    m_impl->subs.push_back({id, notifyName, cb});
    return id;
}

uint32_t AnimNotifySystem::SubscribeState(const std::string& notifyName, NotifyStateCb cb)
{
    uint32_t id = m_impl->nextSubId++;
    m_impl->stateSubs.push_back({id, notifyName, cb});
    return id;
}

void AnimNotifySystem::Unsubscribe(uint32_t subId)
{
    auto& s = m_impl->subs;
    s.erase(std::remove_if(s.begin(),s.end(),[&](auto& r){ return r.subId==subId; }), s.end());
    auto& ss = m_impl->stateSubs;
    ss.erase(std::remove_if(ss.begin(),ss.end(),[&](auto& r){ return r.subId==subId; }), ss.end());
}

uint32_t AnimNotifySystem::CreateInstance(uint32_t actorId,
                                           const std::string& clipName,
                                           float clipLength)
{
    uint32_t id = m_impl->nextInstanceId++;
    InstanceRecord rec;
    rec.instanceId  = id;
    rec.actorId     = actorId;
    rec.clipName    = clipName;
    rec.clipLength  = clipLength;
    m_impl->instances[id] = rec;
    return id;
}

void AnimNotifySystem::DestroyInstance(uint32_t instanceId)
{
    m_impl->instances.erase(instanceId);
}

void AnimNotifySystem::AdvanceInstance(uint32_t instanceId, float dt)
{
    auto it = m_impl->instances.find(instanceId);
    if (it == m_impl->instances.end()) return;
    auto& inst = it->second;
    if (!inst.active) return;

    float prev = inst.currentTime;
    inst.currentTime += dt * inst.speed;

    if (inst.loop && inst.currentTime >= inst.clipLength)
        inst.currentTime = std::fmod(inst.currentTime, inst.clipLength);
    else
        inst.currentTime = std::min(inst.currentTime, inst.clipLength);

    float cur = inst.currentTime;

    // Point notifies
    auto nit = m_impl->notifies.find(inst.clipName);
    if (nit != m_impl->notifies.end()) {
        for (auto& ne : nit->second) {
            if (m_impl->disabledGroups.count(ne.def.group)) continue;
            float t = Resolve(ne.def.time, inst.clipLength, ne.def.useNormalized);
            bool crossed = (prev < t && cur >= t);
            if (crossed) m_impl->FireNotify(inst, ne);
        }
    }

    // State notifies
    auto snit = m_impl->stateNotifies.find(inst.clipName);
    if (snit != m_impl->stateNotifies.end()) {
        for (auto& sne : snit->second) {
            if (m_impl->disabledGroups.count(sne.def.group)) continue;
            float s = Resolve(sne.def.startTime, inst.clipLength, sne.def.useNormalized);
            float e = Resolve(sne.def.endTime,   inst.clipLength, sne.def.useNormalized);
            bool wasIn = (prev >= s && prev < e);
            bool nowIn = (cur  >= s && cur  < e);
            if (!wasIn && nowIn) {
                inst.activeStates.insert(sne.def.notifyName);
                m_impl->FireStateNotify(inst, sne, NotifyStatePhase::Enter);
            } else if (wasIn && nowIn) {
                m_impl->FireStateNotify(inst, sne, NotifyStatePhase::Tick);
            } else if (wasIn && !nowIn) {
                inst.activeStates.erase(sne.def.notifyName);
                m_impl->FireStateNotify(inst, sne, NotifyStatePhase::Exit);
            }
        }
    }

    inst.lastTime = prev;
}

void AnimNotifySystem::SeekInstance(uint32_t instanceId, float time)
{
    auto it = m_impl->instances.find(instanceId);
    if (it == m_impl->instances.end()) return;
    it->second.lastTime    = time;
    it->second.currentTime = time;
}

void AnimNotifySystem::SetInstanceLoop(uint32_t instanceId, bool loop)
{
    auto it = m_impl->instances.find(instanceId);
    if (it != m_impl->instances.end()) it->second.loop = loop;
}

void AnimNotifySystem::SetInstanceSpeed(uint32_t instanceId, float speed)
{
    auto it = m_impl->instances.find(instanceId);
    if (it != m_impl->instances.end()) it->second.speed = speed;
}

void AnimNotifySystem::Tick(float dt)
{
    for (auto& [id, inst] : m_impl->instances) AdvanceInstance(id, dt);
}

} // namespace Engine
