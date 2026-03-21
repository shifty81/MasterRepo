#pragma once
/**
 * @file AchievementSystem.h
 * @brief Achievement definition, progress tracking, and unlock system.
 *
 * Features:
 *   - AchievementDef: condition type (single/cumulative/streak), target,
 *     icon, display name, hidden flag, reward data
 *   - AchievementState: per-player progress tracking (current/total, unlocked)
 *   - AchievementSystem: registry + runtime driver; Update(key, delta) advances
 *     matching achievements; OnUnlock callback fires once per unlock event
 *   - Serialise/deserialise progress to JSON-style flat record for save/load
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Runtime::Gameplay {

// ── Condition type ────────────────────────────────────────────────────────
enum class AchievementCondition : uint8_t {
    /// Accumulated total reaches threshold (e.g. "kill 100 enemies")
    Cumulative,
    /// Single event fires with value >= threshold (e.g. "reach level 10")
    SingleEvent,
    /// Track N consecutive successes without reset (e.g. "5 kills in a row")
    Streak,
    /// Timed: accumulate seconds performing an action
    TimedAccumulation,
};

// ── Definition ────────────────────────────────────────────────────────────
struct AchievementDef {
    std::string          id;
    std::string          name;
    std::string          description;
    std::string          iconKey;          ///< Asset key for icon texture
    AchievementCondition condition{AchievementCondition::Cumulative};
    std::string          eventKey;         ///< Event name that drives this achievement
    float                targetValue{1.0f};///< Threshold to unlock
    bool                 hidden{false};    ///< Not shown until unlocked
    int32_t              rewardXP{0};
    std::string          rewardItemId;     ///< Optional item grant on unlock
    int32_t              sortOrder{0};     ///< For UI ordering
};

// ── Runtime state ─────────────────────────────────────────────────────────
struct AchievementState {
    std::string id;
    float       progress{0.0f};
    float       target{1.0f};
    bool        unlocked{false};
    uint64_t    unlockedAtMs{0};  ///< Timestamp of unlock (ms since epoch)
    uint32_t    streakCurrent{0};
    uint32_t    streakBest{0};

    float Fraction() const { return target > 0.0f ? progress / target : 0.0f; }
    bool  IsComplete() const { return unlocked; }
};

// ── Unlock event ─────────────────────────────────────────────────────────
struct UnlockEvent {
    std::string id;
    std::string name;
    int32_t     rewardXP{0};
    std::string rewardItemId;
    uint64_t    timestampMs{0};
};

using UnlockCallback     = std::function<void(const UnlockEvent&)>;
using ProgressCallback   = std::function<void(const std::string& id, float progress, float target)>;

// ── System ────────────────────────────────────────────────────────────────
class AchievementSystem {
public:
    AchievementSystem();
    ~AchievementSystem();

    // ── registration ─────────────────────────────────────────
    void Register(const AchievementDef& def);
    bool IsRegistered(const std::string& id) const;
    const AchievementDef* GetDef(const std::string& id) const;
    std::vector<std::string> AllIds() const;
    size_t Count() const;

    // ── event-driven update ──────────────────────────────────
    /// Advance all achievements listening to @p eventKey by @p delta.
    /// For streak achievements, @p delta > 0 extends the streak; delta == 0 resets it.
    void Update(const std::string& eventKey, float delta = 1.0f);

    /// Directly unlock an achievement (e.g. story milestone).
    void ForceUnlock(const std::string& id);

    /// Reset streak for a specific key (e.g. player died).
    void ResetStreak(const std::string& eventKey);

    // ── state query ───────────────────────────────────────────
    const AchievementState* GetState(const std::string& id) const;
    bool IsUnlocked(const std::string& id) const;
    float GetProgress(const std::string& id) const;

    std::vector<AchievementState> UnlockedAchievements() const;
    std::vector<AchievementState> InProgress() const;
    size_t UnlockedCount() const;

    // ── callbacks ─────────────────────────────────────────────
    void OnUnlock(UnlockCallback cb);
    void OnProgress(ProgressCallback cb);

    // ── persistence ───────────────────────────────────────────
    /// Flat key=value export suitable for JSON/INI embedding.
    std::unordered_map<std::string, std::string> ExportState() const;
    /// Restore from a previously exported map.
    void ImportState(const std::unordered_map<std::string, std::string>& data);

    void Reset();  ///< Wipe all progress (new game)

private:
    struct Impl;
    Impl* m_impl{nullptr};

    void unlock(AchievementState& st, const AchievementDef& def);
};

} // namespace Runtime::Gameplay
