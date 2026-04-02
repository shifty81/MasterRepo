# NovaForge

> A voxel-first open-world survival and building game built in C++20 — engine, editor, and game in one repo.

NovaForge is a first-person, voxel-authoritative survival and construction game set in a procedurally generated galaxy.  You pilot a **R.I.G. (Rig Interface Gear)** exo-frame, mine asteroids and terrain, salvage wrecks, build structures, and eventually command fleets across faction-contested sectors.

This repository is the complete monorepo: the engine runtime, the game-specific editor, and the game itself.  The editor does not ship with the game.

---

## Current Status — Phase 3 Complete

| Phase | Goal | Status |
|-------|------|--------|
| **Phase 0** — Bootstrap | Clean compile baseline, project manifest, editor and game shells | ✅ Done |
| **Phase 1** — Dev World | Stable sandbox world, spawn path, save/load, world debug overlay | ✅ Done |
| **Phase 2** — Voxel Runtime | Chunk data model, edit API, serialization, voxel inspection in editor | ✅ Done |
| **Phase 3** — First Interaction Loop | R.I.G. state, mining tool, resources, inventory, HUD, standalone client | ✅ Done |
| **Phase 4** — Voxel Mesh Rendering | Mesher, GPU cache, Phong shader, lit terrain in editor + client | ✅ Done |
| **Phase 5** — Movement & FPS Camera | WASD + mouse look, jump, sprint, voxel AABB collision, FPS camera | ✅ Done |
| **Phase 6** — Multiplayer Foundation | Server authority, replication, sessions | 🔜 Next |

> See [`Docs/Game/ROADMAP.md`](Docs/Game/ROADMAP.md) and [`Docs/Game/TASKS.md`](Docs/Game/TASKS.md) for the full backlog.

---

## What's Working Right Now

- **Editor** — Win32 window, OpenGL rendering, 7-panel docking layout with live resize and per-monitor DPI scaling
- **Toolbar** — Play/Reset button to restart the in-editor interaction loop; Launch Game button to spawn the standalone client
- **Viewport** — 3-D grid placeholder, yaw/pitch/zoom camera controls
- **Scene Outliner** — lists ECS entities in the active level
- **Inspector** — shows components of the selected entity
- **Content Browser** — navigates the `Content/` directory tree
- **Voxel Inspector** — live chunk map stats, voxel overlay toggle
- **HUD Panel** — R.I.G. health/energy bars, tool slot, inventory counts (in editor)
- **Standalone Game Client** — Win32 window, OpenGL, HUD overlay, full Phase 3 interaction loop
- **Voxel Mesh Rendering** — culled-face mesher, GPU mesh cache, Phong-lit shader with per-type colour palette, 9-chunk starter terrain
- **FPS Movement** — WASD + mouse look, jump, sprint, gravity, voxel-aware AABB collision, FPS camera locked to player
- **Interaction Loop** — mine voxels, collect resources, place/repair voxels, inventory management
- **215 Catch2 unit tests** — Core, Engine, Physics, Animation, Input, Voxel, VoxelMesh, Interaction, Movement

---

## Architecture

```
Source/
├── Core/          # Math, memory, logging, events, reflection, serialization
├── Engine/        # ECS, world/level, behavior trees, asset system
├── Renderer/      # RHI (OpenGL via WGL/GLAD), forward pipeline, mesh, materials
├── Physics/       # Rigid bodies, collision detection, character controller
├── Audio/         # Device, spatial audio, mixer, cues
├── Animation/     # Skeleton, blend tree, state machine, IK
├── Input/         # Keyboard, mouse, gamepad, action mappings
├── Networking/    # Sockets, replication, sessions
├── UI/            # Custom 2-D renderer (no ImGui) — quad batching + stb_easy_font
├── Game/          # Game layer: world, voxels, interaction loop, gameplay systems
├── Editor/        # Editor application, docking panels, viewport, toolbar
└── Programs/
    ├── NovaForgeEditor/   # Editor executable (Win32 + OpenGL)
    └── NovaForgeGame/     # Standalone game client (Win32 + OpenGL)
```

### Module Dependency Graph

```
NF::Core ──────────────────────────────────────────────────────────────────┐
NF::Engine      → NF::Core                                                 │
NF::Renderer    → NF::Core, NF::Engine, OpenGL (GLAD), optional GLFW       │
NF::Physics     → NF::Core, NF::Engine                                     │
NF::Audio       → NF::Core                                                 │
NF::Animation   → NF::Core, NF::Engine                                     │
NF::Input       → NF::Core                                                 │
NF::Networking  → NF::Core                                                 │
NF::UI          → NF::Core, NF::Renderer                                   │
NF::Game        → NF::Core, NF::Engine, NF::UI, NF::Renderer               │
NF::Editor      → NF::Core, NF::Engine, NF::Renderer, NF::Game, NF::UI    │
                                                                           │
NovaForgeEditor → NF::Editor + all modules ────────────────────────────────┘
NovaForgeGame   → NF::Game + NF::Renderer + NF::UI + supporting modules
```

---

## Build Requirements

| Tool | Minimum Version |
|------|----------------|
| CMake | 3.20 |
| C++ Compiler | GCC 11 · Clang 14 · MSVC 2022 (C++20) |
| OpenGL | 3.3 core (system driver) |

GLAD and GLFW are fetched automatically via CMake FetchContent if not found locally.  On Linux, install `libgl-dev` and optionally `libglfw3-dev`.  On Windows, OpenGL ships with the GPU driver.

---

## Quick Start

```bash
# Clone
git clone https://github.com/shifty81/MasterRepo.git NovaForge
cd NovaForge

# Configure (Debug, all targets)
cmake -B Builds/Debug -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build Builds/Debug --parallel

# Run the Editor
./Builds/Debug/bin/NovaForgeEditor

# Run the Standalone Game Client
./Builds/Debug/bin/NovaForgeGame
```

### CMake Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `NF_BUILD_EDITOR` | `ON` | Build the NovaForge Editor (`NovaForgeEditor`) |
| `NF_BUILD_GAME`   | `ON` | Build the standalone game client (`NovaForgeGame`) |
| `NF_BUILD_TESTS`  | `OFF` | Build the Catch2 unit-test suite |

### CMake Presets

```bash
cmake --preset debug          # Debug build
cmake --preset release        # Optimised release build
cmake --build --preset debug
```

### Running Tests

```bash
cmake -B build -DNF_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## Editor ↔ Game Client Workflow

1. **Open the Editor** (`NovaForgeEditor`).
2. Use the toolbar at the top:
   - **`> Play`** — resets the in-editor interaction loop so you can re-test the Phase 3 loop live.
   - **`[] Stop`** — same as Play for now; a pause/step mode is planned.
   - **`Launch Game`** *(Windows only)* — spawns `NovaForgeGame.exe` as a separate process so you can test the standalone build without leaving the editor.
3. The **HUD Panel** (bottom-right) shows the R.I.G. health/energy bars, active tool, and inventory counts reflecting the live in-editor simulation.

---

## Design Principles

- **Voxel layer is authoritative.** Structure, mining, repair, damage, and destruction are all voxel operations.  The low-poly visual wrapper comes later.
- **R.I.G. first.** The player suit platform is the primary gameplay object.  Mechanics build outward from the R.I.G. state.
- **No ImGui.** The editor uses a fully custom UI renderer (OpenGL quad batching + stb_easy_font).
- **Phases over features.** Each phase has a tight, locked deliverable.  Nothing is merged that doesn't map to the current phase goal.
- **Editor does not ship.** The editor is a development tool.  `NovaForgeGame` is the shippable artifact.

---

## Docs

| Document | Description |
|----------|-------------|
| [`Docs/Game/ROADMAP.md`](Docs/Game/ROADMAP.md) | Phase-by-phase delivery plan |
| [`Docs/Game/TASKS.md`](Docs/Game/TASKS.md) | Checked task list per phase |
| [`Docs/Game/ARCHITECTURE.md`](Docs/Game/ARCHITECTURE.md) | Module layout and design locks |
| [`Docs/Game/PROJECT_RULES.md`](Docs/Game/PROJECT_RULES.md) | Hard boundaries and coding rules |

---

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE).

