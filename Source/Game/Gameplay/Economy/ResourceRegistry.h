#pragma once
#include "Game/Interaction/ResourceItem.h"
#include <cstdint>
#include <string>
#include <array>

namespace NF::Game::Gameplay {

/// @brief Static definition of a tradeable / craftable resource.
struct ResourceDef {
    NF::Game::ResourceType type{NF::Game::ResourceType::None};
    const char*  name{""};         ///< Display name.
    float        basePrice{1.f};   ///< Base credits per unit.
    float        mass{0.1f};       ///< Mass in kg per unit (for cargo).
    bool         tradeable{true};  ///< Can be bought/sold on a TradeMarket.
    bool         craftable{false}; ///< Can be produced by ManufacturingQueue.
};

/// @brief Central catalog of all resource type definitions.
///
/// All definitions are statically allocated — no heap allocation required.
/// Each ResourceType ID (1–7) maps to a fixed entry.
/// ResourceType::None maps to the placeholder entry at index 0.
class ResourceRegistry {
public:
    /// @brief Total number of registered resource types (including None).
    static constexpr int kCount = 8; // None + 7 types

    ResourceRegistry();

    // -------------------------------------------------------------------------
    // Lookup
    // -------------------------------------------------------------------------

    /// @brief Return the definition for @p type.
    [[nodiscard]] const ResourceDef& Get(NF::Game::ResourceType type) const noexcept;

    /// @brief Find a resource definition by name (case-sensitive).
    ///        Returns the entry for ResourceType::None when not found.
    [[nodiscard]] const ResourceDef& FindByName(const char* name) const noexcept;

    /// @brief Total number of entries including None.
    [[nodiscard]] static constexpr int Count() noexcept { return kCount; }

private:
    std::array<ResourceDef, kCount> m_Defs;
};

} // namespace NF::Game::Gameplay
