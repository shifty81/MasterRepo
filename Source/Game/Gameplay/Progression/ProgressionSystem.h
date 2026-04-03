#pragma once
#include <cstdint>
#include <functional>

namespace NF::Game::Gameplay {

/// @brief Skills that can be unlocked through level progression.
enum class SkillUnlock : uint8_t {
    Tier2Mining   = 0,  ///< Unlocked at level 3.
    AdvancedBuild = 1,  ///< Unlocked at level 5.
    Tier3Mining   = 2,  ///< Unlocked at level 7.
    CombatMaster  = 3,  ///< Unlocked at level 10.
    kCount        = 4,
};

/// @brief XP + level + skill-unlock progression tracker.
///
/// XP required to reach level N (1-indexed): kXpPerLevel * (N-1) * N / 2 cumulative.
/// Example: level 2 = 100 XP total, level 3 = 300 XP total (100+200), etc.
/// Skills unlock at specific levels (see @c SkillUnlock).
class ProgressionSystem {
public:
    /// @brief XP required for each successive level.
    static constexpr uint32_t kXpPerLevel = 100;

    ProgressionSystem() = default;

    // -------------------------------------------------------------------------
    // XP & Levels
    // -------------------------------------------------------------------------

    /// @brief Award @p amount of experience points.
    ///
    /// Triggers the level-up callback whenever a new level is reached.
    void AddXP(uint32_t amount);

    /// @brief Total accumulated XP.
    [[nodiscard]] uint32_t GetXP()    const noexcept { return m_TotalXP; }

    /// @brief Current level (starts at 1).
    [[nodiscard]] int      GetLevel() const noexcept { return m_Level; }

    /// @brief XP still needed to advance to the next level (0 when max level).
    [[nodiscard]] uint32_t GetXPForNextLevel() const noexcept;

    /// @brief Total XP required to reach @p level from zero.
    [[nodiscard]] static uint32_t XPThreshold(int level) noexcept;

    // -------------------------------------------------------------------------
    // Skills
    // -------------------------------------------------------------------------

    /// @brief Returns true if @p skill has been unlocked.
    [[nodiscard]] bool IsUnlocked(SkillUnlock skill) const noexcept;

    // -------------------------------------------------------------------------
    // Callbacks
    // -------------------------------------------------------------------------

    /// @brief Set a callback fired whenever the player gains a level.
    ///        Called with the new level value.
    void SetLevelUpCallback(std::function<void(int)> cb) { m_LevelUpCb = std::move(cb); }

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// @brief Reset to initial state (level 1, 0 XP).
    void Reset() noexcept;

private:
    uint32_t m_TotalXP{0};
    int      m_Level{1};

    std::function<void(int)> m_LevelUpCb;

    /// @brief Re-evaluate level and fire callbacks for each new level gained.
    void RecalculateLevel();

    /// @brief Return the unlock-level threshold for a given skill.
    [[nodiscard]] static int UnlockLevel(SkillUnlock skill) noexcept;
};

} // namespace NF::Game::Gameplay
