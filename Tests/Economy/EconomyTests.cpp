#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Game/Gameplay/Economy/ResourceRegistry.h"
#include "Game/Gameplay/Economy/TradeMarket.h"
#include "Game/Gameplay/Manufacturing/ManufacturingQueue.h"
#include "Game/Gameplay/Station/StationServices.h"
#include "Game/Interaction/Inventory.h"
#include "Game/Interaction/RigState.h"

using namespace NF::Game::Gameplay;
using Catch::Matchers::WithinAbs;

static constexpr float kEps = 1e-3f;

// =============================================================================
// ResourceRegistry
// =============================================================================

TEST_CASE("ResourceRegistry: Stone definition has correct base price", "[ResourceRegistry]") {
    ResourceRegistry reg;
    const auto& def = reg.Get(NF::Game::ResourceType::Stone);
    REQUIRE(def.type == NF::Game::ResourceType::Stone);
    REQUIRE_THAT(def.basePrice, WithinAbs(1.f, kEps));
    REQUIRE(def.tradeable);
}

TEST_CASE("ResourceRegistry: Metal is craftable", "[ResourceRegistry]") {
    ResourceRegistry reg;
    REQUIRE(reg.Get(NF::Game::ResourceType::Metal).craftable);
}

TEST_CASE("ResourceRegistry: None type is not tradeable", "[ResourceRegistry]") {
    ResourceRegistry reg;
    REQUIRE(!reg.Get(NF::Game::ResourceType::None).tradeable);
}

TEST_CASE("ResourceRegistry: FindByName returns correct entry", "[ResourceRegistry]") {
    ResourceRegistry reg;
    const auto& def = reg.FindByName("Ore");
    REQUIRE(def.type == NF::Game::ResourceType::Ore);
}

TEST_CASE("ResourceRegistry: FindByName with unknown name returns None", "[ResourceRegistry]") {
    ResourceRegistry reg;
    REQUIRE(reg.FindByName("Unobtainium").type == NF::Game::ResourceType::None);
}

TEST_CASE("ResourceRegistry: out-of-range Get returns None", "[ResourceRegistry]") {
    ResourceRegistry reg;
    REQUIRE(reg.Get(static_cast<NF::Game::ResourceType>(200)).type
            == NF::Game::ResourceType::None);
}

// =============================================================================
// TradeMarket
// =============================================================================

TEST_CASE("TradeMarket: Initialize seeds stock from registry", "[TradeMarket]") {
    ResourceRegistry reg;
    TradeMarket mkt;
    mkt.Initialize(reg, 50);
    REQUIRE(mkt.GetStock(NF::Game::ResourceType::Stone) == 50);
    REQUIRE(mkt.GetStock(NF::Game::ResourceType::Ore)   == 50);
}

TEST_CASE("TradeMarket: Buy reduces stock and deducts credits", "[TradeMarket]") {
    ResourceRegistry reg;
    TradeMarket mkt;
    mkt.Initialize(reg, 50);
    mkt.SetCredits(100.f);
    NF::Game::Inventory inv;
    REQUIRE(mkt.Buy(NF::Game::ResourceType::Stone, 3, inv) == TradeResult::Success);
    REQUIRE(mkt.GetStock(NF::Game::ResourceType::Stone) == 47);
    REQUIRE(inv.GetCount(NF::Game::ResourceType::Stone) == 3);
    REQUIRE(mkt.GetCredits() < 100.f);
}

TEST_CASE("TradeMarket: Buy fails when insufficient funds", "[TradeMarket]") {
    ResourceRegistry reg;
    TradeMarket mkt;
    mkt.Initialize(reg, 50);
    mkt.SetCredits(0.f);
    NF::Game::Inventory inv;
    REQUIRE(mkt.Buy(NF::Game::ResourceType::Ore, 1, inv) == TradeResult::InsufficientFunds);
    REQUIRE(inv.GetCount(NF::Game::ResourceType::Ore) == 0);
}

TEST_CASE("TradeMarket: Buy fails when insufficient stock", "[TradeMarket]") {
    ResourceRegistry reg;
    TradeMarket mkt;
    mkt.Initialize(reg, 2);
    mkt.SetCredits(10000.f);
    NF::Game::Inventory inv;
    REQUIRE(mkt.Buy(NF::Game::ResourceType::Stone, 5, inv) == TradeResult::InsufficientStock);
}

TEST_CASE("TradeMarket: Sell moves items from inventory to market and adds credits", "[TradeMarket]") {
    ResourceRegistry reg;
    TradeMarket mkt;
    mkt.Initialize(reg, 0);
    mkt.SetCredits(0.f);
    NF::Game::Inventory inv;
    inv.AddItem(NF::Game::ResourceType::Ore, 5);

    REQUIRE(mkt.Sell(NF::Game::ResourceType::Ore, 5, inv) == TradeResult::Success);
    REQUIRE(inv.GetCount(NF::Game::ResourceType::Ore) == 0);
    REQUIRE(mkt.GetCredits() > 0.f);
    REQUIRE(mkt.GetStock(NF::Game::ResourceType::Ore) == 5);
}

TEST_CASE("TradeMarket: Sell fails when inventory has insufficient items", "[TradeMarket]") {
    ResourceRegistry reg;
    TradeMarket mkt;
    mkt.Initialize(reg, 0);
    NF::Game::Inventory inv;
    inv.AddItem(NF::Game::ResourceType::Ore, 1);
    REQUIRE(mkt.Sell(NF::Game::ResourceType::Ore, 5, inv) == TradeResult::InsufficientItems);
    REQUIRE(inv.GetCount(NF::Game::ResourceType::Ore) == 1);
}

TEST_CASE("TradeMarket: price increases after buying (demand)", "[TradeMarket]") {
    ResourceRegistry reg;
    TradeMarket mkt;
    mkt.Initialize(reg, 100);
    mkt.SetCredits(10000.f);
    const float before = mkt.GetPrice(NF::Game::ResourceType::Stone);
    NF::Game::Inventory inv;
    mkt.Buy(NF::Game::ResourceType::Stone, 1, inv);
    REQUIRE(mkt.GetPrice(NF::Game::ResourceType::Stone) > before);
}

TEST_CASE("TradeMarket: price decreases after selling (supply)", "[TradeMarket]") {
    ResourceRegistry reg;
    TradeMarket mkt;
    mkt.Initialize(reg, 0);
    NF::Game::Inventory inv;
    inv.AddItem(NF::Game::ResourceType::Stone, 5);
    const float before = mkt.GetPrice(NF::Game::ResourceType::Stone);
    mkt.Sell(NF::Game::ResourceType::Stone, 1, inv);
    REQUIRE(mkt.GetPrice(NF::Game::ResourceType::Stone) < before);
}

TEST_CASE("TradeMarket: price is clamped to floor when selling repeatedly", "[TradeMarket]") {
    ResourceRegistry reg;
    TradeMarket mkt;
    mkt.Initialize(reg, 0);
    NF::Game::Inventory inv;
    inv.AddItem(NF::Game::ResourceType::Stone, 16); // enough items for repeated selling

    for (int i = 0; i < 30; ++i) {
        if (inv.HasItem(NF::Game::ResourceType::Stone))
            mkt.Sell(NF::Game::ResourceType::Stone, 1, inv);
    }
    const float basePrice = reg.Get(NF::Game::ResourceType::Stone).basePrice;
    REQUIRE(mkt.GetPrice(NF::Game::ResourceType::Stone)
            >= basePrice * TradeMarket::kPriceFloor - kEps);
}

// =============================================================================
// ManufacturingQueue
// =============================================================================

TEST_CASE("ManufacturingQueue: has built-in recipes", "[ManufacturingQueue]") {
    ManufacturingQueue mq;
    REQUIRE(mq.RecipeCount() > 0);
}

TEST_CASE("ManufacturingQueue: FindRecipe returns correct recipe for Metal", "[ManufacturingQueue]") {
    ManufacturingQueue mq;
    const CraftRecipe* r = mq.FindRecipe(NF::Game::ResourceType::Metal);
    REQUIRE(r != nullptr);
    REQUIRE(r->output == NF::Game::ResourceType::Metal);
    REQUIRE(r->ingredientCount >= 1);
}

TEST_CASE("ManufacturingQueue: Enqueue deducts ingredients", "[ManufacturingQueue]") {
    ManufacturingQueue mq;
    NF::Game::Inventory inv;
    inv.AddItem(NF::Game::ResourceType::Stone, 10);
    inv.AddItem(NF::Game::ResourceType::Ore,   10);

    const JobId id = mq.Enqueue(NF::Game::ResourceType::Metal, inv);
    REQUIRE(id != kInvalidJobId);
    // Ingredients deducted.
    REQUIRE(inv.GetCount(NF::Game::ResourceType::Stone) < 10);
}

TEST_CASE("ManufacturingQueue: Enqueue fails without sufficient ingredients", "[ManufacturingQueue]") {
    ManufacturingQueue mq;
    NF::Game::Inventory inv; // empty
    REQUIRE(mq.Enqueue(NF::Game::ResourceType::Metal, inv) == kInvalidJobId);
}

TEST_CASE("ManufacturingQueue: Tick completes job after duration", "[ManufacturingQueue]") {
    ManufacturingQueue mq;
    NF::Game::Inventory inv;
    inv.AddItem(NF::Game::ResourceType::Stone, 10);
    inv.AddItem(NF::Game::ResourceType::Ore,   10);

    const CraftRecipe* r = mq.FindRecipe(NF::Game::ResourceType::Metal);
    REQUIRE(r != nullptr);

    mq.Enqueue(NF::Game::ResourceType::Metal, inv);
    REQUIRE(!mq.HasCompletedJob());
    mq.Tick(r->duration + 0.1f);
    REQUIRE(mq.HasCompletedJob());
}

TEST_CASE("ManufacturingQueue: CollectOutput deposits results and removes job", "[ManufacturingQueue]") {
    ManufacturingQueue mq;
    NF::Game::Inventory inv;
    inv.AddItem(NF::Game::ResourceType::Stone, 10);
    inv.AddItem(NF::Game::ResourceType::Ore,   10);

    const CraftRecipe* r = mq.FindRecipe(NF::Game::ResourceType::Metal);
    const uint32_t beforeMetal = inv.GetCount(NF::Game::ResourceType::Metal);
    mq.Enqueue(NF::Game::ResourceType::Metal, inv);
    mq.Tick(r->duration + 0.1f);

    REQUIRE(mq.CollectOutput(inv));
    REQUIRE(inv.GetCount(NF::Game::ResourceType::Metal) > beforeMetal);
    REQUIRE(mq.IsIdle());
}

TEST_CASE("ManufacturingQueue: second job starts after first collected", "[ManufacturingQueue]") {
    ManufacturingQueue mq;
    NF::Game::Inventory inv;
    inv.AddItem(NF::Game::ResourceType::Stone, 20);
    inv.AddItem(NF::Game::ResourceType::Ore,   20);

    const CraftRecipe* r = mq.FindRecipe(NF::Game::ResourceType::Metal);
    mq.Enqueue(NF::Game::ResourceType::Metal, inv);
    mq.Enqueue(NF::Game::ResourceType::Metal, inv);
    REQUIRE(mq.PendingCount() == 2);

    mq.Tick(r->duration + 0.1f);
    REQUIRE(mq.HasCompletedJob());
    mq.CollectOutput(inv);
    REQUIRE(mq.PendingCount() == 1);
}

// =============================================================================
// StationServices
// =============================================================================

TEST_CASE("StationServices: Dock and Undock transitions", "[StationServices]") {
    StationServices st("Outpost Alpha");
    REQUIRE(!st.IsDocked());
    REQUIRE(st.Dock()   == StationServiceResult::Success);
    REQUIRE(st.IsDocked());
    REQUIRE(st.Dock()   == StationServiceResult::AlreadyDocked);
    REQUIRE(st.Undock() == StationServiceResult::Success);
    REQUIRE(!st.IsDocked());
    REQUIRE(st.Undock() == StationServiceResult::NotDocked);
}

TEST_CASE("StationServices: Repair restores rig health when docked with credits", "[StationServices]") {
    ResourceRegistry reg;
    StationServices st("Station");
    st.GetMarket().Initialize(reg, 0);
    st.GetMarket().SetCredits(1000.f);
    st.Dock();

    NF::Game::RigState rig;
    rig.TakeDamage(50.f);
    REQUIRE_THAT(rig.GetHealth(), WithinAbs(50.f, kEps));

    REQUIRE(st.Repair(rig) == StationServiceResult::Success);
    REQUIRE_THAT(rig.GetHealth(), WithinAbs(NF::Game::RigState::kMaxHealth, kEps));
    REQUIRE(st.GetMarket().GetCredits() < 1000.f);
}

TEST_CASE("StationServices: Repair fails when not docked", "[StationServices]") {
    StationServices st("Station");
    NF::Game::RigState rig;
    rig.TakeDamage(10.f);
    REQUIRE(st.Repair(rig) == StationServiceResult::NotDocked);
}

TEST_CASE("StationServices: Repair returns AlreadyFullHealth when rig is intact", "[StationServices]") {
    ResourceRegistry reg;
    StationServices st("Station");
    st.GetMarket().Initialize(reg, 0);
    st.GetMarket().SetCredits(1000.f);
    st.Dock();
    NF::Game::RigState rig; // full health
    REQUIRE(st.Repair(rig) == StationServiceResult::AlreadyFullHealth);
}

TEST_CASE("StationServices: Refuel restores rig energy when docked with credits", "[StationServices]") {
    ResourceRegistry reg;
    StationServices st("Station");
    st.GetMarket().Initialize(reg, 0);
    st.GetMarket().SetCredits(1000.f);
    st.Dock();

    NF::Game::RigState rig;
    (void)rig.ConsumeEnergy(40.f);
    REQUIRE(st.Refuel(rig) == StationServiceResult::Success);
    REQUIRE_THAT(rig.GetEnergy(), WithinAbs(NF::Game::RigState::kMaxEnergy, kEps));
}

TEST_CASE("StationServices: Tick advances manufacturing queue", "[StationServices]") {
    StationServices st("Forge");
    st.Dock();
    NF::Game::Inventory inv;
    inv.AddItem(NF::Game::ResourceType::Stone, 10);
    inv.AddItem(NF::Game::ResourceType::Ore,   10);

    const CraftRecipe* r = st.GetFactory().FindRecipe(NF::Game::ResourceType::Metal);
    REQUIRE(r != nullptr);
    st.GetFactory().Enqueue(NF::Game::ResourceType::Metal, inv);
    st.Tick(r->duration + 0.1f);
    REQUIRE(st.GetFactory().HasCompletedJob());
}

TEST_CASE("StationServices: GetName returns station name", "[StationServices]") {
    StationServices st("Nova Station");
    REQUIRE(st.GetName() == "Nova Station");
}
