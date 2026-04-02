#pragma once
#include <cstdint>
#include <string>

namespace NF::Game {

/// @brief Starter R.I.G. (Rig Interface Gear) state.
///
/// The rig starts as a minimal exo-frame / backpack assembly with limited
/// life-support energy and structural integrity (health).  Helmet deployment
/// and expanded modules are deferred to later phases.
class RigState {
public:
    static constexpr float kMaxHealth         = 100.f;
    static constexpr float kMaxEnergy         = 100.f;
    static constexpr float kEnergyRechargeRate = 5.f;  ///< Per second when idle.

    RigState() = default;
    explicit RigState(std::string name);

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    /// @brief Current structural integrity (0 = destroyed, 100 = full).
    [[nodiscard]] float GetHealth() const noexcept { return m_Health; }

    /// @brief Current power reserve (0 = depleted, 100 = full).
    [[nodiscard]] float GetEnergy() const noexcept { return m_Energy; }

    /// @brief Active tool slot index (0 = mining, 1 = placement, 2 = repair).
    [[nodiscard]] int GetToolSlot() const noexcept { return m_ToolSlot; }

    /// @brief Display name of this rig (e.g. "StarterRig").
    [[nodiscard]] const std::string& GetName() const noexcept { return m_Name; }

    /// @brief Returns true while health > 0.
    [[nodiscard]] bool IsAlive() const noexcept { return m_Health > 0.f; }

    // -------------------------------------------------------------------------
    // Health
    // -------------------------------------------------------------------------

    /// @brief Reduce rig health by @p amount. Clamps to [0, kMaxHealth].
    void TakeDamage(float amount) noexcept;

    /// @brief Restore rig health by @p amount. Clamps to kMaxHealth.
    void RepairHealth(float amount) noexcept;

    // -------------------------------------------------------------------------
    // Energy
    // -------------------------------------------------------------------------

    /// @brief Consume @p amount of energy.
    /// @return True if sufficient energy was available and has been deducted.
    [[nodiscard]] bool ConsumeEnergy(float amount) noexcept;

    /// @brief Recharge energy by @p amount. Clamps to kMaxEnergy.
    void RechargeEnergy(float amount) noexcept;

    // -------------------------------------------------------------------------
    // Tool selection
    // -------------------------------------------------------------------------

    /// @brief Set the active tool slot (clamped to [0, kToolSlotCount-1]).
    void SetToolSlot(int slot) noexcept;

    static constexpr int kToolSlotCount = 3; ///< mining / placement / repair

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// @brief Per-frame update: passively recharges energy when alive.
    void Tick(float dt) noexcept;

    /// @brief Reset all state to defaults (full health, full energy, slot 0).
    void Reset() noexcept;

private:
    std::string m_Name{"StarterRig"};
    float       m_Health{kMaxHealth};
    float       m_Energy{kMaxEnergy};
    int         m_ToolSlot{0};
};

} // namespace NF::Game
