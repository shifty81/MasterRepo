#include "Game/Gameplay/Progression/ProgressionSystem.h"
#include <array>

namespace NF::Game::Gameplay {

// static
uint32_t ProgressionSystem::XPThreshold(int level) noexcept {
    if (level <= 1) return 0;
    // Total XP needed = sum of (i * kXpPerLevel) for i in 1..(level-1)
    // = kXpPerLevel * (level-1)*level/2
    const uint32_t n = static_cast<uint32_t>(level - 1);
    return kXpPerLevel * n * (n + 1) / 2;
}

// static
int ProgressionSystem::UnlockLevel(SkillUnlock skill) noexcept {
    static constexpr std::array<int, static_cast<int>(SkillUnlock::kCount)> kLevels = {
        3,  // Tier2Mining
        5,  // AdvancedBuild
        7,  // Tier3Mining
        10, // CombatMaster
    };
    const int idx = static_cast<int>(skill);
    if (idx < 0 || idx >= static_cast<int>(SkillUnlock::kCount)) return 99;
    return kLevels[static_cast<size_t>(idx)];
}

void ProgressionSystem::AddXP(uint32_t amount) {
    m_TotalXP += amount;
    RecalculateLevel();
}

uint32_t ProgressionSystem::GetXPForNextLevel() const noexcept {
    const uint32_t needed = XPThreshold(m_Level + 1);
    return (needed > m_TotalXP) ? needed - m_TotalXP : 0u;
}

bool ProgressionSystem::IsUnlocked(SkillUnlock skill) const noexcept {
    return m_Level >= UnlockLevel(skill);
}

void ProgressionSystem::Reset() noexcept {
    m_TotalXP = 0;
    m_Level   = 1;
}

void ProgressionSystem::RecalculateLevel() {
    // Advance level while threshold for next level is met.
    while (m_TotalXP >= XPThreshold(m_Level + 1)) {
        ++m_Level;
        if (m_LevelUpCb) m_LevelUpCb(m_Level);
    }
}

} // namespace NF::Game::Gameplay
