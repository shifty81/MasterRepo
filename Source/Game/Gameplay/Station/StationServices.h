#pragma once
#include "Game/Gameplay/Economy/TradeMarket.h"
#include "Game/Gameplay/Manufacturing/ManufacturingQueue.h"
#include "Game/Interaction/RigState.h"
#include <string>

namespace NF::Game::Gameplay {

/// @brief Result of a docking / service action.
enum class StationServiceResult : uint8_t {
    Success,
    AlreadyDocked,
    NotDocked,
    InsufficientFunds,
    AlreadyFullHealth,
    AlreadyFullEnergy,
};

/// @brief Station docking, repair, refuel, trade, and manufacturing services.
///
/// A station hosts:
///   - A @c TradeMarket for buying/selling resources.
///   - A @c ManufacturingQueue for crafting items from resources.
///   - Repair and refuel services charged at a per-unit credit rate.
///
/// Players must Dock() before using any service, and must Undock() when done.
class StationServices {
public:
    static constexpr float kRepairCostPerHp    = 0.5f; ///< Credits per HP restored.
    static constexpr float kRefuelCostPerEnergy = 0.2f; ///< Credits per energy unit.

    explicit StationServices(const std::string& name);

    // -------------------------------------------------------------------------
    // Docking
    // -------------------------------------------------------------------------
    StationServiceResult Dock()   noexcept;
    StationServiceResult Undock() noexcept;
    [[nodiscard]] bool IsDocked() const noexcept { return m_Docked; }

    // -------------------------------------------------------------------------
    // Repair & Refuel
    // -------------------------------------------------------------------------

    /// @brief Fully repair the rig's health using credits from the TradeMarket.
    StationServiceResult Repair(NF::Game::RigState& rig);

    /// @brief Fully recharge the rig's energy using credits from the TradeMarket.
    StationServiceResult Refuel(NF::Game::RigState& rig);

    // -------------------------------------------------------------------------
    // Sub-systems (accessible while docked)
    // -------------------------------------------------------------------------
    [[nodiscard]] TradeMarket&       GetMarket()       noexcept { return m_Market; }
    [[nodiscard]] const TradeMarket& GetMarket() const noexcept { return m_Market; }

    [[nodiscard]] ManufacturingQueue&       GetFactory()       noexcept { return m_Factory; }
    [[nodiscard]] const ManufacturingQueue& GetFactory() const noexcept { return m_Factory; }

    // -------------------------------------------------------------------------
    // Identity
    // -------------------------------------------------------------------------
    [[nodiscard]] const std::string& GetName() const noexcept { return m_Name; }

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// @brief Per-frame update: advances the manufacturing queue.
    void Tick(float dt);

private:
    std::string        m_Name;
    bool               m_Docked{false};
    TradeMarket        m_Market;
    ManufacturingQueue m_Factory;
};

} // namespace NF::Game::Gameplay
