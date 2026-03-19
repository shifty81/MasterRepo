#include "Runtime/Equipment/Equipment.h"

namespace Runtime::Equipment {

bool Equipment::Equip(Slot slot, const Runtime::Inventory::Item& item) {
    m_slots[slot] = EquippedItem{slot, item};
    return true;
}

bool Equipment::Unequip(Slot slot, Runtime::Inventory::Item& outItem) {
    auto it = m_slots.find(slot);
    if (it == m_slots.end())
        return false;
    outItem = it->second.item;
    m_slots.erase(it);
    return true;
}

const EquippedItem* Equipment::GetEquipped(Slot slot) const {
    auto it = m_slots.find(slot);
    if (it == m_slots.end())
        return nullptr;
    return &it->second;
}

bool Equipment::IsSlotFilled(Slot slot) const {
    return m_slots.count(slot) > 0;
}

std::vector<EquippedItem> Equipment::GetAllEquipped() const {
    std::vector<EquippedItem> result;
    result.reserve(m_slots.size());
    for (const auto& [slot, equipped] : m_slots)
        result.push_back(equipped);
    return result;
}

float Equipment::GetTotalWeight() const {
    float total = 0.0f;
    for (const auto& [slot, equipped] : m_slots)
        total += equipped.item.weight * static_cast<float>(equipped.item.quantity);
    return total;
}

} // namespace Runtime::Equipment
