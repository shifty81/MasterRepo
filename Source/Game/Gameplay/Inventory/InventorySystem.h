#pragma once
#include "Game/Interaction/Inventory.h"
#include "Game/Interaction/ResourceItem.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace NF::Game::Gameplay {

/// @brief Opaque handle to a named inventory container.
using ContainerId = uint32_t;
static constexpr ContainerId kInvalidContainer = 0;

/// @brief Full inventory management system — manages multiple named containers.
///
/// Each container is an independent @c Inventory with a name tag.
/// Use AddContainer() to create one, then Transfer() to move items between them.
class InventorySystem {
public:
    InventorySystem()  = default;
    ~InventorySystem() = default;

    // -------------------------------------------------------------------------
    // Container management
    // -------------------------------------------------------------------------

    /// @brief Create a new named container.
    /// @return Unique ContainerId for future lookups; kInvalidContainer on failure.
    ContainerId AddContainer(const std::string& name);

    /// @brief Destroy a container by ID.
    /// @return True if the container existed and was removed.
    bool RemoveContainer(ContainerId id);

    /// @brief Return a pointer to the container, or nullptr if not found.
    [[nodiscard]] NF::Game::Inventory*       GetContainer(ContainerId id)       noexcept;
    [[nodiscard]] const NF::Game::Inventory* GetContainer(ContainerId id) const noexcept;

    /// @brief Look up a container by its name (first match).
    [[nodiscard]] ContainerId FindContainer(const std::string& name) const noexcept;

    /// @brief Number of registered containers.
    [[nodiscard]] int ContainerCount() const noexcept {
        return static_cast<int>(m_Containers.size());
    }

    // -------------------------------------------------------------------------
    // Transfer
    // -------------------------------------------------------------------------

    /// @brief Move @p count of @p type from container @p from to container @p to.
    /// @return True on success; false if either container is missing or
    ///         the source has insufficient items.
    bool Transfer(ContainerId from, ContainerId to,
                  NF::Game::ResourceType type, uint32_t count = 1);

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// @brief Clear all containers.
    void Clear() noexcept;

private:
    struct Entry {
        ContainerId          id{kInvalidContainer};
        std::string          name;
        NF::Game::Inventory  inventory;
    };

    ContainerId m_NextId{1};
    std::vector<Entry> m_Containers;

    [[nodiscard]] Entry*       FindEntry(ContainerId id)       noexcept;
    [[nodiscard]] const Entry* FindEntry(ContainerId id) const noexcept;
};

} // namespace NF::Game::Gameplay
