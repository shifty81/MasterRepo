# MasterRepo — Comprehensive Codebase Audit
> **Date:** March 2026 · **Audited by:** Copilot Agent
> Honest comparison of *actual* code on disk vs roadmap claims.
> This is not a wish-list — it reflects what `g++` compiles today.

---

## Build Status

| Target | Library | Files | Status |
|--------|---------|-------|--------|
| CoreLib | Core | 38 .cpp | ✅ Builds clean |
| VersionLib | Versions | 1 .cpp | ✅ Builds clean |
| AtlasEngine | Engine | 46 .cpp | ✅ Builds clean |
| UILib | UI | 10 .cpp | ✅ Builds clean |
| EditorLib | Editor | 30 .cpp | ✅ Builds clean |
| RuntimeLib | Runtime | 37 .cpp | ✅ Builds clean |
| BuilderLib | Builder | 10 .cpp | ✅ Builds clean |
| AILib | AI | 31 .cpp | ✅ Builds clean |
| AgentsLib | Agents | 12 .cpp | ✅ Builds clean |
| PCGLib | PCG | 27 .cpp | ✅ Builds clean |
| IDELib | IDE | 18 .cpp | ✅ Builds clean |
| ToolsLib | Tools | 36 .cpp | ✅ Builds clean |
| ToolsGUI | Tools/GUI | 9 .cpp | ✅ Builds clean |
| TrainingDataLib | TrainingData | 1 .cpp | ✅ Builds clean |
| **AtlasEditor** | executable | all of above | ✅ Links clean |
| **NovaForge** | executable | engine+runtime | ✅ Links clean |

**One-liner to build everything:** `bash Scripts/Tools/build_all.sh`

---

## Fixes Applied This Session

| Issue | File(s) | Fix |
|-------|---------|-----|
| `Core/Cache/LRUCache` directory missing | `Core/Cache/LRUCache/LRUCache.h/.cpp` | Created thread-safe LRU cache template |
| `Engine/Sim/EntityScheduler` not in CMakeLists | `Engine/CMakeLists.txt` | Added source |
| `PCG/Noise/FractalNoise` not in CMakeLists | `PCG/CMakeLists.txt` | Added source |
| `Runtime/Damage/DamageSystem` not in CMakeLists | `Runtime/CMakeLists.txt` | Added source |
| `TrainingData/TrainingCollector` had no CMake home | `TrainingData/CMakeLists.txt`, root `CMakeLists.txt` | Created target, wired into root |
| `AtlasEditor` only linked 4 libraries | `Editor/CMakeLists.txt` | Now links all 13 subsystem libs |
| `NovaForge` only linked 3 libraries | `Projects/NovaForge/CMakeLists.txt` | Now links full engine+game stack |
| ToolsGUI panels used wrong API names | `GPUProfilerPanel.cpp`, `PerfProfilerPanel.cpp`, `MemoryProfilerPanel.cpp` | Fixed 11 member-name mismatches |

---

## What IS Fully Implemented (Code Exists and Compiles)

### Core (CoreLib)
- ✅ Event bus, message bus, reflection, serialization (JSON + binary)
- ✅ Task/job system, command system (undo/redo), resource manager
- ✅ Plugin system, archive system, version system
- ✅ Hot-reload watcher, crash reporter, database, local CI pipeline
- ✅ Package manager, network protocol generator
- ✅ Deterministic seed, transaction manager, event log, config system
- ✅ Lua binding, typed event dispatcher, signal/slot, ring buffer
- ✅ Pool allocator, thread pool, binary serializer, data compressor
- ✅ **LRU cache** (created this session)
- ✅ HotReloadManager (file-change watcher with tag callbacks)

### Engine (AtlasEngine)
- ✅ Window (GLFW), platform abstraction, math library
- ✅ OpenGL renderer + render pipeline, audio engine + pipeline
- ✅ Physics world, camera + controller, input manager
- ✅ Animation graph + nodes + blend tree, tick scheduler
- ✅ Network manager, scene graph, net context, shader manager
- ✅ LOD system, particle system, post-process pipeline
- ✅ Terrain, voxel world, shadow mapper (cascaded)
- ✅ Occlusion culler, decal system, light manager
- ✅ Network interpolation buffer, delta compression, tile graph
- ✅ **EntityScheduler** (per-entity tick scheduling — wired this session)

### Runtime (RuntimeLib)
- ✅ ECS (entity + component + system), scene manager
- ✅ Player controller, inventory, equipment, crafting, damage, spawn, save/load
- ✅ HUD, builder runtime, collision, prefab system
- ✅ State sync, NPC controller, dialogue, animation controller
- ✅ Game session, behavior tree, achievement system, lobby system
- ✅ Input mapper, AABB physics, component registry, asset streamer
- ✅ Crafting system, AI miner/wallet state machines, skill tree
- ✅ Audio mixer
- ✅ **DamageSystem** (component-based typed damage — wired this session)

### Builder (BuilderLib)
- ✅ Part/module definitions, assembly + build queue, snap rules
- ✅ Collision shapes, physics data, part damage, audit log
- ✅ Interior node (ship module slot management)

### AI (AILib)
- ✅ LLM backend, model manager, project context, memory store
- ✅ Embedding engine, agent scheduler, error/code/archive/asset learning
- ✅ Prompt library, sandbox, training pipeline, model catalog
- ✅ Test generator, context help, regression test, bug triage, tutorial
- ✅ Meta-learning, rendering optimizer, validator
- ✅ Orchestrator, rules engine, metrics dashboard
- ✅ Decision visualizer, conflict resolver, resource monitor, goal planner
- ✅ Knowledge ingestion, arbiter

### Agents (AgentsLib)
- ✅ Code, build, debug, fix, refactor, cleanup, editor, asset, PCG agents
- ✅ Version agent, self-build agent

### PCG (PCGLib)
- ✅ Rules, geometry/LOD, world heightfield, textures, audio, quests
- ✅ Validation, noise, PCG pipeline, biome generator, dungeon generator
- ✅ Story generator, vegetation, weather, city generator
- ✅ Space layout, cave generator, world chunk generator
- ✅ Quest graph generator, texture generator, structure generator
- ✅ Advanced PCG (AI rule generation, validation, live preview)
- ✅ **FractalNoise** (fBm, turbulence, ridged-multi — wired this session)

### IDE (IDELib)
- ✅ Code editor, AI chat, IDE console, debugger, LSP client
- ✅ NL assistant, interactive docs, voice input, project explorer
- ✅ Snippet manager, task panel, watch panel, output panel
- ✅ Breakpoint manager, LSP diagnostics, code search engine
- ✅ Refactor panel, console filter

### Tools (ToolsLib + ToolsGUI)
- ✅ Asset importer/processor, build automation, server manager
- ✅ Replay timeline + recorder, git panel + git history
- ✅ Analytics dashboard, simulation playback, doc generator
- ✅ Code audit, admin console, scaffolder, test pipeline, metrics reporter
- ✅ Performance profiler, benchmark runner, asset pipeline
- ✅ Memory profiler, script runner, dependency analyzer
- ✅ Symbol locator, GPU profiler, texture atlas packer
- ✅ Asset browser
- ✅ **ToolsGUI** — 8 GUI panels (GPU/Perf/Memory/Dependency/Symbol/Git/Atlas/AssetBrowser)

### UI (UILib)
- ✅ Widget framework (Button, Label, TextInput, Checkbox, Slider, ProgressBar)
- ✅ Theme, UIStyle (dark/light), layout solver, UI animation
- ✅ Localization, accessibility, GUI system, UI logic graph

### Editor (EditorLib + AtlasEditor)
- ✅ Commands, scene outliner, inspector, content browser, docking, gizmos
- ✅ Viewport panel, console panel, editor modes, node editor
- ✅ UI layout, overlay system, builder/material/PCG editors
- ✅ Profiler panel, error panel, GUI editor
- ✅ Workspace state, VR preview, plugin marketplace
- ✅ GL/GLFW editor renderer (562 lines — real OpenGL code)
- ✅ Anomaly overlay, undoable command bus
- ✅ Tool registry, entity inspector tool, material override tool
- ✅ `main.cpp` — real window + render loop (ESC to quit)

### TrainingData (TrainingDataLib)
- ✅ Training sample collector (code snippets, error fixes, prompts, assets, diffs)

---

## What Is NOT Yet Done (Open Roadmap Items)

These items are accurately marked `[ ]` in `ROADMAP.md`. The underlying building
blocks exist; what's missing is the **wiring between them**.

### Editor Integration (EI series)
| ID | Task | Status |
|----|------|--------|
| EI-01 | Logger output → console panel (live sink) | ✅ Wired (`Editor/main.cpp` — `Logger::SetSink`) |
| EI-02 | ECS world → scene outliner (live tree) | ✅ Wired (`EditorRenderer.cpp:1946` — live entity list) |
| EI-03 | Mouse picking → entity selection | ✅ Wired (`EditorRenderer.cpp:349,784` — 3-D projection pick) |
| EI-04 | Inspector shows real component properties | ✅ Wired (`EditorRenderer.cpp:2018` — Tag/Transform/Mesh) |
| EI-05 | Gizmos rendered in viewport (translate/rotate/scale handles) | ✅ Wired (`EditorRenderer.cpp:1888` — X/Y/Z axis gizmos) |
| EI-06 | Content browser reads actual filesystem | ✅ Wired (`EditorRenderer.cpp:2289` — scans assets root) |
| EI-07 | Scene save and load from File menu | ✅ Wired (`EditorRenderer.cpp:1700` — File › Save/Open) |
| EI-08 | Build button in editor (calls build_all.sh) | ✅ Wired (`EditorRenderer.cpp:806` — TriggerBuild) |
| EI-09 | Error panel click-to-navigate to file:line | ✅ Wired (`EditorRenderer.cpp:2278` — click-to-nav) |
| EI-10 | Project new/open dialog | ⬜ Not yet done |
| EI-11 | Undo/redo (Ctrl+Z / Ctrl+Y) via UndoableCommandBus | ✅ Wired (`EditorRenderer.cpp:485` — cmd stack) |
| EI-12 | Code editor panel embedded (Monaco or custom) | ⬜ Not yet done |
| EI-13 | Play-in-Editor (PIE) — spawn NovaForge in-process | ✅ Wired (`EditorRenderer.cpp:1788,2654` — P key toggle) |
| EI-14 | AI chat panel connected to Ollama API | ✅ Wired (`EditorRenderer.cpp:2399,175` — async Ollama) |

### NovaForge Game (NF series)
| ID | Task | Status |
|----|------|--------|
| NF-01 | Default scene with basic entities | ✅ Done (`Projects/NovaForge/Scenes/default.scene` + built-in universe) |
| NF-02 | Player controller wired (input → movement) | ✅ Done (`NovaForge/main.cpp` — PlayerController WASD+mouse) |
| NF-03 | Inventory UI visible in game | ✅ Done (Tab key toggle in game loop) |
| NF-04 | Crafting recipes loaded from JSON | ✅ Done (`NovaForge/Recipes/crafting_recipes.json` + LoadRecipes()) |
| NF-05 | Builder mode in game | ✅ Done (F2 toggles NovaForgeBuilderIntegration) |
| NF-06 | PCG space station generated on start | ✅ Done (StructureGenerator called at scene load) |
| NF-07 | AI miner NPCs active | ✅ Done (AIMinerStateMachine ticking in game loop) |
| NF-08 | Save/load game state | ✅ Done (F5 quicksave / F9 quickload) |
| NF-09 | Audio playing in game | ✅ Done (AudioEngine BGM + SFX at startup) |
| NF-10 | Packaged Windows build (installer) | ⬜ Not done |

### AI Integration (AI-LV series)
| ID | Task | Status |
|----|------|--------|
| AI-LV-01 | Ollama connection verified (ping test) | ⬜ Not wired |
| AI-LV-02 | Code generation from AI chat panel | ⬜ Not wired |
| AI-LV-03 | Build-error auto-fix agent | ⬜ Not wired |
| AI-LV-04 | PCG prompt-to-scene generation | ⬜ Not wired |

### Polish (PL series)
| ID | Task | Status |
|----|------|--------|
| PL-01 | Dockable panel drag-and-resize | ⬜ DockingSystem exists; not fully wired to renderer |
| PL-02 | Editor settings / preferences panel | ⬜ Not done |
| PL-03 | Profiler panel shows live GPU/CPU data | ⬜ Panel exists; no live backend hooked |
| PL-04 | Material editor node graph | ⬜ MaterialEditor exists; node graph not connected |
| PL-05 | Animated splash screen | ⬜ Not done |
| PL-06 | Ctrl+P quick open | ⬜ SymbolLocator exists; not hooked to editor input |
| PL-07 | VR preview (OpenXR) | ⬜ VRPreview.cpp exists; OpenXR not integrated |

---

## Gaps That Would Help Most (Priority Order)

> **Update — March 2026:** Most EI and NF items are now implemented.
> Remaining open items:

| ID | Gap | Notes |
|----|-----|-------|
| EI-10 | Project new/open dialog | No modal dialog exists yet |
| EI-12 | Embedded code editor panel | IDE lib exists; no panel in renderer |
| NF-10 | Packaged Windows build (installer) | Requires CPack/NSIS config |
| AI-LV-01 | Ollama connection verified (ping test) | HTTP client exists; no health-check endpoint called |
| AI-LV-02 | Code generation from AI chat panel | Chat calls Ollama but doesn't insert generated code |
| AI-LV-03 | Build-error auto-fix agent | No automated retry loop implemented |
| AI-LV-04 | PCG prompt-to-scene generation | No natural-language → scene pipeline |
| PL-01 | Dockable panel drag-and-resize | DockingSystem exists; not wired to renderer panels |
| PL-03 | Profiler panel shows live GPU/CPU data | ProfilerPanel exists; no live backend pointers |
| PL-04 | Material editor node graph | MaterialEditor exists; node graph not connected |
| PL-06 | Ctrl+P quick open | SymbolLocator exists; not hooked to editor input |

---

## Files That Exist But Are Not In Any CMakeLists (Intentionally Excluded)

These are demo/prototype files kept as reference code — not production targets:

| File | Purpose |
|------|---------|
| `Engine/*Demo.cpp` (8 files) | Standalone subsystem demonstration snippets |
| `Core/Metadata/MetadataDemo.cpp` | Metadata usage example |
| `Core/Reflection/ReflectionDemo.cpp` | Reflection usage example |
| `Core/Serialization/SerializationDemo.cpp` | Serializer usage example |
| `Core/TaskSystem/TaskSystemDemo.cpp` | TaskSystem usage example |
| `Editor/Core/EditorShellDemo.cpp` | Shell prototype |
| `Editor/Core/EditorShellSelectionDemo.cpp` | Selection prototype |
| `Editor/Core/EditorFullIntegration.cpp` | Full integration prototype |
| `Editor/Core/EditorPrototype.cpp` | Early prototype |
| `Experiments/Prototypes/terrain_prototype.cpp` | Terrain PCG experiment |
| `Experiments/TestSystems/ecs_stress_test.cpp` | ECS performance test |

---

## Build Prerequisites (Windows)

```
Git Bash or MSYS2 with:
  cmake >= 3.20
  Visual Studio 2022 (MSVC) — auto-detected by Scripts/Tools/detect_compiler.sh
  GLFW3 (or internet access for FetchContent fallback)
  OpenGL (included with Windows)

One-line build:
  bash Scripts/Tools/build_all.sh
```
