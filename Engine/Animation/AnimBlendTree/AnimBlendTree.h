#pragma once
/**
 * @file AnimBlendTree.h
 * @brief Animation blend tree with state machine, transition conditions, and weighted blending.
 *
 * AnimBlendTree drives character animation by managing a set of animation
 * states and the blending transitions between them:
 *
 *   AnimState: name, clip name, playback speed, loop flag, root-motion flag.
 *   BlendMode: Override, Additive, Lerp.
 *   TransitionCondition: parameter name, op (Equal/Greater/Less/NotEqual/Bool), threshold.
 *   Transition: from/to state ids, condition list (ALL must be true), duration,
 *               blend mode, has_exit_time flag, exit_time normalised (0–1).
 *
 *   AnimBlendTree:
 *     - AddState(desc): register a state; returns stable AnimStateId.
 *     - RemoveState(id).
 *     - AddTransition(t): register a transition; returns TransitionId.
 *     - RemoveTransition(id).
 *     - SetEntryState(id): state played on Start().
 *     - Start(): enter the entry state.
 *     - Update(dt): advance time, evaluate transition conditions, blend.
 *     - GetCurrentState() / GetActiveTransition().
 *     - Parameters: SetFloat / SetInt / SetBool / GetFloat / GetInt / GetBool.
 *     - GetBlendWeight(stateId): weight of a state in the current blend [0,1].
 *     - ForceTransition(toStateId): immediate state switch.
 *     - OnStateEnter(cb) / OnStateExit(cb) / OnTransitionBegin(cb) callbacks.
 *     - BlendTreeStats: stateCount, transitionCount, updateCount, currentNormTime.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace Engine {

// ── Typed handles ─────────────────────────────────────────────────────────
using AnimStateId    = uint32_t;
using TransitionId   = uint32_t;
static constexpr AnimStateId  kInvalidAnimStateId  = 0;
static constexpr TransitionId kInvalidTransitionId = 0;

// ── Blend mode ────────────────────────────────────────────────────────────
enum class BlendMode : uint8_t { Override, Additive, Lerp };

// ── Condition operator ────────────────────────────────────────────────────
enum class CondOp : uint8_t { Equal, NotEqual, Greater, Less, Bool };

// ── Transition condition ──────────────────────────────────────────────────
struct TransitionCondition {
    std::string param;          ///< Parameter name
    CondOp      op{CondOp::Bool};
    float       threshold{0.0f};
    bool        boolValue{true}; ///< Used when op == Bool
};

// ── Animation state descriptor ────────────────────────────────────────────
struct AnimStateDesc {
    std::string name;
    std::string clipName;        ///< Animation clip asset name
    float       speed{1.0f};     ///< Playback speed multiplier
    bool        loop{true};
    bool        applyRootMotion{false};
};

// ── Transition descriptor ─────────────────────────────────────────────────
struct TransitionDesc {
    AnimStateId                   fromState{kInvalidAnimStateId}; ///< kInvalidAnimStateId = "Any"
    AnimStateId                   toState{kInvalidAnimStateId};
    std::vector<TransitionCondition> conditions;
    float                         durationSec{0.2f};   ///< Blend duration
    BlendMode                     blendMode{BlendMode::Lerp};
    bool                          hasExitTime{false};
    float                         exitTime{1.0f};      ///< Normalised (0–1)
    int32_t                       priority{0};         ///< Higher = evaluated first
};

// ── Stats ─────────────────────────────────────────────────────────────────
struct BlendTreeStats {
    uint32_t stateCount{0};
    uint32_t transitionCount{0};
    uint64_t updateCount{0};
    float    currentNormTime{0.0f}; ///< Normalised time in current state [0,1]
};

// ── Callbacks ─────────────────────────────────────────────────────────────
using StateEventCb      = std::function<void(AnimStateId)>;
using TransitionEventCb = std::function<void(TransitionId, AnimStateId from, AnimStateId to)>;

// ── AnimBlendTree ─────────────────────────────────────────────────────────
class AnimBlendTree {
public:
    AnimBlendTree();
    ~AnimBlendTree();

    AnimBlendTree(const AnimBlendTree&) = delete;
    AnimBlendTree& operator=(const AnimBlendTree&) = delete;

    // ── state management ──────────────────────────────────────
    AnimStateId  AddState(const AnimStateDesc& desc);
    bool         RemoveState(AnimStateId id);
    bool         HasState(AnimStateId id) const;
    AnimStateDesc GetStateDesc(AnimStateId id) const;

    // ── transition management ─────────────────────────────────
    TransitionId  AddTransition(const TransitionDesc& t);
    bool          RemoveTransition(TransitionId id);
    bool          HasTransition(TransitionId id) const;

    // ── entry and start ───────────────────────────────────────
    void SetEntryState(AnimStateId id);
    void Start();

    // ── update ────────────────────────────────────────────────
    void Update(float dt);

    // ── state queries ─────────────────────────────────────────
    AnimStateId    GetCurrentState() const;
    TransitionId   GetActiveTransition() const;
    float          GetBlendWeight(AnimStateId id) const;
    float          GetNormalisedTime() const;

    // ── forced transition ─────────────────────────────────────
    void ForceTransition(AnimStateId toState);

    // ── parameters ────────────────────────────────────────────
    void  SetFloat(const std::string& name, float v);
    void  SetInt  (const std::string& name, int32_t v);
    void  SetBool (const std::string& name, bool v);
    float   GetFloat(const std::string& name) const;
    int32_t GetInt  (const std::string& name) const;
    bool    GetBool (const std::string& name) const;

    // ── callbacks ─────────────────────────────────────────────
    void OnStateEnter   (StateEventCb cb);
    void OnStateExit    (StateEventCb cb);
    void OnTransitionBegin(TransitionEventCb cb);

    // ── stats ─────────────────────────────────────────────────
    BlendTreeStats GetStats() const;
    void           Reset();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Engine
