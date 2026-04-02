#pragma once
#include "Game/Interaction/ResourceItem.h"
#include <array>
#include <cstdint>

namespace NF::Game {

/// @brief Simple slot-based resource inventory.
///
/// Holds up to kMaxSlots unique resource stacks.  Stacks of the same
/// ResourceType are merged into a single slot.  Adding to a full inventory
/// (all slots occupied with different types) fails gracefully.
class Inventory {
public:
    static constexpr int kMaxSlots = 16;

    Inventory() = default;

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    /// @brief Total count of @p type across all slots.
    [[nodiscard]] uint32_t GetCount(ResourceType type) const noexcept;

    /// @brief Direct read access to a slot by index [0, kMaxSlots).
    [[nodiscard]] const ResourceStack& GetSlot(int index) const noexcept;

    /// @brief Returns true if @p count of @p type is available.
    [[nodiscard]] bool HasItem(ResourceType type, uint32_t count = 1) const noexcept;

    /// @brief Returns true when no empty slot remains and all slots are occupied
    ///        by distinct types (no room for a new type entry).
    [[nodiscard]] bool IsFull() const noexcept;

    // -------------------------------------------------------------------------
    // Mutators
    // -------------------------------------------------------------------------

    /// @brief Add @p count of @p type to the inventory.
    /// @return True on success, false if the inventory is full and a new slot
    ///         would be required.
    bool AddItem(ResourceType type, uint32_t count = 1);

    /// @brief Remove @p count of @p type.
    /// @return True on success, false if not enough items available.
    bool RemoveItem(ResourceType type, uint32_t count = 1);

    /// @brief Clear all slots.
    void Clear() noexcept;

private:
    std::array<ResourceStack, kMaxSlots> m_Slots{};

    [[nodiscard]] int FindSlot(ResourceType type) const noexcept;
    [[nodiscard]] int FindEmptySlot() const noexcept;
};

} // namespace NF::Game
