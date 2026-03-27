#pragma once
/**
 * @file ObjectiveTracker.h
 * @brief Multi-objective quest tracker: register, progress, complete, chain, UI feed.
 *
 * Features:
 *   - RegisterObjective(id, title, description, required) → bool
 *   - SetProgress(id, current) / IncrementProgress(id, delta)
 *   - GetProgress(id) → float [0, required]
 *   - GetRequired(id) → float
 *   - IsComplete(id) → bool
 *   - CompleteObjective(id): force-complete regardless of progress
 *   - FailObjective(id)
 *   - GetState(id) → ObjectiveState {Inactive, Active, Complete, Failed}
 *   - Activate(id) / Deactivate(id)
 *   - SetOnProgress(id, cb) / SetOnComplete(id, cb) / SetOnFail(id, cb)
 *   - AddChain(fromId, toId): auto-activate toId when fromId completes
 *   - GetActiveObjectiveIds(out[]) → uint32_t
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class ObjectiveState : uint8_t { Inactive, Active, Complete, Failed };

struct ObjectiveDef {
    uint32_t    id;
    std::string title;
    std::string description;
    float       required{1.f};
};

class ObjectiveTracker {
public:
    ObjectiveTracker();
    ~ObjectiveTracker();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Registration
    bool RegisterObjective(const ObjectiveDef& def);

    // Lifecycle
    void Activate  (uint32_t id);
    void Deactivate(uint32_t id);

    // Progress
    void  SetProgress      (uint32_t id, float current);
    void  IncrementProgress(uint32_t id, float delta);
    float GetProgress      (uint32_t id) const;
    float GetRequired      (uint32_t id) const;
    bool  IsComplete       (uint32_t id) const;

    // State
    void           CompleteObjective(uint32_t id);
    void           FailObjective    (uint32_t id);
    ObjectiveState GetState         (uint32_t id) const;

    // Chains
    void AddChain(uint32_t fromId, uint32_t toId);

    // Query
    uint32_t GetActiveObjectiveIds(std::vector<uint32_t>& out) const;

    // Callbacks
    void SetOnProgress(uint32_t id,
                       std::function<void(float current, float required)> cb);
    void SetOnComplete(uint32_t id, std::function<void()> cb);
    void SetOnFail    (uint32_t id, std::function<void()> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
