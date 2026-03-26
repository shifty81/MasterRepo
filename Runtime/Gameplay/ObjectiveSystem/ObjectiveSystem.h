#pragma once
/**
 * @file ObjectiveSystem.h
 * @brief Quest / mission objective tracker with conditions, rewards, and HUD integration.
 *
 * An Objective is a named task with:
 *   - A set of completion conditions (kill N, reach location, collect item…)
 *   - Optional sub-objectives
 *   - Rewards on completion (XP, items, unlock)
 *   - States: Inactive → Active → Completed / Failed
 *
 * Supports:
 *   - Multiple simultaneous active objectives
 *   - Optional time limits with failure on expiry
 *   - Objective chaining (auto-activate next on completion)
 *   - Persistence: save/load active state
 *
 * Typical usage:
 * @code
 *   ObjectiveSystem os;
 *   os.Init();
 *   ObjectiveDesc d;
 *   d.id     = "kill_wolves";
 *   d.title  = "Hunt the Pack";
 *   d.description = "Kill 5 wolves in Darkwood";
 *   d.targetCount = 5;
 *   os.RegisterObjective(d);
 *   os.ActivateObjective("kill_wolves");
 *   os.IncrementProgress("kill_wolves");  // called each kill
 *   bool done = os.IsCompleted("kill_wolves");
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class ObjectiveState : uint8_t { Inactive=0, Active, Completed, Failed };

struct ObjectiveReward {
    int32_t     xp{0};
    int32_t     gold{0};
    std::string itemId;
    std::string unlockId;
};

struct ObjectiveDesc {
    std::string   id;
    std::string   title;
    std::string   description;
    std::string   locKey;
    int32_t       targetCount{1};
    float         timeLimit{-1.f};   ///< -1 = no limit
    std::string   nextObjectiveId;   ///< auto-activate on completion
    ObjectiveReward reward;
    std::vector<std::string> subObjectiveIds;
    bool          optional{false};
    bool          hidden{false};     ///< show only when active
};

struct ObjectiveInstance {
    ObjectiveDesc desc;
    ObjectiveState state{ObjectiveState::Inactive};
    int32_t        progress{0};
    float          elapsed{0.f};
};

class ObjectiveSystem {
public:
    ObjectiveSystem();
    ~ObjectiveSystem();

    void Init();
    void Shutdown();

    // Registration & management
    void RegisterObjective(const ObjectiveDesc& desc);
    void UnregisterObjective(const std::string& id);
    bool HasObjective(const std::string& id) const;
    ObjectiveInstance GetObjective(const std::string& id) const;
    std::vector<ObjectiveInstance> AllObjectives() const;
    std::vector<ObjectiveInstance> ActiveObjectives() const;

    // State control
    void ActivateObjective(const std::string& id);
    void CompleteObjective(const std::string& id);
    void FailObjective(const std::string& id);
    void ResetObjective(const std::string& id);

    // Progress
    void IncrementProgress(const std::string& id, int32_t amount = 1);
    void SetProgress(const std::string& id, int32_t value);

    bool IsCompleted(const std::string& id) const;
    bool IsFailed(const std::string& id)    const;
    bool IsActive(const std::string& id)    const;

    // Tick (for time-limited objectives)
    void Update(float dt);

    // Serialization
    bool SaveState(const std::string& path) const;
    bool LoadState(const std::string& path);

    // Callbacks
    void OnActivated(std::function<void(const std::string& id)> cb);
    void OnCompleted(std::function<void(const std::string& id)> cb);
    void OnFailed(std::function<void(const std::string& id)> cb);
    void OnProgressChanged(std::function<void(const std::string& id, int32_t progress, int32_t target)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
