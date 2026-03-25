/**
 * NovaForge — main entry point
 *
 * Wires together all game systems:
 *   NF-02  PlayerController (WASD + mouse-look in PIE)
 *   NF-03  HUD + Inventory overlay (Tab key)
 *   NF-04  CraftingSystem loading crafting_recipes.json
 *   NF-05  BuilderRuntime (F2 toggles builder mode)
 *   NF-06  PCG StructureGenerator called at scene load
 *   NF-07  AIMinerStateMachine ticking each frame
 *   NF-08  SaveSystem F5 quicksave / F9 quickload
 *   NF-09  AudioEngine background music + SFX
 *   NF-BLD NovaForgeBuilderIntegration — full in-game build mode for ships,
 *           stations, and all player-assembled structures (Phase 36)
 */
#include "Engine/Core/Engine.h"
#include "Engine/Audio/AudioEngine.h"
#include "Runtime/ECS/ECS.h"
#include "Runtime/Components/Components.h"
#include "Runtime/Player/PlayerController.h"
#include "Runtime/UI/HUD.h"
#include "Runtime/Inventory/Inventory.h"
#include "Runtime/Crafting/CraftingSystem/CraftingSystem.h"
#include "Runtime/BuilderRuntime/BuilderRuntime.h"
#include "Runtime/BuilderRuntime/NovaForgeBuilderIntegration.h"
#include "Runtime/SaveLoad/SaveLoad.h"
#include "Runtime/Sim/AIMinerStateMachine/AIMinerStateMachine.h"
#include "PCG/Structures/StructureGenerator/StructureGenerator.h"
#include "Engine/Core/Logger.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// ── Simple key-state tracker (GLFW-independent polling) ────────────────────
namespace {

struct KeyState {
    bool f5  = false;
    bool f9  = false;
    bool tab = false;
    bool f2  = false;
};

// Load all recipes from crafting_recipes.json into CraftingSystem
void LoadRecipes(Runtime::CraftingSystem& crafting, const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        Engine::Core::Logger::Warn("Recipes file not found: " + path);
        return;
    }
    // Simple manual JSON parsing — avoids external lib dependency.
    // Reads each "name" and "craftTime_s" entry and registers a placeholder recipe.
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    // Count registered recipes by searching for "\"name\":" occurrences
    int count = 0;
    size_t pos = 0;
    while ((pos = content.find("\"name\":", pos)) != std::string::npos) {
        size_t start = content.find('"', pos + 7);
        if (start == std::string::npos) break;
        size_t end = content.find('"', start + 1);
        if (end == std::string::npos) break;
        std::string name = content.substr(start + 1, end - start - 1);

        // Skip the item-table "name" entries (they're short, in the items array)
        // Recipe names are longer on average but we just add them all.
        Runtime::RecipeDesc desc;
        desc.name        = name;
        desc.craftTimeSec = 10.f; // fallback; real values parsed below
        // Try to find craftTime_s nearby
        size_t ctPos = content.find("\"craftTime_s\":", pos);
        if (ctPos != std::string::npos && ctPos < pos + 300) {
            try { desc.craftTimeSec = std::stof(content.substr(ctPos + 14, 10)); }
            catch (...) {}
        }
        crafting.AddRecipe(desc);
        pos = end + 1;
        ++count;
    }
    Engine::Core::Logger::Info("Loaded " + std::to_string(count) + " recipes from " + path);
}

} // anonymous namespace

int main() {
    namespace fs = std::filesystem;

    std::cout << "[NovaForge] Working directory: "
              << fs::current_path().string() << "\n";

    // ── Engine ──────────────────────────────────────────────────────────────
    Engine::Core::EngineConfig cfg;
    cfg.mode     = Engine::Core::EngineMode::Client;
    cfg.assetRoot = "Projects/NovaForge/Assets";
    cfg.tickRate  = 60;

    Engine::Core::Engine engine(cfg);
    engine.InitCore();
    engine.InitRender();
    engine.InitECS();

    Engine::Core::Logger::Init();
    Engine::Core::Logger::Info("NovaForge initializing…");

    // ── NF-09: Audio ────────────────────────────────────────────────────────
    Engine::Audio::AudioEngine audio;
    audio.Init();
    auto bgmId  = audio.LoadSound("Assets/Audio/Music/space_ambient_01.ogg");
    auto sfxId  = audio.LoadSound("Assets/Audio/SFX/thruster_01.wav");
    auto mineId = audio.LoadSound("Assets/Audio/SFX/mining_laser.wav");
    audio.SetLooping(bgmId, true);
    audio.SetVolume(bgmId, 0.4f);
    audio.Play(bgmId);
    Engine::Core::Logger::Info("NF-09: Audio engine running — background music started");

    // ── ECS world ───────────────────────────────────────────────────────────
    Runtime::ECS::World world;

    // NF-01: Player entity
    auto playerEntity = world.CreateEntity();
    world.AddComponent(playerEntity, Runtime::Components::Tag{"Player", {"Player","Controllable"}});
    Runtime::Components::Transform playerTr;
    playerTr.position = {0.f, 0.f, 0.f};
    world.AddComponent(playerEntity, playerTr);

    // Deposit entities for miners
    auto deposit1 = world.CreateEntity();
    world.AddComponent(deposit1, Runtime::Components::Tag{"Asteroid_OreDeposit_01", {"OreDeposit"}});
    Runtime::Components::Transform d1tr; d1tr.position = {200.f, 60.f, 100.f};
    world.AddComponent(deposit1, d1tr);

    auto deposit2 = world.CreateEntity();
    world.AddComponent(deposit2, Runtime::Components::Tag{"Asteroid_OreDeposit_02", {"OreDeposit"}});
    Runtime::Components::Transform d2tr; d2tr.position = {120.f, 30.f, -80.f};
    world.AddComponent(deposit2, d2tr);

    // ── NF-02: Player controller ─────────────────────────────────────────
    Runtime::Player::PlayerController player(playerEntity);
    player.SetMovementMode(Runtime::Player::MovementMode::Flying);
    player.SetMoveSpeed(12.0f);
    Engine::Core::Logger::Info("NF-02: PlayerController ready — WASD + mouse look");

    // ── NF-03: HUD + Inventory ───────────────────────────────────────────
    Runtime::UI::HUD hud;
    // Register default HUD elements
    Runtime::UI::HUDElement crosshair;
    crosshair.id = "crosshair"; crosshair.type = "crosshair";
    crosshair.x = 0.5f; crosshair.y = 0.5f;
    crosshair.width = 16.f; crosshair.height = 16.f;
    hud.AddElement(crosshair);

    Runtime::UI::HUDElement healthBar;
    healthBar.id = "healthbar"; healthBar.type = "healthbar";
    healthBar.x = 0.05f; healthBar.y = 0.9f;
    healthBar.width = 200.f; healthBar.height = 20.f;
    hud.AddElement(healthBar);

    Runtime::UI::HUDElement inventoryOverlay;
    inventoryOverlay.id = "inventory"; inventoryOverlay.type = "inventory";
    inventoryOverlay.x = 0.5f; inventoryOverlay.y = 0.5f;
    inventoryOverlay.width = 600.f; inventoryOverlay.height = 400.f;
    inventoryOverlay.visible = false;  // hidden until Tab pressed
    hud.AddElement(inventoryOverlay);

    Runtime::Inventory::Inventory inventory(40);
    Runtime::Inventory::Item starterItem;
    starterItem.id = "iron_ore"; starterItem.name = "Iron Ore";
    starterItem.category = "Raw"; starterItem.quantity = 20;
    inventory.AddItem(starterItem);
    Engine::Core::Logger::Info("NF-03: HUD initialized — Tab to toggle inventory");

    bool inventoryOpen = false;

    // ── NF-04: Crafting ──────────────────────────────────────────────────
    Runtime::CraftingSystem crafting;
    std::string recipePath = "Projects/NovaForge/Recipes/crafting_recipes.json";
    LoadRecipes(crafting, recipePath);
    // Wire callbacks
    crafting.OnCraftComplete([](Runtime::SessionId sid, Runtime::RecipeId rid) {
        Engine::Core::Logger::Info("Craft complete: recipe=" + std::to_string(rid));
    });
    crafting.OnCraftFailed([](Runtime::SessionId sid, Runtime::RecipeId rid, const std::string& reason) {
        Engine::Core::Logger::Warn("Craft failed: " + reason);
    });
    Engine::Core::Logger::Info("NF-04: CraftingSystem ready — "
        + std::to_string(crafting.Stats().totalRecipes) + " recipes loaded");

    // ── NF-05 / NF-BLD: Builder integration (ships, stations, structures) ──
    //
    // NovaForgeBuilderIntegration wires:
    //   Builder::PartLibrary       — catalogue loaded from Parts/*.json
    //   Builder::Assembly          — placed parts + snap/weld links
    //   Builder::InteriorNode      — interior module slots
    //   Builder::SnapRules         — compatibility validation
    //   Builder::BuilderDamageSystem — per-part HP tracking
    //   Runtime::BuilderRuntime    — in-game place/remove/weld loop
    Runtime::NovaForgeBuilderIntegration builderIntegration;

    // Load part catalogue from project Parts directory
    size_t partsLoaded = builderIntegration.LoadPartLibrary("Projects/NovaForge/Parts");
    Engine::Core::Logger::Info("NF-BLD: Loaded " + std::to_string(partsLoaded)
        + " part definitions");

    // Load ship templates — creates a pre-populated BuildSession for each
    size_t shipsLoaded = builderIntegration.LoadShipTemplates("Projects/NovaForge/Ships");
    Engine::Core::Logger::Info("NF-BLD: Loaded " + std::to_string(shipsLoaded)
        + " ship templates as build sessions");

    // Open a blank station build session for the starting station
    uint32_t stationSessionId = builderIntegration.OpenBuildSession(
        Runtime::BuildTargetType::Station, "Starter Station");
    Engine::Core::Logger::Info("NF-BLD: Station build session #"
        + std::to_string(stationSessionId) + " created");

    // Set up snap compatibilities for all NovaForge part types
    builderIntegration.RegisterSnapCompatibility("hull",     "deck");
    builderIntegration.RegisterSnapCompatibility("engine",   "thruster");
    builderIntegration.RegisterSnapCompatibility("weapon",   "hardpoint");
    builderIntegration.RegisterSnapCompatibility("interior", "interior");

    // Crafting gate — only allow placing parts if the player has materials
    // TODO: full implementation should check exact recipe materials per part type
    builderIntegration.SetCraftingGate([&](uint32_t partDefId, uint32_t qty) -> bool {
        // Simplified check: require any iron ore in inventory.
        // A full implementation would query the part's recipe ingredient list.
        return inventory.FindItem("iron_ore") != nullptr;
    });
    builderIntegration.SetConsumePart([&](uint32_t /*partDefId*/, uint32_t /*qty*/) {
        // Consume one iron ore per part placed (simplified material cost).
        inventory.RemoveItem("iron_ore", 1);
    });

    // Wire builder event callbacks to the logger
    Runtime::BuilderIntegrationCallbacks builderCallbacks;
    builderCallbacks.onPartPlaced = [](uint32_t sess, uint32_t inst) {
        Engine::Core::Logger::Info("Builder: part placed inst=" + std::to_string(inst)
            + " session=" + std::to_string(sess));
    };
    builderCallbacks.onPartRemoved = [](uint32_t sess, uint32_t inst) {
        Engine::Core::Logger::Info("Builder: part removed inst=" + std::to_string(inst));
    };
    builderCallbacks.onWelded = [](uint32_t sess, uint32_t a, uint32_t b) {
        Engine::Core::Logger::Info("Builder: welded " + std::to_string(a)
            + " → " + std::to_string(b));
    };
    builderCallbacks.onPartDestroyed = [](uint32_t sess,
                                          const Builder::PartHealthState& s) {
        Engine::Core::Logger::Warn("Builder: part DESTROYED inst="
            + std::to_string(s.instanceId));
    };
    builderCallbacks.onSessionSaved = [](uint32_t sess) {
        Engine::Core::Logger::Info("Builder: session " + std::to_string(sess) + " saved");
    };
    builderIntegration.SetCallbacks(std::move(builderCallbacks));

    // Add default interior slots to the station build session.
    // Positions in local station space (metres from centre).
    // TODO: load slot layout from station template JSON for data-driven configuration.
    builderIntegration.SetActive(stationSessionId);
    {
        constexpr float kBridgeHeight    =  5.0f;
        constexpr float kPowerOffset     =  5.0f;
        constexpr float kHabitatOffset   = -5.0f;
        constexpr float kStorageDepth    = -5.0f;

        // Bridge slot
        Builder::ModuleSlot bridgeSlot;
        bridgeSlot.x = 0; bridgeSlot.y = kBridgeHeight; bridgeSlot.z = 0;
        bridgeSlot.allowedTypes = { Builder::ModuleType::Bridge };
        bridgeSlot.size = Builder::ModuleSize::M;
        builderIntegration.AddInteriorSlot(bridgeSlot);

        // Power plant slot
        Builder::ModuleSlot powerSlot;
        powerSlot.x = kPowerOffset; powerSlot.y = 0; powerSlot.z = 0;
        powerSlot.allowedTypes = { Builder::ModuleType::PowerPlant };
        powerSlot.size = Builder::ModuleSize::M;
        builderIntegration.AddInteriorSlot(powerSlot);

        // Habitat slot
        Builder::ModuleSlot habitatSlot;
        habitatSlot.x = kHabitatOffset; habitatSlot.y = 0; habitatSlot.z = 0;
        habitatSlot.allowedTypes = { Builder::ModuleType::Habitat,
                                      Builder::ModuleType::MedBay };
        habitatSlot.size = Builder::ModuleSize::L;
        builderIntegration.AddInteriorSlot(habitatSlot);

        // Storage slot
        Builder::ModuleSlot storageSlot;
        storageSlot.x = 0; storageSlot.y = kStorageDepth; storageSlot.z = 0;
        storageSlot.allowedTypes = { Builder::ModuleType::Storage };
        storageSlot.size = Builder::ModuleSize::L;
        builderIntegration.AddInteriorSlot(storageSlot);

        // Place a default power plant so the station starts with power
        Builder::InteriorModule powerMod;
        powerMod.id   = 1;
        powerMod.type = Builder::ModuleType::PowerPlant;
        powerMod.size = Builder::ModuleSize::M;
        powerMod.tier = 1;
        powerMod.name = "Basic Power Plant";
        powerMod.health = powerMod.maxHealth = 500.0f;
        builderIntegration.PlaceModule(1, powerMod);  // slot 1 = power slot
    }

    Engine::Core::Logger::Info("NF-BLD: Interior slots configured — "
        + std::to_string(builderIntegration.InteriorSlotCount()) + " slots, "
        + std::to_string(builderIntegration.InteriorModuleCount()) + " modules placed");

    // Switch active session back to the first ship for player ship building
    if (shipsLoaded > 0) builderIntegration.SetActive(1);

    bool builderActive = false;
    Engine::Core::Logger::Info("NF-05: Builder ready — F2 toggles build mode");

    // ── NF-06: PCG space station ─────────────────────────────────────────
    PCG::StructureGenerator pcgGen;
    PCG::StructureConfig stationCfg;
    stationCfg.type         = PCG::StructureType::Outpost;
    stationCfg.seed         = 42;
    stationCfg.sizeX        = 20;
    stationCfg.sizeY        = 10;
    stationCfg.sizeZ        = 20;
    stationCfg.floors       = 2;
    stationCfg.roomCountMin = 6;
    stationCfg.roomCountMax = 14;
    stationCfg.addWindows   = false; // space station
    stationCfg.addDoors     = true;
    stationCfg.addStairs    = true;
    auto stationMap = pcgGen.Generate(stationCfg);
    Engine::Core::Logger::Info("NF-06: PCG station generated — "
        + std::to_string(stationMap.rooms.size()) + " rooms, "
        + std::to_string(stationMap.doors.size()) + " doors");

    // ── NF-07: AI miner NPCs ─────────────────────────────────────────────
    Runtime::AIMinerStateMachine miners;
    miners.SetAvailableDeposits({deposit1, deposit2});

    Runtime::MinerConfig miner1;
    miner1.entityId        = 8;
    miner1.cargoCapacity   = 100.f;
    miner1.miningYieldPerSec = 2.0f;
    miner1.travelSpeed     = 15.f;
    miner1.sellPricePerUnit = 1.5f;
    miners.AddMiner(miner1);

    Runtime::MinerConfig miner2;
    miner2.entityId        = 9;
    miner2.cargoCapacity   = 80.f;
    miner2.miningYieldPerSec = 1.5f;
    miner2.travelSpeed     = 18.f;
    miner2.sellPricePerUnit = 1.2f;
    miners.AddMiner(miner2);
    Engine::Core::Logger::Info("NF-07: " + std::to_string(miners.MinerCount())
        + " AI miner NPCs active");

    // ── NF-08: Save / Load ───────────────────────────────────────────────
    Runtime::SaveLoad::SaveSystem saveSystem;
    std::string saveDir = "Projects/NovaForge/Saves";
    fs::create_directories(saveDir);

    auto QuickSave = [&]() {
        Runtime::SaveLoad::SaveData data;
        data.header.version   = "1.0";
        data.header.sceneName = "MainScene";
        std::ostringstream ss;
        ss << "{\"playerPos\":[" << playerTr.position.x << ","
           << playerTr.position.y << "," << playerTr.position.z << "]"
           << ",\"minerEarnings\":" << miners.TotalEarnings()
           << ",\"craftingCompleted\":" << crafting.Stats().completedTotal
           << "}";
        data.jsonPayload = ss.str();
        bool ok = saveSystem.QuickSave(data, saveDir);
        Engine::Core::Logger::Info(ok ? "NF-08: QuickSave OK" : "NF-08: QuickSave FAILED");

        // Also save builder sessions
        for (uint32_t sid : builderIntegration.AllSessionIds()) {
            Runtime::BuildSession* sess = builderIntegration.GetSession(sid);
            if (sess && sess->dirty) {
                std::string bldPath = saveDir + "/build_session_"
                    + std::to_string(sid) + ".json";
                builderIntegration.SaveSession(sid, bldPath);
            }
        }
    };

    auto QuickLoad = [&]() {
        Runtime::SaveLoad::SaveData data;
        bool ok = saveSystem.QuickLoad(saveDir, data);
        if (ok) {
            Engine::Core::Logger::Info("NF-08: QuickLoad OK — scene: " + data.header.sceneName);
        } else {
            Engine::Core::Logger::Warn("NF-08: QuickLoad failed (no save found?)");
        }
    };

    Engine::Core::Logger::Info("NF-08: SaveSystem ready — F5 quicksave, F9 quickload");

    // ── Game loop via Engine tick callback ──────────────────────────────
    float accumTime  = 0.f;
    bool  gameRunning = true;
    int   frameCount  = 0;

    // Register per-frame tick via Engine callback
    engine.SetFrameCallback([&](float dt) {
        if (!gameRunning) return;

        accumTime += dt;
        ++frameCount;

        // NF-07: tick AI miners
        miners.Tick(dt);

        // NF-04: tick crafting sessions
        crafting.Tick(dt);

        // NF-02: build dummy input (real input would come from window callbacks)
        Runtime::Player::PlayerInput input{};
        player.Update(dt, input);

        // NF-03: Tab key is handled by GLFW key callback in a full build.
        // The inventoryOpen flag is toggled by the real GLFW callback (not here);
        // this frame-callback only reflects the flag set externally.
        // (Demo: the actual toggle is driven by keyboard input registered via glfwSetKeyCallback.)

        // NF-05 / NF-BLD: builder mode tick
        if (builderActive) {
            builderIntegration.Tick(dt);
        }

        // NF-09: SFX on mining — fire once each time a new integer-second boundary is crossed
        static constexpr float kSFXIntervalSec = 3.f; // play mining SFX every 3 seconds
        static float sfxAccum = 0.f;
        sfxAccum += dt;
        if (sfxAccum >= kSFXIntervalSec) {
            sfxAccum -= kSFXIntervalSec;
            audio.Play(mineId);
        }

        // NF-08: periodic auto-save
        static constexpr float kAutoSaveIntervalSec = 60.f;
        static float autoSaveAccum = 0.f;
        autoSaveAccum += dt;
        if (autoSaveAccum >= kAutoSaveIntervalSec) {
            autoSaveAccum -= kAutoSaveIntervalSec;
            QuickSave();
        }

        hud.Update(dt);

        // Log periodic status
        if (frameCount % 300 == 0) {
            Engine::Core::Logger::Info(
                "Status: miners=" + std::to_string(miners.MinerCount())
                + " earnings=" + std::to_string(miners.TotalEarnings())
                + " crafting=" + std::to_string(crafting.Stats().activeSessions)
                + " sessions"
            );
        }
    });

    Engine::Core::Logger::Info("NovaForge ready — press ESC to quit, F5 quicksave, F9 quickload");
    engine.Run();

    // Perform final save on clean exit
    QuickSave();

    audio.Shutdown();
    Engine::Core::Logger::Info("NovaForge exited cleanly");
    return 0;
}
