#pragma once
#include <vector>
#include <map>
#include "Runtime/Inventory/Inventory.h"

namespace Runtime::Equipment {

enum class Slot {
    Head,
    Chest,
    Legs,
    Feet,
    Hands,
    MainHand,
    OffHand,
    Ring1,
    Ring2,
    Neck
};

struct EquippedItem {
    Slot                    slot;
    Runtime::Inventory::Item item;
};

class Equipment {
public:
    // Equip an item in the given slot.  Any previously equipped item is
    // silently replaced (call Unequip first if you need it back).
    bool Equip(Slot slot, const Runtime::Inventory::Item& item);

    // Remove the item from `slot` and place it in `outItem`.
    // Returns false if the slot is empty.
    bool Unequip(Slot slot, Runtime::Inventory::Item& outItem);

    const EquippedItem* GetEquipped(Slot slot) const;
    bool                IsSlotFilled(Slot slot) const;

    std::vector<EquippedItem> GetAllEquipped() const;
    float                     GetTotalWeight() const;

private:
    std::map<Slot, EquippedItem> m_slots;
};

} // namespace Runtime::Equipment
