#pragma once
#include "Game/Interaction/Inventory.h"
#include "Game/Interaction/ResourceItem.h"
#include <cstdint>
#include <string>
#include <vector>

namespace NF::Game::Gameplay {

/// @brief Opaque handle for a storage box.
using StorageBoxId = uint32_t;
static constexpr StorageBoxId kInvalidBoxId = 0;

/// @brief Simple 3-D float position used to locate a storage box in the world.
struct StoragePos {
    float x{0.f}, y{0.f}, z{0.f};

    [[nodiscard]] float DistanceSqTo(const StoragePos& o) const noexcept {
        const float dx = x - o.x, dy = y - o.y, dz = z - o.z;
        return dx*dx + dy*dy + dz*dz;
    }
};

/// @brief Persistent storage containers placed in the world.
///
/// Each storage box is a named container at a fixed world position.
/// Players or systems can deposit/withdraw items, and boxes can be
/// located by position.
class StorageSystem {
public:
    StorageSystem()  = default;
    ~StorageSystem() = default;

    // -------------------------------------------------------------------------
    // Box management
    // -------------------------------------------------------------------------

    /// @brief Register a new storage box.
    StorageBoxId AddBox(const std::string& label, StoragePos pos);

    /// @brief Remove a box (its inventory is discarded).
    bool RemoveBox(StorageBoxId id);

    /// @brief Return the inventory for a box, or nullptr.
    [[nodiscard]] NF::Game::Inventory*       GetBox(StorageBoxId id)       noexcept;
    [[nodiscard]] const NF::Game::Inventory* GetBox(StorageBoxId id) const noexcept;

    /// @brief Number of registered boxes.
    [[nodiscard]] int BoxCount() const noexcept {
        return static_cast<int>(m_Boxes.size());
    }

    // -------------------------------------------------------------------------
    // Spatial lookup
    // -------------------------------------------------------------------------

    /// @brief Find the nearest box within @p radius of @p pos.
    /// @return kInvalidBoxId when none is found within the radius.
    [[nodiscard]] StorageBoxId FindNearest(StoragePos pos, float radius) const noexcept;

    // -------------------------------------------------------------------------
    // Transfer helpers
    // -------------------------------------------------------------------------

    /// @brief Deposit @p count of @p type from @p src into box @p id.
    bool Deposit(StorageBoxId id, NF::Game::Inventory& src,
                 NF::Game::ResourceType type, uint32_t count = 1);

    /// @brief Withdraw @p count of @p type from box @p id into @p dst.
    bool Withdraw(StorageBoxId id, NF::Game::Inventory& dst,
                  NF::Game::ResourceType type, uint32_t count = 1);

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------
    void Clear() noexcept;

private:
    struct BoxEntry {
        StorageBoxId        id{kInvalidBoxId};
        std::string         label;
        StoragePos          pos;
        NF::Game::Inventory inventory;
    };

    StorageBoxId       m_NextId{1};
    std::vector<BoxEntry> m_Boxes;

    [[nodiscard]] BoxEntry*       FindEntry(StorageBoxId id)       noexcept;
    [[nodiscard]] const BoxEntry* FindEntry(StorageBoxId id) const noexcept;
};

} // namespace NF::Game::Gameplay
