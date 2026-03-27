#pragma once
/**
 * @file AnimationStateMachine.h
 * @brief Layer-based animation state machine: states, transitions, blend trees, parameters.
 *
 * Features:
 *   - AddState(id, clipName, speed, looping): register an animation state
 *   - AddTransition(fromId, toId, conditionFn, blendTime): connect two states
 *   - SetDefaultState(id)
 *   - Play(agentId, stateId, blendTime): manually jump to a state
 *   - Tick(agentId, dt) → AnimFrame: evaluate current state(s), returns blend result
 *   - SetBool(agentId, name, v) / SetFloat(agentId, name, v) / SetInt(agentId, name, v)
 *   - GetBool(agentId, name) / GetFloat / GetInt
 *   - GetCurrentState(agentId) → stateId
 *   - GetNormalizedTime(agentId) → float [0,1]
 *   - AddLayer(layerId, maskBoneIds[]): additive layer over base layer
 *   - SetLayerWeight(layerId, weight)
 *   - SetOnEnter(stateId, cb) / SetOnExit(stateId, cb)
 *   - SetOnTransition(fromId, toId, cb)
 *   - Reset(agentId) / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine {

struct AnimFrame {
    std::string clipName;
    float       normalizedTime{0};
    float       blendWeight{1};
    uint32_t    stateId{0};
};

class AnimationStateMachine {
public:
    AnimationStateMachine();
    ~AnimationStateMachine();

    void Init    ();
    void Shutdown();

    // State/transition registration
    void AddState     (uint32_t id, const std::string& clipName,
                       float speed=1.f, bool looping=true);
    void AddTransition(uint32_t fromId, uint32_t toId,
                       std::function<bool(uint32_t agentId)> condFn,
                       float blendTime=0.2f);
    void SetDefaultState(uint32_t id);

    // Per-agent control
    void      Play            (uint32_t agentId, uint32_t stateId, float blendTime=0.2f);
    AnimFrame Tick            (uint32_t agentId, float dt);
    void      Reset           (uint32_t agentId);
    uint32_t  GetCurrentState (uint32_t agentId) const;
    float     GetNormalizedTime(uint32_t agentId) const;

    // Parameters
    void  SetBool (uint32_t agentId, const std::string& name, bool v);
    void  SetFloat(uint32_t agentId, const std::string& name, float v);
    void  SetInt  (uint32_t agentId, const std::string& name, int32_t v);
    bool  GetBool (uint32_t agentId, const std::string& name) const;
    float GetFloat(uint32_t agentId, const std::string& name) const;
    int32_t GetInt(uint32_t agentId, const std::string& name) const;

    // Layers
    void AddLayer      (uint32_t layerId, const std::vector<uint32_t>& maskBoneIds);
    void SetLayerWeight(uint32_t layerId, float weight);

    // Callbacks
    void SetOnEnter     (uint32_t stateId, std::function<void(uint32_t agentId)> cb);
    void SetOnExit      (uint32_t stateId, std::function<void(uint32_t agentId)> cb);
    void SetOnTransition(uint32_t fromId, uint32_t toId,
                         std::function<void(uint32_t agentId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
