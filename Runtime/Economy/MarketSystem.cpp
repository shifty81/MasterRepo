#include "Runtime/Economy/MarketSystem.h"
#include <algorithm>
#include <cmath>

namespace Runtime {

void MarketSystem::Init() {
    m_markets.clear();
    m_routes.clear();
    m_txLog.clear();
    m_nextMarketId = 1;
    m_nextRouteId  = 1;
    m_dayAccum     = 0.0f;
}

// ── Markets ───────────────────────────────────────────────────────────────────

uint32_t MarketSystem::CreateMarket(const std::string& name, uint32_t factionId) {
    Market mkt;
    mkt.id        = m_nextMarketId++;
    mkt.name      = name;
    mkt.factionId = factionId;
    uint32_t id   = mkt.id;
    m_markets[id] = std::move(mkt);
    return id;
}

void MarketSystem::DestroyMarket(uint32_t marketId) { m_markets.erase(marketId); }

Market* MarketSystem::GetMarket(uint32_t marketId) {
    auto it = m_markets.find(marketId);
    return it != m_markets.end() ? &it->second : nullptr;
}

std::vector<uint32_t> MarketSystem::AllMarketIds() const {
    std::vector<uint32_t> ids;
    for (const auto& [id, _] : m_markets) ids.push_back(id);
    return ids;
}

// ── Commodities ───────────────────────────────────────────────────────────────

void MarketSystem::AddCommodity(uint32_t marketId, const std::string& commodity,
                                 float basePrice, int32_t initialSupply,
                                 int32_t dailyDemand, float volatility) {
    auto it = m_markets.find(marketId);
    if (it == m_markets.end()) return;
    CommodityListing cl;
    cl.commodity    = commodity;
    cl.basePrice    = basePrice;
    cl.currentPrice = basePrice;
    cl.supply       = initialSupply;
    cl.demand       = dailyDemand;
    cl.volatility   = volatility;
    cl.priceFloor   = basePrice * 0.1f;
    cl.priceCeiling = basePrice * 10.0f;
    it->second.listings[commodity] = cl;
}

void MarketSystem::RemoveCommodity(uint32_t marketId, const std::string& commodity) {
    auto it = m_markets.find(marketId);
    if (it != m_markets.end()) it->second.listings.erase(commodity);
}

float MarketSystem::GetPrice(uint32_t marketId, const std::string& commodity) const {
    auto mit = m_markets.find(marketId);
    if (mit == m_markets.end()) return 0.0f;
    auto lit = mit->second.listings.find(commodity);
    return lit != mit->second.listings.end() ? lit->second.currentPrice : 0.0f;
}

int32_t MarketSystem::GetSupply(uint32_t marketId, const std::string& commodity) const {
    auto mit = m_markets.find(marketId);
    if (mit == m_markets.end()) return 0;
    auto lit = mit->second.listings.find(commodity);
    return lit != mit->second.listings.end() ? lit->second.supply : 0;
}

// ── Transactions ──────────────────────────────────────────────────────────────

float MarketSystem::Buy(uint32_t marketId, const std::string& commodity, int32_t qty) {
    auto mit = m_markets.find(marketId);
    if (mit == m_markets.end()) return 0.0f;
    auto lit = mit->second.listings.find(commodity);
    if (lit == mit->second.listings.end()) return 0.0f;
    CommodityListing& cl = lit->second;
    int32_t actual = std::min(qty, cl.supply);
    if (actual <= 0) return 0.0f;
    float total = cl.currentPrice * actual;
    cl.supply -= actual;
    UpdatePrice(mit->second, cl, -actual); // buying reduces supply → price up
    Transaction tx{ marketId, commodity, actual, cl.currentPrice, total, 0.0 };
    m_txLog.push_back(tx);
    if (m_onTradeCompleted) m_onTradeCompleted(tx);
    return total;
}

float MarketSystem::Sell(uint32_t marketId, const std::string& commodity, int32_t qty) {
    auto mit = m_markets.find(marketId);
    if (mit == m_markets.end()) return 0.0f;
    auto& listings = mit->second.listings;
    if (listings.find(commodity) == listings.end()) {
        // Auto-create listing if market doesn't carry it yet
        AddCommodity(marketId, commodity, 1.0f, 0, 0, 0.15f);
    }
    CommodityListing& cl = listings[commodity];
    float total = cl.currentPrice * qty;
    cl.supply += qty;
    UpdatePrice(mit->second, cl, qty); // selling increases supply → price down
    Transaction tx{ marketId, commodity, -qty, cl.currentPrice, total, 0.0 };
    m_txLog.push_back(tx);
    if (m_onTradeCompleted) m_onTradeCompleted(tx);
    return total;
}

// ── Trade routes ──────────────────────────────────────────────────────────────

uint32_t MarketSystem::AddTradeRoute(uint32_t marketA, uint32_t marketB,
                                      const std::string& commodity, float distanceAU) {
    TradeRoute r;
    r.id         = m_nextRouteId++;
    r.marketA    = marketA;
    r.marketB    = marketB;
    r.commodity  = commodity;
    r.distanceAU = distanceAU;
    uint32_t id  = r.id;
    m_routes[id] = std::move(r);
    return id;
}

void MarketSystem::RemoveTradeRoute(uint32_t routeId) { m_routes.erase(routeId); }

float MarketSystem::EstimateRouteProfit(uint32_t routeId) const {
    auto rit = m_routes.find(routeId);
    if (rit == m_routes.end()) return 0.0f;
    const TradeRoute& r = rit->second;
    float priceA = GetPrice(r.marketA, r.commodity);
    float priceB = GetPrice(r.marketB, r.commodity);
    float spread = std::abs(priceB - priceA);
    float travelCost = r.distanceAU * 0.5f; // 0.5 credits per AU per unit
    return spread - travelCost;
}

// ── Simulation tick ───────────────────────────────────────────────────────────

void MarketSystem::Tick(float deltaTimeSec) {
    m_dayAccum += deltaTimeSec;
    if (m_dayAccum < m_dayLengthSec) return;
    m_dayAccum -= m_dayLengthSec;

    // Restock markets and apply daily demand pressure
    for (auto& [id, mkt] : m_markets) {
        for (auto& [name, cl] : mkt.listings) {
            // Natural restock towards a baseline supply
            int32_t restock = std::max(0, cl.demand - cl.supply) / 2;
            if (restock > 0) cl.supply += restock;
            // Natural price drift back towards base
            float diff = cl.basePrice - cl.currentPrice;
            cl.currentPrice += diff * 0.05f; // 5% drift per day
            cl.currentPrice = std::clamp(cl.currentPrice, cl.priceFloor, cl.priceCeiling);
        }
    }

    // Simulate trade route flows
    for (const auto& [id, route] : m_routes)
        if (route.active) SimulateTradeRoute(route);
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void MarketSystem::SetPriceChangedCallback(PriceChangedFn fn)   { m_onPriceChanged   = std::move(fn); }
void MarketSystem::SetTradeCompletedCallback(TradeCompletedFn fn){ m_onTradeCompleted = std::move(fn); }

// ── Stats ─────────────────────────────────────────────────────────────────────

size_t MarketSystem::MarketCount()     const { return m_markets.size(); }
size_t MarketSystem::TradeRouteCount() const { return m_routes.size(); }
const std::vector<Transaction>& MarketSystem::GetTransactionLog() const { return m_txLog; }
void MarketSystem::ClearTransactionLog() { m_txLog.clear(); }

// ── Private helpers ───────────────────────────────────────────────────────────

void MarketSystem::UpdatePrice(Market& mkt, CommodityListing& cl, int32_t qtyDelta) {
    // Simple supply-demand: price rises as supply falls below demand
    float ratio = (cl.demand > 0)
        ? (float)cl.supply / (float)cl.demand
        : 1.0f;
    // ratio < 1 → shortage → higher price; ratio > 1 → surplus → lower price
    float targetPrice = cl.basePrice / std::max(0.01f, ratio);
    float change = (targetPrice - cl.currentPrice) * cl.volatility;
    cl.currentPrice = std::clamp(cl.currentPrice + change,
                                  cl.priceFloor, cl.priceCeiling);
    if (m_onPriceChanged)
        m_onPriceChanged(mkt.id, cl.commodity, cl.currentPrice);
}

void MarketSystem::SimulateTradeRoute(const TradeRoute& route) {
    // Move a small shipment from the cheaper market to the dearer one
    float priceA = GetPrice(route.marketA, route.commodity);
    float priceB = GetPrice(route.marketB, route.commodity);
    if (priceA <= 0.f || priceB <= 0.f) return;
    uint32_t srcId = (priceA < priceB) ? route.marketA : route.marketB;
    uint32_t dstId = (priceA < priceB) ? route.marketB : route.marketA;
    constexpr int32_t kShipmentSize = 10;
    // Silently move goods (no player involvement)
    auto* src = GetMarket(srcId);
    auto* dst = GetMarket(dstId);
    if (!src || !dst) return;
    auto slit = src->listings.find(route.commodity);
    if (slit == src->listings.end() || slit->second.supply < kShipmentSize) return;
    slit->second.supply -= kShipmentSize;
    auto& dstListing = dst->listings[route.commodity];
    dstListing.supply += kShipmentSize;
}

} // namespace Runtime
