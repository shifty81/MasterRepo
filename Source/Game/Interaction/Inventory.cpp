#include "Game/Interaction/Inventory.h"
#include <cassert>

namespace NF::Game {

// ---- private helpers --------------------------------------------------------

int Inventory::FindSlot(ResourceType type) const noexcept {
    for (int i = 0; i < kMaxSlots; ++i) {
        if (m_Slots[i].type == type && m_Slots[i].count > 0)
            return i;
    }
    return -1;
}

int Inventory::FindEmptySlot() const noexcept {
    for (int i = 0; i < kMaxSlots; ++i) {
        if (m_Slots[i].IsEmpty())
            return i;
    }
    return -1;
}

// ---- queries ----------------------------------------------------------------

uint32_t Inventory::GetCount(ResourceType type) const noexcept {
    const int idx = FindSlot(type);
    return idx >= 0 ? m_Slots[idx].count : 0u;
}

const ResourceStack& Inventory::GetSlot(int index) const noexcept {
    assert(index >= 0 && index < kMaxSlots);
    return m_Slots[static_cast<size_t>(index)];
}

bool Inventory::HasItem(ResourceType type, uint32_t count) const noexcept {
    return GetCount(type) >= count;
}

bool Inventory::IsFull() const noexcept {
    return FindEmptySlot() == -1;
}

// ---- mutators ---------------------------------------------------------------

bool Inventory::AddItem(ResourceType type, uint32_t count) {
    if (type == ResourceType::None || count == 0) return false;

    // Merge into existing slot.
    int idx = FindSlot(type);
    if (idx >= 0) {
        m_Slots[idx].count += count;
        return true;
    }

    // Allocate new slot.
    idx = FindEmptySlot();
    if (idx < 0) return false; // inventory full

    m_Slots[idx].type  = type;
    m_Slots[idx].count = count;
    return true;
}

bool Inventory::RemoveItem(ResourceType type, uint32_t count) {
    const int idx = FindSlot(type);
    if (idx < 0) return false;
    if (m_Slots[idx].count < count) return false;

    m_Slots[idx].count -= count;
    if (m_Slots[idx].count == 0)
        m_Slots[idx] = ResourceStack{}; // clear the slot

    return true;
}

void Inventory::Clear() noexcept {
    m_Slots.fill(ResourceStack{});
}

} // namespace NF::Game
