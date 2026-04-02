#include "Game/Interaction/RigState.h"
#include <algorithm>
#include <utility>

namespace NF::Game {

RigState::RigState(std::string name)
    : m_Name(std::move(name))
{}

void RigState::TakeDamage(float amount) noexcept {
    m_Health = std::max(0.f, m_Health - amount);
}

void RigState::RepairHealth(float amount) noexcept {
    m_Health = std::min(kMaxHealth, m_Health + amount);
}

bool RigState::ConsumeEnergy(float amount) noexcept {
    if (amount < 0.f) return false;
    if (m_Energy < amount) return false;
    m_Energy -= amount;
    return true;
}

void RigState::RechargeEnergy(float amount) noexcept {
    m_Energy = std::min(kMaxEnergy, m_Energy + amount);
}

void RigState::SetToolSlot(int slot) noexcept {
    m_ToolSlot = std::clamp(slot, 0, kToolSlotCount - 1);
}

void RigState::Tick(float dt) noexcept {
    if (!IsAlive()) return;
    RechargeEnergy(kEnergyRechargeRate * dt);
}

void RigState::Reset() noexcept {
    m_Health   = kMaxHealth;
    m_Energy   = kMaxEnergy;
    m_ToolSlot = 0;
}

} // namespace NF::Game
