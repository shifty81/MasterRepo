#pragma once
/**
 * @file PerceptionSystem.h
 * @brief AI sight/hearing/touch perception with stimuli registration and range queries.
 *
 * Features:
 *   - RegisterAgent(id, sightRange, sightAngleDeg, hearingRange): create a perceiver
 *   - UnregisterAgent(id)
 *   - RegisterStimulus(stimId, type, pos, strength): add/update a world stimulus
 *   - RemoveStimulus(stimId)
 *   - SetSightBlocker(fn): callback(origin, target) → bool (line-of-sight query)
 *   - Update(dt): age existing percepts, evaluate new detections, fire callbacks
 *   - GetPercepts(agentId) → Percept[]: current detected stimuli with age/strength
 *   - IsAware(agentId, stimId) → bool
 *   - SetOnPerceived(agentId, cb): fired when a new stimulus enters perception
 *   - SetOnLost(agentId, cb): fired when a stimulus ages out of perception
 *   - SetForgetTime(seconds): duration for a percept to age before being dropped
 *   - SetAgentTeam(agentId, teamId): skip self-perception within same team
 *   - Stimulus types: Visual, Audio, Touch, Custom
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class StimulusType : uint8_t { Visual, Audio, Touch, Custom };

struct PSVec3 { float x,y,z; };

struct Percept {
    uint32_t     stimId;
    StimulusType type;
    PSVec3       lastKnownPos;
    float        strength;
    float        age;         ///< seconds since last detected
    bool         isFresh;     ///< detected this frame
};

class PerceptionSystem {
public:
    PerceptionSystem();
    ~PerceptionSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Agent management
    void RegisterAgent  (uint32_t id, float sightRange, float sightAngleDeg,
                         float hearingRange);
    void UnregisterAgent(uint32_t id);
    void SetAgentTeam   (uint32_t agentId, uint32_t teamId);
    void SetAgentPos    (uint32_t agentId, PSVec3 pos, PSVec3 forwardDir);

    // Stimulus management
    void RegisterStimulus(uint32_t stimId, StimulusType type,
                          PSVec3 pos, float strength=1.f);
    void UpdateStimulus  (uint32_t stimId, PSVec3 pos, float strength);
    void RemoveStimulus  (uint32_t stimId);

    // LoS callback
    void SetSightBlocker(std::function<bool(PSVec3 origin, PSVec3 target)> fn);

    // Per-frame update
    void Update(float dt);

    // Query
    std::vector<Percept> GetPercepts(uint32_t agentId) const;
    bool                 IsAware    (uint32_t agentId, uint32_t stimId) const;

    // Config
    void SetForgetTime(float seconds);

    // Callbacks
    void SetOnPerceived(uint32_t agentId, std::function<void(const Percept&)> cb);
    void SetOnLost     (uint32_t agentId, std::function<void(uint32_t stimId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
