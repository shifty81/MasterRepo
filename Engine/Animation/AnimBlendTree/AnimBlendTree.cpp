#include "Engine/Animation/AnimBlendTree/AnimBlendTree.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <unordered_map>
#include <variant>
#include <stdexcept>

namespace Engine {

// ── Parameter store ───────────────────────────────────────────────────────
using ParamValue = std::variant<float, int32_t, bool>;

// ── Internal state entry ──────────────────────────────────────────────────
struct AnimStateEntry {
    AnimStateId   id{kInvalidAnimStateId};
    AnimStateDesc desc;
};

// ── Internal transition entry ─────────────────────────────────────────────
struct TransitionEntry {
    TransitionId   id{kInvalidTransitionId};
    TransitionDesc desc;
};

struct AnimBlendTree::Impl {
    AnimStateId  nextStateId{1};
    TransitionId nextTransId{1};

    std::vector<AnimStateEntry>  states;
    std::vector<TransitionEntry> transitions;
    std::unordered_map<std::string, ParamValue> params;

    AnimStateId  entryState{kInvalidAnimStateId};
    AnimStateId  currentState{kInvalidAnimStateId};
    TransitionId activeTransition{kInvalidTransitionId};
    float        stateTime{0.0f};       ///< Elapsed time in current state
    float        transTime{0.0f};       ///< Elapsed time in current transition
    float        blendWeight{0.0f};     ///< toState weight during blend [0,1]

    BlendTreeStats stats;

    StateEventCb       onEnterCb;
    StateEventCb       onExitCb;
    TransitionEventCb  onTransBeginCb;

    AnimStateEntry* FindState(AnimStateId id) {
        for (auto& s : states) if (s.id == id) return &s;
        return nullptr;
    }
    const AnimStateEntry* FindState(AnimStateId id) const {
        for (const auto& s : states) if (s.id == id) return &s;
        return nullptr;
    }
    TransitionEntry* FindTransition(TransitionId id) {
        for (auto& t : transitions) if (t.id == id) return &t;
        return nullptr;
    }

    bool EvalConditions(const TransitionDesc& td) const {
        for (const auto& cond : td.conditions) {
            auto it = params.find(cond.param);
            if (it == params.end()) return false;
            const auto& val = it->second;
            switch (cond.op) {
            case CondOp::Bool:
                if (!std::holds_alternative<bool>(val)) return false;
                if (std::get<bool>(val) != cond.boolValue) return false;
                break;
            case CondOp::Greater:
                if (!std::holds_alternative<float>(val)) return false;
                if (std::get<float>(val) <= cond.threshold) return false;
                break;
            case CondOp::Less:
                if (!std::holds_alternative<float>(val)) return false;
                if (std::get<float>(val) >= cond.threshold) return false;
                break;
            case CondOp::Equal:
                if (std::holds_alternative<float>(val)) {
                    if (std::fabs(std::get<float>(val) - cond.threshold) > 1e-5f) return false;
                } else if (std::holds_alternative<int32_t>(val)) {
                    if (std::get<int32_t>(val) != static_cast<int32_t>(cond.threshold)) return false;
                }
                break;
            case CondOp::NotEqual:
                if (std::holds_alternative<float>(val)) {
                    if (std::fabs(std::get<float>(val) - cond.threshold) <= 1e-5f) return false;
                } else if (std::holds_alternative<int32_t>(val)) {
                    if (std::get<int32_t>(val) == static_cast<int32_t>(cond.threshold)) return false;
                }
                break;
            }
        }
        return true;
    }
};

AnimBlendTree::AnimBlendTree()  : m_impl(std::make_unique<Impl>()) {}
AnimBlendTree::~AnimBlendTree() = default;

// ── State management ──────────────────────────────────────────────────────
AnimStateId AnimBlendTree::AddState(const AnimStateDesc& desc) {
    AnimStateId id = m_impl->nextStateId++;
    m_impl->states.push_back({id, desc});
    ++m_impl->stats.stateCount;
    return id;
}
bool AnimBlendTree::RemoveState(AnimStateId id) {
    auto it = std::find_if(m_impl->states.begin(), m_impl->states.end(),
        [id](const AnimStateEntry& s){ return s.id == id; });
    if (it == m_impl->states.end()) return false;
    m_impl->states.erase(it);
    if (m_impl->stats.stateCount > 0) --m_impl->stats.stateCount;
    return true;
}
bool AnimBlendTree::HasState(AnimStateId id) const {
    return m_impl->FindState(id) != nullptr;
}
AnimStateDesc AnimBlendTree::GetStateDesc(AnimStateId id) const {
    const auto* s = m_impl->FindState(id);
    return s ? s->desc : AnimStateDesc{};
}

// ── Transition management ─────────────────────────────────────────────────
TransitionId AnimBlendTree::AddTransition(const TransitionDesc& t) {
    TransitionId id = m_impl->nextTransId++;
    m_impl->transitions.push_back({id, t});
    ++m_impl->stats.transitionCount;
    return id;
}
bool AnimBlendTree::RemoveTransition(TransitionId id) {
    auto it = std::find_if(m_impl->transitions.begin(), m_impl->transitions.end(),
        [id](const TransitionEntry& t){ return t.id == id; });
    if (it == m_impl->transitions.end()) return false;
    m_impl->transitions.erase(it);
    if (m_impl->stats.transitionCount > 0) --m_impl->stats.transitionCount;
    return true;
}
bool AnimBlendTree::HasTransition(TransitionId id) const {
    return m_impl->FindTransition(id) != nullptr;
}

// ── Entry and start ───────────────────────────────────────────────────────
void AnimBlendTree::SetEntryState(AnimStateId id) { m_impl->entryState = id; }

void AnimBlendTree::Start() {
    m_impl->currentState    = m_impl->entryState;
    m_impl->activeTransition = kInvalidTransitionId;
    m_impl->stateTime       = 0.0f;
    m_impl->transTime       = 0.0f;
    m_impl->blendWeight     = 0.0f;
    if (m_impl->onEnterCb && m_impl->currentState != kInvalidAnimStateId)
        m_impl->onEnterCb(m_impl->currentState);
}

// ── Update ────────────────────────────────────────────────────────────────
void AnimBlendTree::Update(float dt) {
    ++m_impl->stats.updateCount;
    m_impl->stateTime += dt;

    // If in a transition, advance blend
    if (m_impl->activeTransition != kInvalidTransitionId) {
        auto* te = m_impl->FindTransition(m_impl->activeTransition);
        if (te) {
            m_impl->transTime += dt;
            float dur = te->desc.durationSec > 0.0f ? te->desc.durationSec : 1e-4f;
            m_impl->blendWeight = std::min(m_impl->transTime / dur, 1.0f);
            if (m_impl->blendWeight >= 1.0f) {
                // Transition complete
                AnimStateId toState = te->desc.toState;
                if (m_impl->onExitCb)  m_impl->onExitCb(m_impl->currentState);
                m_impl->currentState     = toState;
                m_impl->activeTransition = kInvalidTransitionId;
                m_impl->stateTime        = 0.0f;
                m_impl->transTime        = 0.0f;
                m_impl->blendWeight      = 0.0f;
                if (m_impl->onEnterCb && toState != kInvalidAnimStateId)
                    m_impl->onEnterCb(toState);
            }
        }
        m_impl->stats.currentNormTime = m_impl->stateTime;
        return;
    }

    // Evaluate transitions (sorted by priority desc)
    std::vector<TransitionEntry*> candidates;
    for (auto& te : m_impl->transitions) {
        if (te.desc.fromState == kInvalidAnimStateId || te.desc.fromState == m_impl->currentState)
            candidates.push_back(&te);
    }
    std::stable_sort(candidates.begin(), candidates.end(),
        [](const TransitionEntry* a, const TransitionEntry* b){
            return a->desc.priority > b->desc.priority;
        });

    for (auto* te : candidates) {
        if (!m_impl->EvalConditions(te->desc)) continue;
        if (te->desc.hasExitTime) {
            const auto* cs = m_impl->FindState(m_impl->currentState);
            if (!cs) continue;
            // Simple: assume 1s clip length for norm time check
            float normTime = std::fmod(m_impl->stateTime, 1.0f);
            if (normTime < te->desc.exitTime) continue;
        }
        // Begin transition
        if (m_impl->onTransBeginCb)
            m_impl->onTransBeginCb(te->id, m_impl->currentState, te->desc.toState);
        m_impl->activeTransition = te->id;
        m_impl->transTime        = 0.0f;
        m_impl->blendWeight      = 0.0f;
        break;
    }
    m_impl->stats.currentNormTime = m_impl->stateTime;
}

// ── State queries ─────────────────────────────────────────────────────────
AnimStateId  AnimBlendTree::GetCurrentState()     const { return m_impl->currentState; }
TransitionId AnimBlendTree::GetActiveTransition() const { return m_impl->activeTransition; }

float AnimBlendTree::GetBlendWeight(AnimStateId id) const {
    if (m_impl->activeTransition == kInvalidTransitionId) {
        return (id == m_impl->currentState) ? 1.0f : 0.0f;
    }
    auto* te = m_impl->FindTransition(m_impl->activeTransition);
    if (!te) return 0.0f;
    if (id == te->desc.toState)   return m_impl->blendWeight;
    if (id == m_impl->currentState) return 1.0f - m_impl->blendWeight;
    return 0.0f;
}

float AnimBlendTree::GetNormalisedTime() const { return m_impl->stats.currentNormTime; }

void AnimBlendTree::ForceTransition(AnimStateId toState) {
    if (m_impl->onExitCb && m_impl->currentState != kInvalidAnimStateId)
        m_impl->onExitCb(m_impl->currentState);
    m_impl->currentState     = toState;
    m_impl->activeTransition = kInvalidTransitionId;
    m_impl->stateTime        = 0.0f;
    m_impl->transTime        = 0.0f;
    m_impl->blendWeight      = 0.0f;
    if (m_impl->onEnterCb && toState != kInvalidAnimStateId)
        m_impl->onEnterCb(toState);
}

// ── Parameters ────────────────────────────────────────────────────────────
void    AnimBlendTree::SetFloat(const std::string& n, float v)    { m_impl->params[n] = v; }
void    AnimBlendTree::SetInt  (const std::string& n, int32_t v)  { m_impl->params[n] = v; }
void    AnimBlendTree::SetBool (const std::string& n, bool v)     { m_impl->params[n] = v; }

float   AnimBlendTree::GetFloat(const std::string& n) const {
    auto it = m_impl->params.find(n);
    return (it != m_impl->params.end() && std::holds_alternative<float>(it->second))
        ? std::get<float>(it->second) : 0.0f;
}
int32_t AnimBlendTree::GetInt(const std::string& n) const {
    auto it = m_impl->params.find(n);
    return (it != m_impl->params.end() && std::holds_alternative<int32_t>(it->second))
        ? std::get<int32_t>(it->second) : 0;
}
bool    AnimBlendTree::GetBool(const std::string& n) const {
    auto it = m_impl->params.find(n);
    return (it != m_impl->params.end() && std::holds_alternative<bool>(it->second))
        ? std::get<bool>(it->second) : false;
}

// ── Callbacks ─────────────────────────────────────────────────────────────
void AnimBlendTree::OnStateEnter    (StateEventCb cb)      { m_impl->onEnterCb     = std::move(cb); }
void AnimBlendTree::OnStateExit     (StateEventCb cb)      { m_impl->onExitCb      = std::move(cb); }
void AnimBlendTree::OnTransitionBegin(TransitionEventCb cb){ m_impl->onTransBeginCb= std::move(cb); }

// ── Stats / reset ─────────────────────────────────────────────────────────
BlendTreeStats AnimBlendTree::GetStats() const { return m_impl->stats; }

void AnimBlendTree::Reset() {
    m_impl->states.clear();
    m_impl->transitions.clear();
    m_impl->params.clear();
    m_impl->entryState       = kInvalidAnimStateId;
    m_impl->currentState     = kInvalidAnimStateId;
    m_impl->activeTransition = kInvalidTransitionId;
    m_impl->stateTime        = 0.0f;
    m_impl->transTime        = 0.0f;
    m_impl->blendWeight      = 0.0f;
    m_impl->stats            = {};
}

} // namespace Engine
