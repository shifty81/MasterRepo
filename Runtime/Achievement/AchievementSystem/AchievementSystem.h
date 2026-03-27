#pragma once
/**
 * @file AchievementSystem.h
 * @brief Achievement registry, progress tracking, unlock conditions, callbacks.
 *
 * Features:
 *   - Achievement definitions: id, title, description, icon, hidden, points
 *   - Progress-based achievements (0 to goal count) or single-unlock (binary)
 *   - Condition evaluation: arbitrary predicate or stat-threshold
 *   - Stat integration: named counters that auto-check achievements when incremented
 *   - Per-achievement unlock callback
 *   - Global on-unlock callback
 *   - Persistent state: Serialize/Deserialize to JSON blob
 *   - Category groups: filter/query by category
 *   - Date/time of unlock (timestamp)
 *   - Platform unlock dispatch (pluggable backend)
 *   - Total unlocked / total points / completion percentage
 *
 * Typical usage:
 * @code
 *   AchievementSystem as;
 *   as.Init();
 *   as.Register({"kill100", "Century Kill", "Kill 100 enemies", "", false, 50, 100});
 *   as.SetOnUnlock([](const AchievementDef& def){ ShowToast(def.title); });
 *   as.IncrementStat("enemiesKilled");
 *   as.BindStatToAchievement("enemiesKilled", "kill100", 100);
 *   as.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct AchievementDef {
    std::string id;
    std::string title;
    std::string description;
    std::string iconId;
    bool        hidden{false};
    uint32_t    points{10};
    uint32_t    progressGoal{1};  ///< 1 = single-unlock
    std::string category;
};

struct AchievementState {
    std::string  id;
    uint32_t     progress{0};
    bool         unlocked{false};
    uint64_t     unlockTimestamp{0};
};

class AchievementSystem {
public:
    AchievementSystem();
    ~AchievementSystem();

    void Init();
    void Shutdown();
    void Tick(float dt);

    // Definitions
    void Register  (const AchievementDef& def);
    bool Has       (const std::string& id) const;
    const AchievementDef* Get(const std::string& id) const;
    std::vector<AchievementDef> GetAll()                        const;
    std::vector<AchievementDef> GetByCategory(const std::string& cat) const;

    // Progress
    void     Unlock       (const std::string& id);
    void     SetProgress  (const std::string& id, uint32_t progress);
    void     AddProgress  (const std::string& id, uint32_t delta=1);
    bool     IsUnlocked   (const std::string& id) const;
    uint32_t GetProgress  (const std::string& id) const;
    const AchievementState* GetState(const std::string& id) const;

    // Stat integration
    void     IncrementStat       (const std::string& statKey, uint32_t delta=1);
    void     SetStat             (const std::string& statKey, uint32_t value);
    uint32_t GetStat             (const std::string& statKey) const;
    void     BindStatToAchievement(const std::string& statKey, const std::string& achievId,
                                   uint32_t threshold=1);

    // Summary
    uint32_t TotalUnlocked()    const;
    uint32_t TotalPoints()      const;
    uint32_t EarnedPoints()     const;
    float    CompletionPct()    const;

    // Callbacks
    using UnlockCb = std::function<void(const AchievementDef&)>;
    void SetOnUnlock(UnlockCb cb);
    void SetOnProgress(std::function<void(const AchievementDef&, uint32_t progress)> cb);

    // Persistence
    std::string Serialize()             const;
    bool        Deserialize(const std::string& json);

    // Platform dispatch
    using PlatformUnlockFn = std::function<void(const std::string& platformId)>;
    void SetPlatformUnlockFn(PlatformUnlockFn fn);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
