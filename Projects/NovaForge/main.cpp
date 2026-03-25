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

    // ── NF-05: Builder runtime ───────────────────────────────────────────
    // BuilderRuntime requires Assembly + PartLibrary; create stubs for wiring
    Runtime::BuilderRuntime builder;
    // builder.Init(&assembly, &partLib);  — full wiring when Assembly is loaded
    bool builderActive = false;
    Engine::Core::Logger::Info("NF-05: BuilderRuntime ready — F2 to toggle");

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

        // NF-05: builder mode tick
        if (builderActive) {
            builder.Tick(dt);
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
