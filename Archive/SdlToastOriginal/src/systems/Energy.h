#ifndef ENERGY_H
#define ENERGY_H

/**
 * Energy system — governs how many actions the player can perform per day.
 * Energy is consumed by farming, combat, mining, and fishing actions.
 * Restored each morning when the player sleeps (day advance).
 */
class Energy {
public:
    Energy();

    // Query
    int GetCurrent() const { return m_current; }
    int GetMax() const { return m_max; }
    bool HasEnergy(int cost) const { return m_current >= cost; }
    bool IsExhausted() const { return m_current <= 0; }

    // Consume energy; returns true if enough energy was available
    bool Consume(int amount);

    // Restore energy (clamped to max)
    void Restore(int amount);

    // Fully restore energy (called on new day / sleep)
    void RestoreFull();

    // Increase max energy (level-up reward)
    void IncreaseMax(int amount);

    // Setters for save/load
    void SetCurrent(int current);
    void SetMax(int max);

    // Action energy costs
    static constexpr int COST_TILL      = 4;
    static constexpr int COST_WATER     = 2;
    static constexpr int COST_PLANT     = 3;
    static constexpr int COST_HARVEST   = 2;
    static constexpr int COST_CHOP      = 6;
    static constexpr int COST_MINE      = 5;
    static constexpr int COST_FISH_CAST = 4;
    static constexpr int COST_ATTACK    = 3;

    // Defaults
    static constexpr int DEFAULT_MAX = 100;

private:
    int m_current;
    int m_max;
};

#endif // ENERGY_H
