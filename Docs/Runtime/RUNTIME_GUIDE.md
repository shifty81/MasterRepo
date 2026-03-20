# RuntimeLib — Runtime Guide

## Overview

RuntimeLib is the game-runtime foundation of the engine. It provides all the
subsystems needed to run a living game world — from entity management to
player control, crafting, saving, and procedural world layout.

```
Runtime/
├── ECS/             Entity-Component-System core
├── Components/      Reusable component definitions
├── Systems/         Registered game systems
├── Player/          Player controller and input processing
├── Inventory/       Item inventory management
├── Equipment/       Equipment slots and stat calculation
├── Crafting/        Recipes and crafting logic
├── Damage/          Damage, health, and death handling
├── Spawn/           Entity spawning and pooling
├── SaveLoad/        Serialized game-state persistence
├── UI/              HUD and in-world UI elements
├── World/           Scene management and spatial layouts
├── Collision/       AABB and broad-phase collision
├── Gameplay/        Prefabs, game-rules, triggers
└── BuilderRuntime/  Runtime support for the editor builder mode
```

---

## Key Classes

| Class | File | Description |
|---|---|---|
| `Runtime::ECS::World` | ECS/ECS.h | Central ECS registry — creates/destroys entities, dispatches `Update` |
| `Runtime::ECS::System` | ECS/System.h | Base class for all game systems; override `Update(float dt)` |
| `Runtime::ECS::DeltaEditStore` | ECS/DeltaEditStore.h | Records incremental entity mutations for undo / network sync |
| `Runtime::Player::PlayerController` | Player/PlayerController.h | Handles player movement, camera, and input mapping |
| `Runtime::Inventory::Inventory` | Inventory/Inventory.h | Slot-based item container with weight/capacity limits |
| `Runtime::Equipment::Equipment` | Equipment/Equipment.h | Equipment slots (weapon, armour, …) with stat modifiers |
| `Runtime::Crafting::CraftingSystem` | Crafting/Crafting.h | Recipe registry and crafting transaction logic |
| `Runtime::Damage::DamageSystem` | Damage/Damage.h | Applies typed damage, invokes death callbacks |
| `Runtime::Spawn::SpawnSystem` | Spawn/Spawn.h | Entity pools, spawn-points, respawn timers |
| `Runtime::SaveLoad::SaveSystem` | SaveLoad/SaveLoad.h | JSON serialise / deserialise full world state |
| `Runtime::UI::HUD` | UI/HUD.h | Renders player HUD elements (health bar, hotbar, minimap) |
| `Runtime::World::SceneManager` | World/SceneManager.h | Loads / unloads scenes; owns the active `ECS::World` |
| `Runtime::World::VoxelGridLayout` | World/VoxelGridLayout.h | Voxel-based spatial grid for large worlds |
| `Runtime::World::CubeSphereLayout` | World/CubeSphereLayout.h | Cube-sphere planet layout for spherical worlds |
| `Runtime::Collision::Collision` | Collision/Collision.h | Broad-phase AABB collision detection and response |
| `Runtime::Gameplay::PrefabSystem` | Gameplay/Prefabs/PrefabSystem.h | Instantiates pre-built entity archetypes from data |
| `Runtime::BuilderRuntime::BuilderRuntime` | BuilderRuntime/BuilderRuntime.h | Editor "play-in-editor" bridge |

---

## Build Instructions

### Prerequisites
- CMake ≥ 3.20
- C++20 compiler (GCC ≥ 11 / Clang ≥ 13 / MSVC 2022)

### Configure and build RuntimeLib

```bash
# From the repository root:
cmake -B build -DCMAKE_BUILD_TYPE=Debug .
cmake --build build --target RuntimeLib
```

### Build everything

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release .
cmake --build build --parallel
```

---

## Usage Examples

### ECS — Creating entities and components

```cpp
#include "Runtime/ECS/ECS.h"

Runtime::ECS::World world;

// Create an entity
auto entity = world.CreateEntity();

// Attach a component (any-typed data)
struct Position { float x, y, z; };
world.AddComponent<Position>(entity, {1.0f, 0.0f, 5.0f});

// Query all entities with a component
world.ForEach<Position>([](Runtime::ECS::EntityID id, Position& pos) {
    pos.x += 1.0f;
});

// Update all registered systems
world.Update(0.016f);  // 16 ms frame

// Destroy an entity
world.DestroyEntity(entity);
```

### Player Controller

```cpp
#include "Runtime/Player/PlayerController.h"

Runtime::Player::PlayerController player;
player.SetMoveSpeed(5.0f);
player.SetEntity(myPlayerEntity);

// Call once per frame from your game loop
player.Update(deltaTime, inputState);
```

### SaveLoad — Saving and loading game state

```cpp
#include "Runtime/SaveLoad/SaveLoad.h"

Runtime::SaveLoad::SaveSystem saves;

// Save current world to a slot
saves.Save("slot_01", world);

// Load world from a slot
if (saves.Load("slot_01", world)) {
    // world is now restored
}

// List available saves
auto slots = saves.ListSaves();
for (auto& s : slots) {
    std::cout << s.name << " — " << s.timestamp << "\n";
}
```

---

## Architecture Diagram

```
┌──────────────────────────────────────────────────────────┐
│                      Game Loop                            │
│   SceneManager::Update(dt)                                │
└────────────────────┬─────────────────────────────────────┘
                     │ owns
           ┌─────────▼──────────┐
           │   ECS::World        │  ← entity/component registry
           └──┬──────────────┬──┘
              │ systems      │ components
     ┌────────▼────┐    ┌────▼──────────────────────────────────┐
     │  Systems    │    │  Components (per entity)               │
     │  ─────────  │    │  Position · Health · Inventory          │
     │  Player     │    │  Equipment · Damage · Spawn             │
     │  Crafting   │    │  UI · Collision · Prefab               │
     │  Damage     │    └────────────────────────────────────────┘
     │  Spawn      │
     └────────┬────┘
              │ reads / writes
   ┌──────────▼───────────────────────────────────────────────┐
   │  Subsystem Managers                                        │
   │  Inventory · Equipment · Crafting · Damage · Spawn        │
   │  SaveLoad · HUD · Collision · PrefabSystem                 │
   └──────────────────────────────────────────────────────────┘
              │ persistence
   ┌──────────▼─────────────┐
   │  SaveLoad (JSON files)  │
   └────────────────────────┘
```

---

## Extending RuntimeLib

1. **New component** — define a plain struct in `Components/`, register in
   `World::RegisterComponent<T>()`.
2. **New system** — inherit `Runtime::ECS::System`, override `Update(float dt)`,
   register with `World::RegisterSystem<MySystem>()`.
3. **New subsystem** — add a `.h`/`.cpp` pair in an appropriate subdirectory and
   add the `.cpp` to `Runtime/CMakeLists.txt`.

All subsystems must be added to `RuntimeLib` via `CMakeLists.txt` — no
dynamic loading is used (offline engine).
