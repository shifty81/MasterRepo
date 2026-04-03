#include "Game/Gameplay/Station/StationServices.h"

namespace NF::Game::Gameplay {

StationServices::StationServices(const std::string& name)
    : m_Name(name)
{
}

StationServiceResult StationServices::Dock() noexcept {
    if (m_Docked) return StationServiceResult::AlreadyDocked;
    m_Docked = true;
    return StationServiceResult::Success;
}

StationServiceResult StationServices::Undock() noexcept {
    if (!m_Docked) return StationServiceResult::NotDocked;
    m_Docked = false;
    return StationServiceResult::Success;
}

StationServiceResult StationServices::Repair(NF::Game::RigState& rig) {
    if (!m_Docked) return StationServiceResult::NotDocked;

    const float missing = NF::Game::RigState::kMaxHealth - rig.GetHealth();
    if (missing <= 0.f) return StationServiceResult::AlreadyFullHealth;

    const float cost = missing * kRepairCostPerHp;
    if (m_Market.GetCredits() < cost) return StationServiceResult::InsufficientFunds;

    m_Market.AddCredits(-cost);
    rig.RepairHealth(missing);
    return StationServiceResult::Success;
}

StationServiceResult StationServices::Refuel(NF::Game::RigState& rig) {
    if (!m_Docked) return StationServiceResult::NotDocked;

    const float missing = NF::Game::RigState::kMaxEnergy - rig.GetEnergy();
    if (missing <= 0.f) return StationServiceResult::AlreadyFullEnergy;

    const float cost = missing * kRefuelCostPerEnergy;
    if (m_Market.GetCredits() < cost) return StationServiceResult::InsufficientFunds;

    m_Market.AddCredits(-cost);
    rig.RechargeEnergy(missing);
    return StationServiceResult::Success;
}

void StationServices::Tick(float dt) {
    m_Factory.Tick(dt);
}

} // namespace NF::Game::Gameplay
