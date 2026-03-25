/**
 * @file IntegrationTests.cpp
 * @brief Integration test cases for the NovaForge game systems (Phase 37.12).
 *
 * Each test boots the relevant subsystem, exercises it, and asserts expected
 * outcomes.  Run via:
 *   cd Builds/debug && ctest --output-on-failure -R integration
 */

#include "Tests/Integration/IntegrationTestRunner.h"

// ── System headers ────────────────────────────────────────────────────────────
#include "Runtime/Economy/MarketSystem.h"
#include "Runtime/Factions/FactionSystem.h"
#include "Runtime/Combat/CombatSystem.h"
#include "Runtime/Gameplay/ProgressionSystem.h"
#include "Runtime/Hazards/HazardSystem.h"
#include "Runtime/AI/FleetController.h"
#include "PCG/Narrative/NarrativeEngine.h"
#include "Builder/Assembly/Assembly.h"
#include "Builder/Parts/PartDef.h"
#include "Runtime/BuilderRuntime/BuilderRuntime.h"
#include "Runtime/SaveLoad/SaveLoad.h"
#include "Engine/Net/Rollback/RollbackNetcode.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

using namespace Tests;

// ── Economy ───────────────────────────────────────────────────────────────────

static void Test_Market_BuySell(bool& _passed, std::string& _failMsg) {
    Runtime::MarketSystem market;
    market.Init();
    uint32_t sid = market.CreateMarket("Test Station");
    IT_EXPECT(sid != 0, "CreateMarket returned 0");
    market.AddCommodity(sid, "Iron Ore", 10.0f, 100);
    IT_EXPECT_EQ(market.GetSupply(sid, "Iron Ore"), 100, "Initial supply != 100");
    float cost = market.Buy(sid, "Iron Ore", 10);
    IT_EXPECT(cost > 0.0f, "Buy cost should be > 0");
    IT_EXPECT_EQ(market.GetSupply(sid, "Iron Ore"), 90, "Supply after buy should be 90");
    float revenue = market.Sell(sid, "Iron Ore", 5);
    IT_EXPECT(revenue > 0.0f, "Sell revenue should be > 0");
    IT_EXPECT_EQ(market.GetSupply(sid, "Iron Ore"), 95, "Supply after sell should be 95");
}

static void Test_Market_TradeRoute(bool& _passed, std::string& _failMsg) {
    Runtime::MarketSystem market;
    market.Init();
    uint32_t a = market.CreateMarket("A");
    uint32_t b = market.CreateMarket("B");
    market.AddCommodity(a, "Fuel", 5.0f,  1000);
    market.AddCommodity(b, "Fuel", 15.0f,   10);
    uint32_t rid = market.AddTradeRoute(a, b, "Fuel", 2.0f);
    float profit = market.EstimateRouteProfit(rid);
    IT_EXPECT(profit > 0.0f, "Trade route from cheap to expensive should be profitable");
}

// ── Factions ──────────────────────────────────────────────────────────────────

static void Test_Factions_Reputation(bool& _passed, std::string& _failMsg) {
    Runtime::FactionSystem factions;
    factions.Init();
    uint32_t miners = factions.CreateFaction("Miners");
    IT_EXPECT(factions.GetReputation(miners) == 0.0f, "Initial rep should be 0");
    factions.AddReputation(miners, 300.0f, "quest");
    IT_EXPECT(factions.GetStance(miners) == Runtime::PlayerStance::Friendly,
              "Rep 300 should be Friendly");
    factions.AddReputation(miners, 300.0f, "donation");
    IT_EXPECT(factions.GetStance(miners) == Runtime::PlayerStance::Allied,
              "Rep 600 should be Allied");
}

static void Test_Factions_Diplomacy(bool& _passed, std::string& _failMsg) {
    Runtime::FactionSystem factions;
    factions.Init();
    uint32_t a = factions.CreateFaction("A");
    uint32_t b = factions.CreateFaction("B");
    factions.SetDiplomacy(a, b, Runtime::DiplomaticState::War);
    IT_EXPECT(factions.IsAtWar(a, b), "A and B should be at war");
    IT_EXPECT(!factions.IsAllied(a, b), "A and B should not be allied");
    factions.SetDiplomacy(a, b, Runtime::DiplomaticState::Alliance);
    IT_EXPECT(factions.IsAllied(a, b), "A and B should be allied");
}

// ── Combat ────────────────────────────────────────────────────────────────────

static void Test_Combat_FireDamage(bool& _passed, std::string& _failMsg) {
    Runtime::CombatSystem combat;
    combat.Init();
    Runtime::WeaponDef laser{"Laser", Runtime::WeaponType::Laser, 50.0f, 500.0f, 0.5f, 360.0f};
    Runtime::CombatEntityDesc atk{"Attacker", 0,0,0, 0, 500.0f, 200.0f, 5.0f, {laser}};
    Runtime::CombatEntityDesc tgt{"Target",   100,0,0, 0, 500.0f, 200.0f, 5.0f, {}};
    uint32_t atkId = combat.Register(atk);
    uint32_t tgtId = combat.Register(tgt);
    bool fired = combat.FireWeapon(atkId, 0, tgtId);
    IT_EXPECT(fired, "FireWeapon should succeed within range");
    auto* target = combat.GetEntity(tgtId);
    IT_EXPECT(target != nullptr, "Target entity should exist");
    // Shields should have absorbed the 50 damage (200/4 = 50 per facing)
    float totalShield = target->shields.Total();
    IT_EXPECT(totalShield < 200.0f, "Shields should have taken some damage");
}

static void Test_Combat_ArcCheck(bool& _passed, std::string& _failMsg) {
    Runtime::CombatSystem combat;
    combat.Init();
    // Weapon with 90° arc (±45° from forward)
    Runtime::WeaponDef gun{"Gun", Runtime::WeaponType::Railgun, 100.0f, 1000.0f, 1.0f, 90.0f};
    Runtime::CombatEntityDesc atk{"Ship", 0,0,0, 0, 500.0f, 0.0f, 0.0f, {gun}};
    Runtime::CombatEntityDesc front{"Front", 0,0,-100, 0, 300.0f, 0.0f, 0.0f, {}};
    Runtime::CombatEntityDesc side{"Side",   100,0,0,  0, 300.0f, 0.0f, 0.0f, {}};
    uint32_t atkId   = combat.Register(atk);
    uint32_t frontId = combat.Register(front);
    uint32_t sideId  = combat.Register(side);
    IT_EXPECT( combat.IsInWeaponArc(atkId, 0, frontId), "Front target should be in arc");
    IT_EXPECT(!combat.IsInWeaponArc(atkId, 0, sideId),  "Side target should be out of arc");
}

// ── Progression ───────────────────────────────────────────────────────────────

static void Test_Progression_LevelUp(bool& _passed, std::string& _failMsg) {
    Runtime::ProgressionSystem prog;
    prog.Init();
    IT_EXPECT_EQ(prog.GetLevel(), 1u, "Initial level should be 1");
    prog.AddXP(Runtime::XPSource::Combat, 500.0f);  // Level 2 requires 400 XP
    IT_EXPECT(prog.GetLevel() >= 2, "Should have levelled up");
    IT_EXPECT(prog.GetTalentPoints() >= 1u, "Should have at least 1 talent point");
}

static void Test_Progression_TechTree(bool& _passed, std::string& _failMsg) {
    Runtime::ProgressionSystem prog;
    prog.Init();
    uint32_t hull = prog.AddTechNode("Hull Reinforcement", "More HP", 1);
    uint32_t armor = prog.AddTechNode("Advanced Armour", "Shield boost", 2, {hull});
    IT_EXPECT(!prog.CanUnlockTech(armor), "Armour needs Hull prereq first");
    prog.AddXP(Runtime::XPSource::Quest, 500.0f);  // get a talent point
    bool ok = prog.SpendTalentPoint(hull);
    IT_EXPECT(ok, "Should be able to unlock Hull Reinforcement");
    IT_EXPECT(prog.CanUnlockTech(armor), "Armour should now be unlockable");
}

// ── Hazards ───────────────────────────────────────────────────────────────────

static void Test_Hazards_EnterExit(bool& _passed, std::string& _failMsg) {
    Runtime::HazardSystem hazards;
    hazards.Init();
    Runtime::HazardZone rad;
    rad.type      = Runtime::HazardType::Radiation;
    rad.cx = 0; rad.cy = 0; rad.cz = 0;
    rad.radius    = 50.0f;
    rad.intensity = 1.0f;
    uint32_t hid = hazards.AddHazard(rad);

    float px = 0, py = 0, pz = 0;
    hazards.RegisterEntity(1, &px, &py, &pz);

    bool entered = false, exited = false;
    hazards.SetEnteredCallback([&](uint32_t, uint32_t) { entered = true; });
    hazards.SetExitedCallback([&](uint32_t,  uint32_t) { exited  = true; });

    hazards.Tick(0.016f);
    IT_EXPECT(entered, "Entity should have entered the radiation zone");

    px = 200.0f;  // move outside
    hazards.Tick(0.016f);
    IT_EXPECT(exited, "Entity should have exited the radiation zone");
}

// ── Fleet ─────────────────────────────────────────────────────────────────────

static void Test_Fleet_Patrol(bool& _passed, std::string& _failMsg) {
    Runtime::FleetController fc;
    fc.Init();
    uint32_t fid = fc.CreateFleet("Alpha Wing", Runtime::FleetFormation::Line);
    IT_EXPECT(fid != 0, "CreateFleet should return non-zero id");
    Runtime::AIShipAgent ship{1, 0.0f, 0.0f, 0.0f, 200.0f, 200.0f, 50.0f};
    fc.AddShipToFleet(fid, ship);
    Runtime::Waypoint wp0{0,0,0}, wp1{500,0,0}, wp2{500,0,500};
    fc.SetPatrolRoute(fid, {wp0, wp1, wp2}, true);
    // Tick enough to move towards first waypoint
    for (int i = 0; i < 100; ++i) fc.Tick(0.1f);
    auto* fleet = fc.GetFleet(fid);
    IT_EXPECT(fleet != nullptr, "Fleet should exist");
    // Ship should have moved from 0,0,0
    float finalX = fleet->ships[0].posX;
    IT_EXPECT(finalX > 0.0f, "Ship should have moved towards first waypoint");
}

// ── Narrative ─────────────────────────────────────────────────────────────────

static void Test_Narrative_Substitution(bool& _passed, std::string& _failMsg) {
    PCG::NarrativeEngine nar;
    nar.Init();
    nar.SetContext("player_name",     std::string("Commander"));
    nar.SetContext("faction_rep",     350.0f);

    PCG::DialogueTemplate tmpl;
    tmpl.id = "greeting";
    PCG::DialogueVariant v;
    v.text   = "Hello {player_name}, your rep is {faction_rep}.";
    v.weight = 1.0f;
    tmpl.variants.push_back(v);
    nar.AddDialogueTemplate(tmpl);

    std::string line = nar.GenerateLine("greeting");
    IT_EXPECT(line.find("Commander") != std::string::npos,
              "Substitution should replace {player_name}");
    IT_EXPECT(line.find("350") != std::string::npos,
              "Substitution should replace {faction_rep}");
}

static void Test_Narrative_EventFlags(bool& _passed, std::string& _failMsg) {
    PCG::NarrativeEngine nar;
    nar.Init();
    PCG::NarrativeEvent evt;
    evt.name        = "first_delivery";
    evt.setsFlags   = {"delivered_first_cargo"};
    evt.trigger     = [](const PCG::NarrativeContext& ctx) {
        return ctx.flags.count("cargo_accepted") > 0;
    };
    nar.AddEvent(std::move(evt));

    nar.EvaluateEvents();
    IT_EXPECT(!nar.HasFlag("delivered_first_cargo"), "Flag should not be set before trigger");
    nar.SetFlag("cargo_accepted");
    nar.EvaluateEvents();
    IT_EXPECT(nar.HasFlag("delivered_first_cargo"), "Flag should be set after trigger fires");
}

// ── Builder round-trip ────────────────────────────────────────────────────────

static void Test_Builder_PlaceWeld(bool& _passed, std::string& _failMsg) {
    Builder::PartLibrary lib;
    Builder::PartDef def;
    def.id   = 1;
    def.name = "Hull Block";
    def.stats.hitpoints = 100.0f;
    Builder::SnapPoint sp0; sp0.id = 0; sp0.compatibleTypes = {"hull"};
    Builder::SnapPoint sp1; sp1.id = 1; sp1.compatibleTypes = {"hull"};
    def.snapPoints = {sp0, sp1};
    lib.Register(def);

    Builder::Assembly assembly;
    Runtime::BuilderRuntime runtime;
    runtime.Init(&assembly, &lib);
    runtime.BeginBuild();

    float t1[16] = {1,0,0,0,0,1,0,0,0,0,1,0, 0,0,0,1};
    float t2[16] = {1,0,0,0,0,1,0,0,0,0,1,0, 5,0,0,1};
    uint32_t inst1 = runtime.PlacePart(1, t1);
    uint32_t inst2 = runtime.PlacePart(1, t2);
    IT_EXPECT(inst1 != 0, "First part should be placed");
    IT_EXPECT(inst2 != 0, "Second part should be placed");
    IT_EXPECT_EQ(assembly.PartCount(), (size_t)2, "Assembly should have 2 parts");

    bool welded = runtime.WeldParts(inst1, 1, inst2, 0);
    IT_EXPECT(welded, "Parts should weld successfully");
    IT_EXPECT_EQ(assembly.LinkCount(), (size_t)1, "Assembly should have 1 link");

    runtime.EndBuild();
}

// ── SaveLoad round-trip ───────────────────────────────────────────────────────

static void Test_SaveLoad_RoundTrip(bool& _passed, std::string& _failMsg) {
    Runtime::SaveLoad::SaveSystem saveSystem;
    Runtime::SaveLoad::SaveData data;
    data.header.version   = "1.0";
    data.header.sceneName = "TestScene";
    data.jsonPayload      = "{\"test\":true}";

    const std::string dir = "/tmp/integration_test_save";
    bool ok = saveSystem.QuickSave(data, dir);
    IT_EXPECT(ok, "QuickSave should succeed");

    Runtime::SaveLoad::SaveData loaded;
    bool loadOk = saveSystem.QuickLoad(dir, loaded);
    IT_EXPECT(loadOk, "QuickLoad should succeed");
    IT_EXPECT(loaded.jsonPayload == "{\"test\":true}", "Payload should round-trip");
}

// ── Rollback netcode ──────────────────────────────────────────────────────────

static void Test_Rollback_NoRollbackNeeded(bool& _passed, std::string& _failMsg) {
    Engine::net::RollbackNetcode rb;
    rb.Init(60, 8);
    IT_EXPECT(!rb.NeedsRollback(), "Should not need rollback initially");
    rb.RecordLocalInput(1, {0x01});
    Engine::net::PeerInput remote;
    remote.peerId     = 1;
    remote.tick       = 1;
    remote.inputBytes = {0x01}; // same as local
    remote.confirmed  = true;
    rb.ConfirmRemoteInput(remote);
    IT_EXPECT(!rb.NeedsRollback(), "Matching inputs should not trigger rollback");
}

static void Test_Rollback_TriggerRollback(bool& _passed, std::string& _failMsg) {
    Engine::net::RollbackNetcode rb;
    rb.Init(60, 8);
    rb.SetSaveStateFn([]() -> std::vector<uint8_t> { return {0x01, 0x02}; });
    rb.SetLoadStateFn([](const std::vector<uint8_t>&) {});
    rb.RecordLocalInput(1, {0x01});
    rb.SaveState(1);
    Engine::net::PeerInput remote;
    remote.peerId     = 1;
    remote.tick       = 1;
    remote.inputBytes = {0xFF}; // differs from predicted
    remote.confirmed  = true;
    rb.ConfirmRemoteInput(remote);
    IT_EXPECT(rb.NeedsRollback(), "Differing inputs should trigger rollback");
    IT_EXPECT_EQ(rb.PendingRollbacks(), 1u, "One tick should need re-simulation");
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    IntegrationTestRunner& runner = GlobalRunner();

    runner.AddTest("Market: buy/sell",           Test_Market_BuySell);
    runner.AddTest("Market: trade route profit", Test_Market_TradeRoute);
    runner.AddTest("Factions: reputation stances", Test_Factions_Reputation);
    runner.AddTest("Factions: diplomacy",        Test_Factions_Diplomacy);
    runner.AddTest("Combat: fire & damage",      Test_Combat_FireDamage);
    runner.AddTest("Combat: weapon arc",         Test_Combat_ArcCheck);
    runner.AddTest("Progression: level-up",      Test_Progression_LevelUp);
    runner.AddTest("Progression: tech tree",     Test_Progression_TechTree);
    runner.AddTest("Hazards: enter/exit",        Test_Hazards_EnterExit);
    runner.AddTest("Fleet: patrol",              Test_Fleet_Patrol);
    runner.AddTest("Narrative: substitution",    Test_Narrative_Substitution);
    runner.AddTest("Narrative: event flags",     Test_Narrative_EventFlags);
    runner.AddTest("Builder: place & weld",      Test_Builder_PlaceWeld);
    runner.AddTest("SaveLoad: round-trip",       Test_SaveLoad_RoundTrip);
    runner.AddTest("Rollback: no rollback needed", Test_Rollback_NoRollbackNeeded);
    runner.AddTest("Rollback: trigger rollback", Test_Rollback_TriggerRollback);

    int failures = runner.Run();
    runner.PrintSummary();
    return failures == 0 ? 0 : 1;
}
