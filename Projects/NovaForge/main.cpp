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
#include "Engine/Window/Window.h"
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
// ── NovaForgeHUD requires stb_easy_font implementation in this TU ─────────
#define STB_EASY_FONT_IMPLEMENTATION
#include "Projects/NovaForge/NovaForgeHUD.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>

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

    // ── Engine ──────────────────────────────────────────────────────────────
    Engine::Core::EngineConfig cfg;
    cfg.mode        = Engine::Core::EngineMode::Client;
    cfg.assetRoot   = "Projects/NovaForge/Assets";
    cfg.windowTitle = "NovaForge";
    cfg.tickRate    = 60;

    Engine::Core::Engine engine(cfg);
    engine.InitCore();
    engine.InitRender();
    engine.InitECS();

    Engine::Core::Logger::Init();
    Engine::Core::Logger::Info("NovaForge initializing…");
    Engine::Core::Logger::Info("Working directory: " + fs::current_path().string());

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
    world.AddComponent(playerEntity, Runtime::Components::Tag{"Player", {"Player","Controllable","Ship"}});
    Runtime::Components::Transform playerTr;
    playerTr.position = {0.f, 0.f, 0.f};
    world.AddComponent(playerEntity, playerTr);

    // ── Initial scene objects visible from the spawn camera ──────────────
    // Prometheus Station directly ahead (looking down +Z at yaw=0)
    auto stationEntity = world.CreateEntity();
    world.AddComponent(stationEntity, Runtime::Components::Tag{"Prometheus Station", {"Station"}});
    Runtime::Components::Transform stTr; stTr.position = {0.f, 0.f, 300.f};
    world.AddComponent(stationEntity, stTr);

    // Nearby companion ship to give the scene life
    auto escortEntity = world.CreateEntity();
    world.AddComponent(escortEntity, Runtime::Components::Tag{"Escort_Vanguard", {"Ship","Fighter","NPC"}});
    Runtime::Components::Transform escTr; escTr.position = {30.f, 5.f, 120.f};
    world.AddComponent(escortEntity, escTr);

    // Nearby planet (Novus Prime) close enough to be clearly visible
    auto planetEntity = world.CreateEntity();
    world.AddComponent(planetEntity, Runtime::Components::Tag{"Novus Prime", {"Planet","Rocky","Habitable"}});
    Runtime::Components::Transform ptTr; ptTr.position = {-200.f, -50.f, 800.f};
    world.AddComponent(planetEntity, ptTr);

    // The home star (Sol-equivalent) far in the distance
    auto starEntity = world.CreateEntity();
    world.AddComponent(starEntity, Runtime::Components::Tag{"Sol", {"Star"}});
    Runtime::Components::Transform starTr; starTr.position = {0.f, 200.f, 5000.f};
    world.AddComponent(starEntity, starTr);

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
    {
        std::error_code ec;
        fs::create_directories(saveDir, ec);
        // Silently ignore errors (e.g., read-only working directory);
        // QuickSave also uses the error_code variant internally.
    }

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

    // ── FPS camera & key state — declared before any lambdas capture them ──
    float  playerYaw   = 0.f;   // horizontal look (degrees)
    float  playerPitch = 0.f;   // vertical look   (degrees, clamped ±89)
    bool   keyW = false, keyA = false, keyS = false, keyD = false;
    bool   keySpace = false, keyShift = false;
    double lastMouseX = 640.0, lastMouseY = 360.0;
    bool   mouseInitialized = false;

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

        // NF-02: FPS movement from real keyboard state
        Runtime::Player::PlayerInput input{};
        {
            float speed = 1.f;
            if (keyW) { input.moveForward =  speed; }
            if (keyS) { input.moveForward = -speed; }
            if (keyA) { input.moveRight   = -speed; }
            if (keyD) { input.moveRight   =  speed; }
            if (keySpace) input.moveUp =  1.f;
            if (keyShift) input.moveUp = -1.f;
            input.lookYaw   = playerYaw;
            input.lookPitch = playerPitch;
        }
        player.Update(dt, input);

        // Apply yaw-oriented movement directly to the player transform.
        // PlayerController::Update is a stub (no World ref), so we handle
        // position integration here with the camera's own basis vectors.
        {
            auto* tr = world.GetComponent<Runtime::Components::Transform>(playerEntity);
            if (tr && (keyW || keyS || keyA || keyD || keySpace || keyShift)) {
                const float yR  = playerYaw   * 3.14159265f / 180.f;
                const float pR  = playerPitch * 3.14159265f / 180.f;
                const float spd = player.GetMoveSpeed() * dt;
                // Camera forward (matches FPSLoadViewMatrix convention)
                const float fwdX = std::cos(pR) * std::sin(yR);
                const float fwdY = std::sin(pR);
                const float fwdZ = std::cos(pR) * std::cos(yR);
                // Camera right = cross(forward, world_up) normalised
                const float rightX = -std::cos(yR);   // -fz at pitch=0
                const float rightZ =  std::sin(yR);   //  fx at pitch=0
                if (keyW) { tr->position.x += fwdX  * spd; tr->position.y += fwdY * spd; tr->position.z += fwdZ  * spd; }
                if (keyS) { tr->position.x -= fwdX  * spd; tr->position.y -= fwdY * spd; tr->position.z -= fwdZ  * spd; }
                if (keyD) { tr->position.x += rightX * spd; tr->position.z += rightZ * spd; }
                if (keyA) { tr->position.x -= rightX * spd; tr->position.z -= rightZ * spd; }
                if (keySpace) tr->position.y += spd;
                if (keyShift) tr->position.y -= spd;
            }
        }

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

    // Wire FPS input before calling engine.Run() so the engine's ESC
    // chaining (done inside Run) will preserve our callbacks.
    if (auto* win = engine.GetWindow()) {
        win->onMouseMove = [&](double x, double y) {
            if (!mouseInitialized) { lastMouseX = x; lastMouseY = y; mouseInitialized = true; return; }
            double dx = x - lastMouseX;
            double dy = y - lastMouseY;
            lastMouseX = x; lastMouseY = y;
            playerYaw   -= (float)dx * 0.15f;
            playerPitch -= (float)dy * 0.15f;
            playerPitch  = std::max(-89.f, std::min(89.f, playerPitch));
        };
        win->onKey = [&](int key, bool pressed) {
            // GLFW key codes (platform-independent integers)
            constexpr int kW = 87, kA = 65, kS = 83, kD = 68;
            constexpr int kSpace = 32, kShift = 340;
            if (key == kW) keyW = pressed;
            if (key == kA) keyA = pressed;
            if (key == kS) keyS = pressed;
            if (key == kD) keyD = pressed;
            if (key == kSpace) keySpace = pressed;
            if (key == kShift) keyShift = pressed;
        };
    }

    // ── NovaForge HUD — graphical overlay drawn into the engine window ──────
    // The HUD is rendered via the RenderCallback so it happens inside the
    // engine's own BeginFrame/EndFrame pair each tick.
    NovaForge::HUD gameHud;

    // Feed every Logger line into the HUD's log panel
    Engine::Core::Logger::SetSink([&](const std::string& msg) {
        gameHud.AppendLog(msg);
    });

    // Track real frame time for accurate HUD FPS display
    auto lastFrameTime = std::chrono::steady_clock::now();

    // Capture references for the render callback
    engine.SetRenderCallback([&](int w, int h) {
        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - lastFrameTime).count();
        lastFrameTime = now;
        if (dt <= 0.0 || dt > 1.0) dt = 1.0 / 60.0;

        // ── Build world objects from ECS for 3-D FPS rendering ──────────
        auto state = gameHud.BuildState(
            static_cast<int>(engine.GetWorld().EntityCount()),
            miners.MinerCount(),
            miners.TotalEarnings(),
            crafting.Stats().activeSessions,
            builderActive,
            0.0
        );

        // ── Compute nearest celestial body for approach prompt ────────────
        {
            auto* playerTr2 = world.GetComponent<Runtime::Components::Transform>(playerEntity);
            float ppx = playerTr2 ? playerTr2->position.x : 0.f;
            float ppy = playerTr2 ? playerTr2->position.y : 0.f;
            float ppz = playerTr2 ? playerTr2->position.z : 0.f;
            float minDist = 1e9f;
            for (auto& obj : state.worldObjects) {
                float dx = obj.x - ppx, dy = obj.y - ppy, dz = obj.z - ppz;
                float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                if (dist < minDist && obj.size >= 1.5f) {
                    minDist = dist;
                    state.nearestBody.name     = obj.name;
                    state.nearestBody.distance = dist;
                    // WorldObject carries only name+size (no tags vector), so
                    // gas-giant detection falls back to name matching.
                    bool isGas = (obj.name.find("Jupiter")!=std::string::npos ||
                                  obj.name.find("Saturn") !=std::string::npos ||
                                  obj.name.find("Uranus") !=std::string::npos ||
                                  obj.name.find("Neptune")!=std::string::npos ||
                                  obj.name.find("Gas")    !=std::string::npos);
                    state.nearestBody.isGasGiant = isGas;
                    state.nearestBody.canLand    = !isGas && obj.size < 5.0f;
                }
            }
            state.showApproachPrompt = (minDist < 50.f && minDist > 2.f);
        }

        state.worldObjects.clear();
        {
            static const uint32_t kColors[] = {
                0xFFDD44FF, 0x4A90D9FF, 0x7ED321FF, 0xF5A623FF,
                0xBD10E0FF, 0xE05555FF, 0x55BBEEFF, 0xAAFF88FF
            };
            auto entities = world.GetEntities();
            for (size_t i = 0; i < entities.size(); i++) {
                auto id = entities[i];
                auto* tr  = world.GetComponent<Runtime::Components::Transform>(id);
                auto* tag = world.GetComponent<Runtime::Components::Tag>(id);
                NovaForge::WorldObject obj;
                if (tr) { obj.x = tr->position.x; obj.y = tr->position.y; obj.z = tr->position.z; }
                if (tag) obj.name = tag->name;
                obj.color = kColors[i % 8];
                obj.size  = 0.5f;
                if (tag) {
                    for (auto& t : tag->tags) {
                        if (t == "Star")     { obj.size = 5.0f; obj.color = 0xFFEE44FF; break; }
                        if (t == "GasGiant") { obj.size = 3.0f; break; }
                        if (t == "Planet")   { obj.size = 2.0f; break; }
                        if (t == "Station" || t == "JumpGate") { obj.size = 2.5f; break; }
                        if (t == "Ship" || t == "Fighter") { obj.size = 1.5f; break; }
                    }
                }
                state.worldObjects.push_back(obj);
            }
        }

        // ── Update HUD camera from player transform ──────────────────────
        {
            constexpr float kCameraEyeHeight = 1.7f;
            auto* tr = world.GetComponent<Runtime::Components::Transform>(playerEntity);
            float px = tr ? tr->position.x : 0.f;
            float py = (tr ? tr->position.y : 0.f) + kCameraEyeHeight;
            float pz = tr ? tr->position.z : 0.f;
            gameHud.SetCamera(px, py, pz, playerYaw, playerPitch);
        }

        gameHud.Draw(w, h, state, dt);
    });

    engine.Run();

    // Perform final save on clean exit
    QuickSave();

    audio.Shutdown();
    Engine::Core::Logger::Info("NovaForge exited cleanly");
    return 0;
}
