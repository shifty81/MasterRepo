#pragma once
/**
 * @file MarketSystem.h
 * @brief Dynamic economy — markets, prices, trade routes, and supply chains.
 *
 * Supports:
 *   - Per-station commodity markets with supply, demand, and price floors/ceilings
 *   - Dynamic price updates driven by buy/sell activity
 *   - Trade routes connecting two markets (auto-arbitrage simulation)
 *   - Faction-controlled supply chains that modulate base prices
 *
 * Typical usage:
 * @code
 *   MarketSystem market;
 *   market.Init();
 *   uint32_t stationA = market.CreateMarket("Sigma Station");
 *   market.AddCommodity(stationA, "Iron Ore", 10.f, 500);  // base price 10, 500 units
 *   market.Sell(stationA, "Iron Ore", 50);  // player sells 50 units
 *   float price = market.GetPrice(stationA, "Iron Ore");
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Runtime {

// ── Commodity listing on a single market ─────────────────────────────────────

struct CommodityListing {
    std::string commodity;
    float       basePrice{1.0f};
    float       currentPrice{1.0f};
    float       priceFloor{0.1f};
    float       priceCeiling{1000.0f};
    int32_t     supply{0};      ///< units in stock (negative = shortage)
    int32_t     demand{0};      ///< units wanted per day
    float       volatility{0.1f}; ///< price elasticity coefficient
};

// ── Market node (one station / outpost) ──────────────────────────────────────

struct Market {
    uint32_t    id{0};
    std::string name;
    uint32_t    factionId{0};  ///< controlling faction (0 = neutral)
    std::unordered_map<std::string, CommodityListing> listings;
};

// ── Trade route between two markets ──────────────────────────────────────────

struct TradeRoute {
    uint32_t    id{0};
    uint32_t    marketA{0};
    uint32_t    marketB{0};
    std::string commodity;
    float       distanceAU{1.0f};  ///< affects travel-cost and profit
    bool        active{true};
};

// ── Transaction record ────────────────────────────────────────────────────────

struct Transaction {
    uint32_t    marketId{0};
    std::string commodity;
    int32_t     quantity{0};   ///< positive = buy, negative = sell
    float       pricePerUnit{0.0f};
    float       totalValue{0.0f};
    double      timestamp{0.0};
};

using PriceChangedFn   = std::function<void(uint32_t marketId,
                                             const std::string& commodity,
                                             float newPrice)>;
using TradeCompletedFn = std::function<void(const Transaction&)>;

// ── MarketSystem ─────────────────────────────────────────────────────────────

class MarketSystem {
public:
    void Init();

    // ── Markets ───────────────────────────────────────────────────────────
    uint32_t CreateMarket(const std::string& name, uint32_t factionId = 0);
    void     DestroyMarket(uint32_t marketId);
    Market*  GetMarket(uint32_t marketId);
    std::vector<uint32_t> AllMarketIds() const;

    // ── Commodities ───────────────────────────────────────────────────────
    /// Add or update a commodity listing on a market.
    void AddCommodity(uint32_t marketId, const std::string& commodity,
                      float basePrice, int32_t initialSupply,
                      int32_t dailyDemand = 10, float volatility = 0.1f);
    void RemoveCommodity(uint32_t marketId, const std::string& commodity);

    float    GetPrice(uint32_t marketId, const std::string& commodity) const;
    int32_t  GetSupply(uint32_t marketId, const std::string& commodity) const;

    // ── Transactions ──────────────────────────────────────────────────────
    /// Player buys `qty` units of commodity from a market. Returns total cost.
    float Buy(uint32_t marketId, const std::string& commodity, int32_t qty);

    /// Player sells `qty` units of commodity to a market. Returns total revenue.
    float Sell(uint32_t marketId, const std::string& commodity, int32_t qty);

    // ── Trade routes ──────────────────────────────────────────────────────
    uint32_t AddTradeRoute(uint32_t marketA, uint32_t marketB,
                           const std::string& commodity, float distanceAU = 1.0f);
    void     RemoveTradeRoute(uint32_t routeId);
    float    EstimateRouteProfit(uint32_t routeId) const;

    // ── Simulation tick ───────────────────────────────────────────────────
    /// Advance one in-game "day" — restocks, adjusts demand, propagates trade.
    void Tick(float deltaTimeSec);

    // ── Callbacks ─────────────────────────────────────────────────────────
    void SetPriceChangedCallback(PriceChangedFn fn);
    void SetTradeCompletedCallback(TradeCompletedFn fn);

    // ── Stats ─────────────────────────────────────────────────────────────
    size_t MarketCount()     const;
    size_t TradeRouteCount() const;
    const std::vector<Transaction>& GetTransactionLog() const;
    void ClearTransactionLog();

private:
    void UpdatePrice(Market& mkt, CommodityListing& listing, int32_t qtyDelta);
    void SimulateTradeRoute(const TradeRoute& route);

    uint32_t m_nextMarketId{1};
    uint32_t m_nextRouteId{1};
    float    m_dayAccum{0.0f};
    float    m_dayLengthSec{600.0f}; ///< 10 real-minutes = 1 in-game day

    std::unordered_map<uint32_t, Market>     m_markets;
    std::unordered_map<uint32_t, TradeRoute> m_routes;
    std::vector<Transaction>                 m_txLog;

    PriceChangedFn   m_onPriceChanged;
    TradeCompletedFn m_onTradeCompleted;
};

} // namespace Runtime
