#pragma once
/**
 * @file AnimationGraph.h
 * @brief State-machine-driven animation blend graph with conditions, clips, blend spaces.
 *
 * Features:
 *   - CreateGraph(id) / DestroyGraph(id)
 *   - AddState(graphId, stateId, clipName, loop) → bool
 *   - AddTransition(graphId, fromState, toState, condVar, threshold, crossfadeSec)
 *   - SetParameter(graphId, name, value): float/bool/int parameters
 *   - GetParameter(graphId, name) → float
 *   - AddBlendSpace1D(graphId, stateId, axisParam): map clips to 1D axis
 *   - AddBlendSpace2D(graphId, stateId, axisX, axisY): 2D blend space
 *   - AddClipToBlendSpace(graphId, stateId, x, y, clipName)
 *   - SetEntryState(graphId, stateId)
 *   - Tick(graphId, dt): evaluate conditions, blend clips
 *   - GetCurrentState(graphId) → uint32_t
 *   - GetCurrentClipTime(graphId) → float
 *   - GetBlendWeight(graphId, stateId) → float: cross-fade weight [0,1]
 *   - SetOnStateEnter(graphId, stateId, cb) / SetOnStateExit(graphId, stateId, cb)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

class AnimationGraph {
public:
    AnimationGraph();
    ~AnimationGraph();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Graph management
    void CreateGraph (uint32_t id);
    void DestroyGraph(uint32_t id);

    // States
    bool AddState    (uint32_t graphId, uint32_t stateId,
                      const std::string& clipName, bool loop = true);
    void SetEntryState(uint32_t graphId, uint32_t stateId);

    // Transitions
    void AddTransition(uint32_t graphId, uint32_t fromState, uint32_t toState,
                       const std::string& condVar, float threshold,
                       float crossfadeSec = 0.2f);

    // Parameters
    void  SetParameter(uint32_t graphId, const std::string& name, float value);
    float GetParameter(uint32_t graphId, const std::string& name) const;

    // Blend spaces
    void AddBlendSpace1D(uint32_t graphId, uint32_t stateId,
                         const std::string& axisParam);
    void AddBlendSpace2D(uint32_t graphId, uint32_t stateId,
                         const std::string& axisX, const std::string& axisY);
    void AddClipToBlendSpace(uint32_t graphId, uint32_t stateId,
                              float x, float y, const std::string& clipName);

    // Per-frame
    void Tick(uint32_t graphId, float dt);

    // Query
    uint32_t GetCurrentState   (uint32_t graphId) const;
    float    GetCurrentClipTime(uint32_t graphId) const;
    float    GetBlendWeight    (uint32_t graphId, uint32_t stateId) const;

    // Callbacks
    void SetOnStateEnter(uint32_t graphId, uint32_t stateId,
                         std::function<void()> cb);
    void SetOnStateExit (uint32_t graphId, uint32_t stateId,
                         std::function<void()> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
