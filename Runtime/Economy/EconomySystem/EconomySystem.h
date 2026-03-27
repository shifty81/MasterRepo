#pragma once
/**
 * @file EconomySystem.h
 * @brief Currency ledger, buy/sell with price modifiers, supply/demand, transaction log.
 *
 * Features:
 *   - Per-entity currency ledger (entityId → balance map)
 *   - Item catalogue: ItemDef (id, name, basePrice, category)
 *   - Listing: RegisterItem, SetStockLevel(itemId, qty)
 *   - Buy(buyerId, itemId, qty) / Sell(sellerId, itemId, qty)
 *   - Dynamic pricing: price = basePrice * (1 + demandFactor - supplyFactor)
 *   - Price modifier: global multiplier, per-item multiplier, per-entity discount
 *   - Transaction log: last N transactions (seller, buyer, item, qty, price)
 *   - GetBalance(entityId) / AddCurrency / RemoveCurrency
 *   - On-transaction callback
 *   - Tax rate: deducted from seller on each sale
 *
 * Typical usage:
 * @code
 *   EconomySystem eco;
 *   eco.Init();
 *   eco.RegisterItem({"sword", "Iron Sword", 50.f, "weapon"});
 *   eco.SetStockLevel(0, "sword", 10); // merchant 0 has 10 swords
 *   eco.AddCurrency(1, 200.f);         // player has 200g
 *   eco.Buy(1, 0, "sword", 1);         // player buys from merchant
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct ItemDef {
    std::string id;
    std::string name;
    float       basePrice{1.f};
    std::string category;
};

struct Transaction {
    uint32_t    sellerId, buyerId;
    std::string itemId;
    uint32_t    quantity{0};
    float       totalPrice{0.f};
    float       timestamp {0.f};
};

class EconomySystem {
public:
    EconomySystem();
    ~EconomySystem();

    void Init    ();
    void Shutdown();

    // Item catalogue
    void           RegisterItem  (const ItemDef& def);
    void           UnregisterItem(const std::string& id);
    const ItemDef* GetItem       (const std::string& id) const;

    // Stock
    void     SetStockLevel(uint32_t sellerId, const std::string& itemId, uint32_t qty);
    uint32_t GetStockLevel(uint32_t sellerId, const std::string& itemId) const;

    // Currency
    void  AddCurrency   (uint32_t entityId, float amount);
    bool  RemoveCurrency(uint32_t entityId, float amount);
    float GetBalance    (uint32_t entityId) const;

    // Trade
    bool Buy (uint32_t buyerId,  uint32_t sellerId, const std::string& itemId, uint32_t qty);
    bool Sell(uint32_t sellerId, uint32_t buyerId,  const std::string& itemId, uint32_t qty);

    // Pricing
    float GetPrice(uint32_t sellerId, const std::string& itemId) const;
    void  SetGlobalPriceMultiplier  (float m);
    void  SetItemPriceMultiplier    (const std::string& itemId, float m);
    void  SetEntityDiscount         (uint32_t entityId, float discount); ///< [0,1]
    void  SetTaxRate                (float rate);                        ///< [0,1]

    // Log
    const std::vector<Transaction>& GetTransactionLog() const;
    void SetTransactionLogSize(uint32_t maxEntries);

    // Callback
    void SetOnTransaction(std::function<void(const Transaction&)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
