#include "Runtime/NPC/NPCController.h"
#include <unordered_map>
#include <cmath>

namespace Runtime {

// ── per-NPC state ─────────────────────────────────────────────────────────────
struct NPCState {
    NPCParams    params;
    NPCBehaviour current{NPCBehaviour::Idle};
    float        stateTimer{0.0f};
    float        reactionAccum{0.0f};
};

// ── Impl ─────────────────────────────────────────────────────────────────────
struct NPCController::Impl {
    std::unordered_map<uint32_t, NPCState> npcs;
    std::vector<NPCStateCallback>          onState;
    std::vector<NPCActionCallback>         onAction;
};

NPCController::NPCController() : m_impl(new Impl()) {}
NPCController::~NPCController() { delete m_impl; }

void NPCController::RegisterNPC(uint32_t npcId, const NPCParams& params) {
    NPCState s;
    s.params  = params;
    s.current = params.behaviour;
    m_impl->npcs[npcId] = s;
}

void NPCController::UnregisterNPC(uint32_t npcId) {
    m_impl->npcs.erase(npcId);
}

bool NPCController::HasNPC(uint32_t npcId) const {
    return m_impl->npcs.count(npcId) > 0;
}

std::vector<uint32_t> NPCController::GetNPCIds() const {
    std::vector<uint32_t> ids;
    ids.reserve(m_impl->npcs.size());
    for (auto& [id, _] : m_impl->npcs) ids.push_back(id);
    return ids;
}

void NPCController::TuneParams(uint32_t npcId, const NPCParams& params) {
    auto it = m_impl->npcs.find(npcId);
    if (it == m_impl->npcs.end()) return;
    NPCBehaviour prev = it->second.current;
    it->second.params = params;
    if (params.behaviour != prev) {
        it->second.current = params.behaviour;
        NPCStateEvent ev{npcId, prev, params.behaviour, "TuneParams"};
        for (auto& cb : m_impl->onState) cb(ev);
    }
}

NPCParams NPCController::GetParams(uint32_t npcId) const {
    auto it = m_impl->npcs.find(npcId);
    return it != m_impl->npcs.end() ? it->second.params : NPCParams{};
}

void NPCController::ForceBehaviour(uint32_t npcId, NPCBehaviour behaviour,
                                    const std::string& reason)
{
    auto it = m_impl->npcs.find(npcId);
    if (it == m_impl->npcs.end()) return;
    NPCBehaviour prev = it->second.current;
    if (prev == behaviour) return;
    it->second.current    = behaviour;
    it->second.stateTimer = 0.0f;
    NPCStateEvent ev{npcId, prev, behaviour, reason.empty() ? "ForceBehaviour" : reason};
    for (auto& cb : m_impl->onState) cb(ev);
}

void NPCController::Update(float dt) {
    for (auto& [id, state] : m_impl->npcs) {
        state.stateTimer    += dt;
        state.reactionAccum += dt;

        if (state.reactionAccum < state.params.reactionTimeSec) continue;
        state.reactionAccum = 0.0f;

        // Simple behaviour-tree tick.
        std::string action;
        switch (state.current) {
        case NPCBehaviour::Idle:
            action = "idle";
            // After 5 s of idling, switch to patrol if configured.
            if (state.stateTimer > 5.0f &&
                state.params.behaviour == NPCBehaviour::Patrol) {
                ForceBehaviour(id, NPCBehaviour::Patrol, "IdleTimeout");
            }
            break;
        case NPCBehaviour::Patrol: action = "patrol"; break;
        case NPCBehaviour::Guard:  action = "guard";  break;
        case NPCBehaviour::Aggressive: action = "attack";  break;
        case NPCBehaviour::Friendly:   action = "greet";   break;
        case NPCBehaviour::Fleeing:    action = "flee";    break;
        }
        for (auto& cb : m_impl->onAction) cb(id, action);
    }
}

void NPCController::Reset() {
    for (auto& [id, state] : m_impl->npcs) {
        state.current      = state.params.behaviour;
        state.stateTimer   = 0.0f;
        state.reactionAccum = 0.0f;
    }
}

void NPCController::OnStateChange(NPCStateCallback cb) {
    m_impl->onState.push_back(std::move(cb));
}
void NPCController::OnAction(NPCActionCallback cb) {
    m_impl->onAction.push_back(std::move(cb));
}

} // namespace Runtime
