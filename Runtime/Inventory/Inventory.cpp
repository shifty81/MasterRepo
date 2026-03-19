#include "Runtime/Inventory/Inventory.h"

namespace Runtime::Inventory {

Inventory::Inventory(int capacity)
    : m_capacity(capacity) {}

bool Inventory::AddItem(const Item& item) {
    if (item.quantity <= 0)
        return false;

    int remaining = item.quantity;

    // Try to stack onto an existing slot with the same id.
    for (auto& existing : m_items) {
        if (existing.id == item.id && existing.quantity < existing.maxStack) {
            int space    = existing.maxStack - existing.quantity;
            int toAdd    = std::min(space, remaining);
            existing.quantity += toAdd;
            remaining         -= toAdd;
            if (remaining == 0)
                return true;
        }
    }

    // Open new slots for the remainder.
    while (remaining > 0) {
        if (static_cast<int>(m_items.size()) >= m_capacity)
            return false; // inventory full

        Item newSlot  = item;
        newSlot.quantity = std::min(remaining, item.maxStack);
        m_items.push_back(newSlot);
        remaining -= newSlot.quantity;
    }

    return true;
}

bool Inventory::RemoveItem(const std::string& id, int quantity) {
    if (quantity <= 0)
        return false;

    // First pass: check we have enough total quantity.
    int total = 0;
    for (const auto& it : m_items)
        if (it.id == id)
            total += it.quantity;

    if (total < quantity)
        return false;

    // Second pass: consume from stacks, removing empty ones.
    int remaining = quantity;
    for (auto it = m_items.begin(); it != m_items.end() && remaining > 0; ) {
        if (it->id == id) {
            int take = std::min(it->quantity, remaining);
            it->quantity -= take;
            remaining    -= take;
            if (it->quantity == 0)
                it = m_items.erase(it);
            else
                ++it;
        } else {
            ++it;
        }
    }

    return true;
}

Item* Inventory::FindItem(const std::string& id) {
    for (auto& it : m_items)
        if (it.id == id)
            return &it;
    return nullptr;
}

const Item* Inventory::FindItem(const std::string& id) const {
    for (const auto& it : m_items)
        if (it.id == id)
            return &it;
    return nullptr;
}

std::vector<Item> Inventory::GetItemsByCategory(const std::string& category) const {
    std::vector<Item> result;
    for (const auto& it : m_items)
        if (it.category == category)
            result.push_back(it);
    return result;
}

int Inventory::GetUsedSlots() const {
    return static_cast<int>(m_items.size());
}

float Inventory::GetTotalWeight() const {
    float total = 0.0f;
    for (const auto& it : m_items)
        total += it.weight * static_cast<float>(it.quantity);
    return total;
}

void Inventory::Clear() {
    m_items.clear();
}

} // namespace Runtime::Inventory
