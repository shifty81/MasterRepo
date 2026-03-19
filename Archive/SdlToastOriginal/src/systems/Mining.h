#ifndef MINING_H
#define MINING_H

#include <string>
#include <vector>

/**
 * Represents a type of ore or gem that can be mined.
 */
struct OreType {
    std::string name;
    int value;         // Sell price
    int hardness;      // 1-10 mining difficulty
    int minSkillLevel; // Minimum mining skill required to find
};

/**
 * Mining system — determines what ores/gems can be found and mining success.
 * Works similarly to FishingSystem: skill level + difficulty + luck.
 */
class MiningSystem {
public:
    MiningSystem();

    // Get all ores available at a given skill level
    std::vector<const OreType*> GetAvailableOres(int skillLevel) const;

    // Attempt to mine. Returns ore index on success, -1 on failure.
    // skillLevel (0-10) and ore hardness affect success.
    int AttemptMine(int skillLevel) const;

    // Deterministic mine for testing — pass a roll in [0, 100)
    int AttemptMineWithRoll(int skillLevel, int roll) const;

    // Access ore data
    const OreType* GetOre(int index) const;
    int GetOreCount() const { return static_cast<int>(m_ores.size()); }

    // Calculate energy cost based on skill level (higher skill = less energy)
    int GetEnergyCost(int skillLevel) const;

    // Calculate XP gained from mining an ore
    int GetXPReward(int oreIndex) const;

private:
    std::vector<OreType> m_ores;
    void InitOres();
};

#endif // MINING_H
