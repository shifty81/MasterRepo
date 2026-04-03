#include "Game/Gameplay/Economy/TradeMarket.h"

namespace NF::Game::Gameplay {

MarketEntry* TradeMarket::FindEntry(NF::Game::ResourceType type) noexcept {
    for (auto& e : m_Entries) {
        if (e.type == type) return &e;
    }
    return nullptr;
}

const MarketEntry* TradeMarket::FindEntry(NF::Game::ResourceType type) const noexcept {
    for (const auto& e : m_Entries) {
        if (e.type == type) return &e;
    }
    return nullptr;
}

void TradeMarket::AdjustPrice(MarketEntry& e, float basePrice, float direction) noexcept {
    // direction: +1 = demand (price up), -1 = supply (price down)
    e.currentPrice += direction * basePrice * kPriceStep;
    const float floor   = basePrice * kPriceFloor;
    const float ceiling = basePrice * kPriceCeiling;
    if (e.currentPrice < floor)   e.currentPrice = floor;
    if (e.currentPrice > ceiling) e.currentPrice = ceiling;
}

// ---- setup ------------------------------------------------------------------

void TradeMarket::Initialize(const ResourceRegistry& reg, uint32_t initialStock) {
    m_Reg = &reg;
    for (int i = 0; i < kEntryCount; ++i) {
        const auto& def = reg.Get(static_cast<NF::Game::ResourceType>(i));
        m_Entries[i].type         = def.type;
        m_Entries[i].stock        = def.tradeable ? initialStock : 0u;
        m_Entries[i].currentPrice = def.basePrice;
    }
}

// ---- queries ----------------------------------------------------------------

float TradeMarket::GetPrice(NF::Game::ResourceType type) const noexcept {
    const MarketEntry* e = FindEntry(type);
    return e ? e->currentPrice : 0.f;
}

uint32_t TradeMarket::GetStock(NF::Game::ResourceType type) const noexcept {
    const MarketEntry* e = FindEntry(type);
    return e ? e->stock : 0u;
}

void TradeMarket::AddCredits(float amount) noexcept {
    m_PlayerCredits += amount;
    if (m_PlayerCredits < 0.f) m_PlayerCredits = 0.f;
}

// ---- trade actions ----------------------------------------------------------

TradeResult TradeMarket::Buy(NF::Game::ResourceType type, uint32_t count,
                              NF::Game::Inventory& inv)
{
    if (type == NF::Game::ResourceType::None) return TradeResult::InvalidType;
    if (!m_Reg) return TradeResult::InvalidType;

    const ResourceDef& def = m_Reg->Get(type);
    if (!def.tradeable) return TradeResult::NotTradeable;

    MarketEntry* e = FindEntry(type);
    if (!e) return TradeResult::InvalidType;

    if (e->stock < count) return TradeResult::InsufficientStock;

    const float cost = e->currentPrice * static_cast<float>(count);
    if (m_PlayerCredits < cost) return TradeResult::InsufficientFunds;

    // Execute.
    if (!inv.AddItem(type, count)) return TradeResult::InsufficientStock; // inv full
    m_PlayerCredits -= cost;
    e->stock        -= count;
    AdjustPrice(*e, def.basePrice, +1.f); // demand up

    return TradeResult::Success;
}

TradeResult TradeMarket::Sell(NF::Game::ResourceType type, uint32_t count,
                               NF::Game::Inventory& inv)
{
    if (type == NF::Game::ResourceType::None) return TradeResult::InvalidType;
    if (!m_Reg) return TradeResult::InvalidType;

    const ResourceDef& def = m_Reg->Get(type);
    if (!def.tradeable) return TradeResult::NotTradeable;

    if (!inv.HasItem(type, count)) return TradeResult::InsufficientItems;

    MarketEntry* e = FindEntry(type);
    if (!e) return TradeResult::InvalidType;

    // Execute.
    inv.RemoveItem(type, count);
    const float revenue = e->currentPrice * static_cast<float>(count);
    m_PlayerCredits += revenue;
    e->stock        += count;
    AdjustPrice(*e, def.basePrice, -1.f); // supply up, price down

    return TradeResult::Success;
}

} // namespace NF::Game::Gameplay
