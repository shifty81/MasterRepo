#pragma once
#include "Game/Gameplay/Economy/ResourceRegistry.h"
#include "Game/Interaction/ResourceItem.h"
#include "Game/Interaction/Inventory.h"
#include <cstdint>

namespace NF::Game::Gameplay {

/// @brief Result of a trade (buy or sell) action.
enum class TradeResult : uint8_t {
    Success,             ///< Trade completed.
    InsufficientFunds,   ///< Buyer does not have enough credits.
    InsufficientStock,   ///< Market does not have the requested quantity.
    InsufficientItems,   ///< Seller does not have the items.
    NotTradeable,        ///< Resource type is not marked tradeable.
    InvalidType,         ///< Unknown resource type.
};

/// @brief Per-resource market entry tracking supply, demand, and price.
struct MarketEntry {
    NF::Game::ResourceType type{NF::Game::ResourceType::None};
    uint32_t stock{0};          ///< Units available for purchase.
    float    currentPrice{1.f}; ///< Effective price per unit (fluctuates).
};

/// @brief Dynamic player-facing commodity market.
///
/// Uses a simple supply/demand model: buying reduces stock and raises price;
/// selling increases stock and lowers price.  Prices fluctuate between
/// kPriceFloor and kPriceCeiling relative to the base price from
/// ResourceRegistry.
///
/// The market tracks a credits pool as a floating-point balance.  At game
/// start the market is seeded with initial stock (see @c Initialize).
class TradeMarket {
public:
    static constexpr float kPriceFloor   = 0.5f; ///< Fraction of base price.
    static constexpr float kPriceCeiling = 3.0f; ///< Fraction of base price.

    /// @brief Price movement per unit bought/sold (as fraction of base price).
    static constexpr float kPriceStep    = 0.05f;

    TradeMarket() = default;

    // -------------------------------------------------------------------------
    // Setup
    // -------------------------------------------------------------------------

    /// @brief Initialise market entries from the registry with default stock.
    void Initialize(const ResourceRegistry& reg, uint32_t initialStock = 100);

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    /// @brief Current effective price per unit for @p type.
    [[nodiscard]] float GetPrice(NF::Game::ResourceType type) const noexcept;

    /// @brief Current stock for @p type.
    [[nodiscard]] uint32_t GetStock(NF::Game::ResourceType type) const noexcept;

    /// @brief Player's current credit balance.
    [[nodiscard]] float GetCredits() const noexcept { return m_PlayerCredits; }

    // -------------------------------------------------------------------------
    // Credits management
    // -------------------------------------------------------------------------

    void AddCredits(float amount) noexcept;
    void SetCredits(float amount) noexcept { m_PlayerCredits = amount; }

    // -------------------------------------------------------------------------
    // Trade actions
    // -------------------------------------------------------------------------

    /// @brief Buy @p count units of @p type from the market into @p inv.
    ///        Deducts credits from the player.
    TradeResult Buy(NF::Game::ResourceType type, uint32_t count,
                    NF::Game::Inventory& inv);

    /// @brief Sell @p count units of @p type from @p inv to the market.
    ///        Adds credits to the player.
    TradeResult Sell(NF::Game::ResourceType type, uint32_t count,
                     NF::Game::Inventory& inv);

private:
    static constexpr int kEntryCount = ResourceRegistry::kCount;

    MarketEntry m_Entries[kEntryCount]{};
    float       m_PlayerCredits{0.f};
    const ResourceRegistry* m_Reg{nullptr};

    [[nodiscard]] MarketEntry* FindEntry(NF::Game::ResourceType type) noexcept;
    [[nodiscard]] const MarketEntry* FindEntry(NF::Game::ResourceType type) const noexcept;

    void AdjustPrice(MarketEntry& e, float basePrice, float direction) noexcept;
};

} // namespace NF::Game::Gameplay
