# MasterRepo — Project Roadmap

> **Status as of March 2026**
> A living task list showing every required step, its dependencies, and current completion state.
> Checkboxes: `[x]` = done · `[ ]` = pending · `[-]` = in progress / partially done

---

## Quick Status

| Area | Done | Pending | Total |
|------|------|---------|-------|
| Foundation & Build | 8/8 | 0 | 8 |
| Core Systems | 11/11 | 0 | 11 |
| Engine | 6/6 | 0 | 6 |
| Editor — Infrastructure | 12/12 | 0 | 12 |
| Editor — Integration | 0/14 | 14 | 14 |
| Runtime & Gameplay | 13/13 | 0 | 13 |
| AI & Agents | 12/12 | 0 | 12 |
| Builder System | 9/9 | 0 | 9 |
| PCG | 9/9 | 0 | 9 |
| IDE & Tools | 8/8 | 0 | 8 |
| Asset Pipeline | 8/8 | 0 | 8 |
| Deploy & CI | 11/11 | 0 | 11 |
| Game Project (NovaForge) | 0/10 | 10 | 10 |
| **TOTAL** | **107/120** | **24** | **131** |

---

## How to Read This Document

Each task entry uses this format:

```
- [ ] **TASK-ID · Task Name**
      Requires: TASK-ID, TASK-ID
      Output: path/to/output
      Notes: any relevant detail
```

Tasks marked `[x]` are complete and their outputs exist in the repo.
Tasks marked `[ ]` are the **next steps** — work top-to-bottom within each section.

---

## Section 1 — Foundation & Build System ✅

All foundation tasks are complete.

- [x] **F-01 · Directory scaffold**
      Requires: nothing
      Output: All top-level directories (`Engine/`, `Core/`, `Editor/`, etc.)

- [x] **F-02 · Root CMakeLists.txt**
      Requires: F-01
      Output: `CMakeLists.txt` — builds all modules in dependency order

- [x] **F-03 · CMakePresets.json**
      Requires: F-02
      Output: `CMakePresets.json` — Debug / Release / cross-platform presets

- [x] **F-04 · bootstrap.sh / bootstrap.ps1**
      Requires: F-01
      Output: `bootstrap.sh`, `bootstrap.ps1` — one-command dev environment setup

- [x] **F-05 · build_all.sh**
      Requires: F-02
      Output: `Scripts/Tools/build_all.sh` — full project build with logging

- [x] **F-06 · Windows compiler detection (vswhere)**
      Requires: F-05
      Output: `Scripts/Tools/detect_compiler.sh` — MSVC / MinGW / LLVM auto-detect
      Notes: Fixed March 2026 — `${PROGRAMFILES(X86)}` bad-substitution bug resolved

- [x] **F-07 · .gitignore**
      Requires: F-01
      Output: `.gitignore` — excludes Builds/, models, caches

- [x] **F-08 · Config defaults**
      Requires: F-01
      Output: `Config/Engine.json`, `Config/Editor.json`, `Config/AI.json`, etc.

---

## Section 2 — Core Systems ✅

- [x] **C-01 · Message Bus**
      Requires: F-01
      Output: `Core/Messaging/MessageBus.h`, `Message.h`

- [x] **C-02 · Event System**
      Requires: F-01
      Output: `Core/Events/Event.h`, `EventBus.h`

- [x] **C-03 · Reflection System**
      Requires: C-01
      Output: `Core/Reflection/Reflect.h`, `TypeInfo.h`

- [x] **C-04 · Metadata System**
      Requires: C-03
      Output: `Core/Metadata/Metadata.h`

- [x] **C-05 · Serialization (JSON + Binary)**
      Requires: C-03
      Output: `Core/Serialization/Serializer.h`, `BinarySerializer/BinarySerializer.h`

- [x] **C-06 · Task / Job System**
      Requires: C-01
      Output: `Core/TaskSystem/TaskSystem.h`, `Core/JobSystem/JobSystem.h`

- [x] **C-07 · Command System (undo/redo)**
      Requires: C-02
      Output: `Core/CommandSystem/CommandSystem.h`, `Command.h`

- [x] **C-08 · Resource Manager**
      Requires: C-05
      Output: `Core/ResourceManager/ResourceManager.h`

- [x] **C-09 · Plugin System**
      Requires: C-06
      Output: `Core/PluginSystem/PluginSystem.h`, `IPlugin.h`

- [x] **C-10 · Archive System**
      Requires: C-05
      Output: `Core/ArchiveSystem/ArchiveSystem.h`, `ArchiveRecord.h`

- [x] **C-11 · Version System**
      Requires: C-05, C-10
      Output: `Core/VersionSystem/VersionSystem.h`, `VersionSnapshot.h`

---

## Section 3 — Engine Foundation ✅

- [x] **E-01 · Window system (GLFW)**
      Requires: F-02
      Output: `Engine/Window/Window.h/.cpp` — creates GL context, event callbacks

- [x] **E-02 · Platform abstraction**
      Requires: E-01
      Output: `Engine/Platform/Platform.h/.cpp`

- [x] **E-03 · Math library**
      Requires: F-01
      Output: `Engine/Math/Math.h` — Vec2/Vec3/Vec4/Mat4/Quat

- [x] **E-04 · OpenGL renderer**
      Requires: E-01, E-03
      Output: `Engine/Render/Renderer.h/.cpp`, `RenderPipeline.h/.cpp`

- [x] **E-05 · Audio engine**
      Requires: E-01
      Output: `Engine/Audio/AudioEngine.h/.cpp`, `AudioPipeline.h/.cpp`

- [x] **E-06 · Physics world**
      Requires: E-03
      Output: `Engine/Physics/PhysicsWorld.h/.cpp`

---

## Section 4 — Editor Infrastructure ✅

The editor window opens and renders all panels. Infrastructure is complete.

- [x] **ED-01 · Custom UI framework (no ImGui)**
      Requires: E-04, C-02
      Output: `UI/Widgets/Widget.h`, `UI/Layouts/Layout.h`, `UI/Themes/Theme.h`

- [x] **ED-02 · Docking system**
      Requires: ED-01
      Output: `Editor/Docking/DockingSystem.h/.cpp` — add/remove/show/resize panels

- [x] **ED-03 · OpenGL 2.1 editor renderer**
      Requires: E-01, E-04, ED-01
      Output: `Editor/Render/EditorRenderer.h/.cpp` (562 lines)
      Notes: Draws title bar, menu bar, viewport, outliner, inspector, console, status bar

- [x] **ED-04 · Editor main entry point**
      Requires: ED-03, E-01
      Output: `Editor/main.cpp` — opens 1280×720 window, runs render loop, ESC to quit

- [x] **ED-05 · Gizmo system**
      Requires: ED-03, E-03
      Output: `Editor/Gizmos/GizmoSystem.h/.cpp` — select entity, drag on X/Y/Z axis

- [x] **ED-06 · Overlay system**
      Requires: ED-03
      Output: `Editor/Overlay/OverlaySystem.h/.cpp`, `AnomalyOverlay.h/.cpp`

- [x] **ED-07 · Editor modes**
      Requires: ED-02
      Output: `Editor/Modes/EditorModes.h/.cpp` — select, place, paint, sculpt

- [x] **ED-08 · Node editor framework**
      Requires: ED-01
      Output: `Editor/NodeEditors/NodeEditor.h/.cpp`

- [x] **ED-09 · Scene outliner panel**
      Requires: ED-02
      Output: `Editor/Panels/SceneOutliner/LayerTagSystem.h/.cpp`

- [x] **ED-10 · Inspector panel**
      Requires: ED-02, C-03
      Output: `Editor/Panels/Inspector/InspectorTools.h/.cpp`

- [x] **ED-11 · Content browser panel**
      Requires: ED-02
      Output: `Editor/Panels/ContentBrowser/ContentBrowser.h/.cpp`

- [x] **ED-12 · Console / error panels**
      Requires: ED-02
      Output: `Editor/Panels/Console/ConsolePanel.h/.cpp`, `ErrorPanel/ErrorPanel.h/.cpp`

---

## Section 5 — Editor Integration  ← NEXT STEPS

> **This section is the current priority.** The editor window opens and renders,
> but panels show dummy/stub data. These tasks wire real engine systems into the editor UI.
> Work through them in order — each builds on the previous.

- [ ] **EI-01 · Logger → Console sink**
      Requires: ED-12 (done), E-04 (done)
      Output: `Engine/Core/Logger.cpp` — add sink callback; `Editor/main.cpp` — register sink
      Steps:
        1. Add `Logger::SetSink(std::function<void(const std::string&)>)` to `Logger.h/.cpp`
        2. In `Editor/main.cpp`, after `renderer.Init()`, call:
           `Engine::Core::Logger::SetSink([&](const std::string& msg){ renderer.AppendConsole(msg); })`
      Why first: Every other integration step produces log output — console must capture it

- [ ] **EI-02 · ECS world → scene outliner**
      Requires: EI-01, ED-09 (done), C-03 (done)
      Output: `Editor/Render/EditorRenderer.cpp` — replace `m_sceneObjects` hardcoded list
      Steps:
        1. Pass `Runtime::ECS::World*` pointer into `EditorRenderer` (add member + setter)
        2. In `DrawOutliner()`, iterate `world->GetEntities()` instead of `m_sceneObjects`
        3. Show entity ID + name component (or fallback to "Entity #N")
      Why second: All subsequent editor work assumes live entities are visible

- [ ] **EI-03 · Mouse picking → entity selection**
      Requires: EI-02, ED-05 (done)
      Output: `Editor/Render/EditorRenderer.cpp` — `OnMouseButton()` handler
      Steps:
        1. On left-click inside viewport bounds, compute normalised device coords
        2. Call `ECS::World::QueryAtScreenPos()` (or iterate entities + AABB test)
        3. Store selected entity ID; call `GizmoSystem::SelectEntity()` with its transform
        4. Highlight selected entity in outliner

- [ ] **EI-04 · Inspector shows real component properties**
      Requires: EI-03, C-03 (done)
      Output: `Editor/Panels/Inspector/InspectorTools.cpp` — wire to `Core::Reflection`
      Steps:
        1. On entity selection, iterate components via `Core::ForEachProperty(typeName, ...)`
        2. Render each property as an editable field (float slider, int box, string input)
        3. Call component setter on value change; record with `UndoableCommandBus`

- [ ] **EI-05 · Gizmos rendered in viewport**
      Requires: EI-03, ED-05 (done)
      Output: `Editor/Render/EditorRenderer.cpp` — `DrawViewport()` calls gizmo draw
      Steps:
        1. After drawing the 3D grid, check `GizmoSystem::HasSelection()`
        2. Project entity world position to screen space
        3. Draw X/Y/Z axis arrows at entity position using coloured lines
        4. Highlight active drag axis on `GizmoSystem::GetActiveAxis()`

- [ ] **EI-06 · Content browser reads filesystem**
      Requires: EI-01, ED-11 (done)
      Output: `Editor/Panels/ContentBrowser/ContentBrowser.cpp`
      Steps:
        1. On init, call `std::filesystem::recursive_directory_iterator` on asset root
        2. Build a tree of folders and files; filter by extension (.obj, .png, .json, .glsl)
        3. Render as a scrollable icon grid in the content browser panel
        4. Double-click on a scene file calls `SceneManager::Load()`

- [ ] **EI-07 · Scene save and load from menu**
      Requires: EI-02, C-05 (done)
      Output: `Editor/main.cpp` — menu bar "File > Save Scene" / "File > Open Scene"
      Steps:
        1. Add menu bar click detection in `EditorRenderer::DrawMenuBar()`
        2. On "Save Scene", serialize `ECS::World` via `BinarySerializer` to `Projects/NovaForge/Scenes/`
        3. On "Open Scene", deserialize and repopulate `ECS::World`; refresh outliner

- [ ] **EI-08 · Build button in editor**
      Requires: EI-01
      Output: `Editor/main.cpp` — menu bar "Build > Build All"
      Steps:
        1. On "Build" click, launch `Scripts/Tools/build_all.sh --debug-only` via `std::system()`
        2. Pipe stdout/stderr into `Logger`; errors appear in console and error panel
        3. Parse error lines; populate `ErrorPanel` list with file:line links

- [ ] **EI-09 · Error panel click-to-navigate**
      Requires: EI-08
      Output: `Editor/Panels/ErrorPanel/ErrorPanel.cpp`
      Steps:
        1. Store errors as `{file, line, message}` structs
        2. On click, open the file in the IDE code editor panel (EI-12) at the correct line

- [ ] **EI-10 · Project new / open dialog**
      Requires: EI-07
      Output: New file: `Editor/Panels/ProjectDialog/ProjectDialog.h/.cpp`
      Steps:
        1. On startup (or "File > New Project"), show a modal overlay
        2. "New" — create `Projects/<name>/` scaffold, default scene, open it
        3. "Open" — native file picker (or typed path) to select a project directory

- [ ] **EI-11 · Undo / redo (Ctrl+Z / Ctrl+Y)**
      Requires: EI-04, C-07 (done)
      Output: `Editor/main.cpp` — key handler; `Editor/UndoableCommandBus.cpp`
      Steps:
        1. In `OnKey()`, detect Ctrl+Z → `UndoableCommandBus::Undo()`
        2. Detect Ctrl+Y / Ctrl+Shift+Z → `UndoableCommandBus::Redo()`
        3. Status bar shows last undone action name

- [ ] **EI-12 · Code editor panel embedded**
      Requires: ED-02 (done), ED-08 (done)
      Output: `IDE/CodeEditor/CodeEditor.cpp` — wired into editor docking
      Steps:
        1. Add a docked "Code" panel to the editor layout
        2. `CodeEditor::OpenFile(path, line)` renders file content with syntax highlight
        3. Connect to `EI-09` so error clicks open the right file

- [ ] **EI-13 · Play-in-editor (PIE)**
      Requires: EI-07, EI-02
      Output: `Editor/main.cpp` — toolbar Play/Stop buttons
      Steps:
        1. On "▶ Play", snapshot world state, then call `Engine::RunClient()` in a sub-context
        2. Render game output in the viewport (or a second window)
        3. On "■ Stop", restore the snapshotted world state

- [ ] **EI-14 · AI chat panel connected to Ollama**
      Requires: EI-01
      Output: `IDE/AIChat/AIChat.cpp` — HTTP client to local Ollama API
      Steps:
        1. Add libcurl or a minimal HTTP socket client
        2. POST prompt to `http://localhost:11434/api/generate`
        3. Stream response tokens into the chat panel
        4. "Apply" button sends AI-generated code to the code editor panel

---

## Section 6 — Game Project (NovaForge)

> Requires: EI-07 (scene save/load), EI-10 (project dialog), and EI-13 (PIE) to be useful.

- [ ] **NF-01 · Default scene with basic entities**
      Requires: EI-02, EI-07
      Output: `Projects/NovaForge/Scenes/default.scene`
      Steps: Create a scene file with a player entity, a few asteroid entities, and a light

- [ ] **NF-02 · Player controller wired up**
      Requires: NF-01, EI-13
      Output: `Runtime/Player/PlayerController.cpp` — WASD + mouse look working in PIE

- [ ] **NF-03 · Inventory UI visible in game**
      Requires: NF-02
      Output: `Runtime/UI/HUD.cpp` — Tab key opens inventory overlay

- [ ] **NF-04 · Crafting recipes loaded**
      Requires: NF-03, C-08
      Output: `Projects/NovaForge/Recipes/` — JSON recipe files loaded by `CraftingSystem`

- [ ] **NF-05 · Builder mode in game**
      Requires: NF-02
      Output: `Runtime/BuilderRuntime/BuilderRuntime.cpp` — place/snap/weld modules

- [ ] **NF-06 · PCG space station generated on start**
      Requires: NF-01
      Output: `PCG/Structures/StructureGenerator/` — call `Generate()` at scene load

- [ ] **NF-07 · AI miner NPCs active**
      Requires: NF-06
      Output: `Runtime/Sim/AIMinerStateMachine/` — ticking in game world

- [ ] **NF-08 · Save / load game state**
      Requires: NF-02, C-05
      Output: `Runtime/SaveLoad/SaveLoad.cpp` — F5 quicksave, F9 quickload

- [ ] **NF-09 · Audio playing in game**
      Requires: NF-02
      Output: `Engine/Audio/AudioEngine.cpp` — background music + SFX on events

- [ ] **NF-10 · Packaged Windows build**
      Requires: NF-08
      Output: `Builds/release/bin/NovaForge.exe` + asset bundle

---

## Section 7 — AI Agent Live Integration

> Requires: EI-14 (AI chat), and a local Ollama instance running.

- [ ] **AI-LV-01 · Ollama connection verified**
      Requires: EI-14
      Output: `WorkspaceState/AgentState.json` — records connected model name
      Steps: Run `ollama pull codellama` or `ollama pull llama3`; confirm chat panel responds

- [ ] **AI-LV-02 · Code generation from chat**
      Requires: AI-LV-01, EI-12
      Output: AI response → `Editor/Panels/Code/` — "Apply" inserts code at cursor

- [ ] **AI-LV-03 · Build-error auto-fix**
      Requires: AI-LV-01, EI-08
      Output: `Agents/FixAgent/FixAgent.cpp` — on build failure, send errors to LLM, apply patch

- [ ] **AI-LV-04 · PCG prompt-to-scene**
      Requires: AI-LV-01, NF-06
      Output: `Agents/PCGAgent/PCGAgent.cpp` — natural language → PCG rules → scene objects

---

## Section 8 — Polish & Advanced

- [ ] **PL-01 · Dockable panel drag-resize**
      Requires: ED-02 (done)
      Output: `Editor/Docking/DockingSystem.cpp` — mouse-drag splitter bars

- [ ] **PL-02 · Editor settings / preferences panel**
      Requires: EI-02
      Output: `Editor/Panels/Settings/SettingsPanel.h/.cpp`
      Settings: theme colour, key bindings, AI model, GLFW resolution

- [ ] **PL-03 · Profiler panel live data**
      Requires: EI-01
      Output: `Editor/Panels/Profiler/ProfilerPanel.cpp` — wire `PerformanceProfiler` callbacks

- [ ] **PL-04 · Material editor node graph**
      Requires: ED-08 (done)
      Output: `Editor/MaterialEditor/MaterialEditor.cpp` — node graph using `NodeEditor` framework

- [ ] **PL-05 · Animated splash screen**
      Requires: ED-03 (done)
      Output: `Editor/main.cpp` — show splash overlay during `Engine.Init()` calls

- [ ] **PL-06 · Ctrl+P quick open**
      Requires: EI-06, EI-12
      Output: Overlay with fuzzy search of assets + entities using `CodeSearchEngine`

- [ ] **PL-07 · VR preview (OpenXR)**
      Requires: EI-13
      Output: `Editor/Viewport/VRPreview.cpp` — real OpenXR session + frame submit

---

## Dependency Graph (Forward-Looking Tasks)

```
EI-01 (Logger sink)
  └─► EI-02 (ECS → outliner)
        └─► EI-03 (mouse picking)
              └─► EI-04 (inspector props)
              └─► EI-05 (gizmos in viewport)
  └─► EI-06 (content browser FS)
  └─► EI-08 (build button)
        └─► EI-09 (error panel links)
              └─► EI-12 (code editor panel)
EI-07 (scene save/load)
  └─► EI-10 (project dialog)
  └─► EI-13 (play-in-editor)
        └─► NF-01..NF-10 (game project)
EI-04 + C-07 ─► EI-11 (undo/redo)
EI-14 (AI chat Ollama)
  └─► AI-LV-01..AI-LV-04 (live AI)
ED-02 ─► PL-01 (drag-resize panels)
```

---

## Recommended Session Order

Work through these in order for the fastest path to a usable editor:

| Session | Tasks | Outcome |
|---------|-------|---------|
| **Session 1** | EI-01 | Real log output in console |
| **Session 2** | EI-02 | Real ECS entities in outliner |
| **Session 3** | EI-03, EI-04, EI-05 | Click to select + inspect + move objects |
| **Session 4** | EI-07, EI-10 | Open/save scenes from the editor |
| **Session 5** | EI-06 | Content browser shows real assets |
| **Session 6** | EI-08, EI-09, EI-12 | Build from editor, click errors to navigate |
| **Session 7** | EI-11 | Undo/redo working |
| **Session 8** | EI-13 | Play game from inside editor |
| **Session 9** | EI-14, AI-LV-01 | AI chat live with Ollama |
| **Session 10** | NF-01..NF-05 | First playable NovaForge scene |

---

## Completed Phases Reference

> Full history of all 32 completed build phases is in [Appendix C](#appendix-c-phases-18-32).
> All 259 headers and 278 source files build clean on Linux.

| Phases | Subsystems built |
|--------|-----------------|
| 0–10 | Foundation, Core, Engine, Editor infra, Runtime, AI, Builder, PCG, IDE, Assets, Deploy |
| 11–17 | GUI systems, Blender addon, AI testing, NL assistant, Core expansion, Self-build agent |
| 18–32 | 90+ additional subsystems (audio mixer, anim blend, net foundations, crafting, lighting, etc.) |

---

## Appendix A — Build Commands

```bash
# Full build (debug + release)
./Scripts/Tools/build_all.sh

# Debug only (faster, ~2×)
./Scripts/Tools/build_all.sh --debug-only

# Clean rebuild
./Scripts/Tools/build_all.sh --clean --debug-only

# Run the editor
./Builds/debug/bin/AtlasEditor

# Run the game
./Builds/debug/bin/NovaForge
```

> **Windows (Git Bash):** `Scripts/Tools/detect_compiler.sh` auto-detects MSVC via
> `vswhere.exe`. Run from Git Bash — no manual vcvarsall needed.

---

<<<<<<< HEAD
## Phase 1 – Core Systems

**Goal:** Build the shared foundation that all other systems depend on.

**Milestone:** Event-driven architecture with serialization, reflection, and a task system running.

| # | Task | Description | Output |
|---|------|-------------|--------|
| 1.1 | Message Bus | Publish/subscribe message system | `Core/Messaging/` |
| 1.2 | Event System | Typed event dispatching | `Core/Events/` |
| 1.3 | Reflection System | Runtime type info, property iteration | `Core/Reflection/` |
| 1.4 | Metadata System | Tag any object with key-value metadata | `Core/Metadata/` |
| 1.5 | Serialization | JSON/binary save/load for all reflected types | `Core/Serialization/` |
| 1.6 | Task / Job System | Multi-threaded task execution | `Core/TaskSystem/` |
| 1.7 | Command System | Undo/redo command pattern | `Core/CommandSystem/` |
| 1.8 | Resource Manager | Asset loading, caching, reference counting | `Core/ResourceManager/` |
| 1.9 | Plugin System | Dynamic plugin loading | `Core/PluginSystem/` |
| 1.10 | Archive System | Archive management with metadata | `Core/ArchiveSystem/` |
| 1.11 | Version System | Snapshot/frame creation and diffing | `Core/VersionSystem/` |

**Dependencies:** Phase 0

**Usable result:** Robust core library usable by all subsystems; events, serialization, and task scheduling working.

---

## Phase 2 – Engine Foundation

**Goal:** Get a rendering window with input handling, basic shapes, and a camera.

**Milestone:** 3D viewport with camera controls, basic shape rendering, and keyboard/mouse input.

| # | Task | Description | Output |
|---|------|-------------|--------|
| 2.1 | Window / Platform | SDL2 or GLFW window, OS abstraction | `Engine/Window/`, `Engine/Platform/` |
| 2.2 | Input System | Keyboard, mouse, gamepad with action mapping | `Engine/Input/` |
| 2.3 | Math Library | Vectors, matrices, quaternions, transforms | `Engine/Math/` |
| 2.4 | Renderer (basic) | OpenGL 4.x forward renderer, camera, lights | `Engine/Render/` |
| 2.5 | Audio (stub) | Audio system interface, placeholder impl | `Engine/Audio/` |
| 2.6 | Physics (stub) | Physics world interface, Bullet integration stub | `Engine/Physics/` |

**Dependencies:** Phase 0, Phase 1 (events, messaging)

**Usable result:** An interactive 3D viewport with camera movement and shape rendering.

---

## Phase 3 – Editor Framework

**Goal:** Build the editor shell — custom UI with dockable panels and a 3D viewport.

**Milestone:** Dockable editor with scene viewport, hierarchy panel, inspector panel, and gizmos.

| # | Task | Description | Output |
|---|------|-------------|--------|
| 3.1 | Custom UI System | Widgets, layouts, themes (Atlas rule: no ImGui) | `Editor/UI/`, `UI/` |
| 3.2 | Docking System | Dock/undock/split/tab panels | `Editor/Docking/` |
| 3.3 | Panel System | Base panel class, panel registry | `Editor/Panels/` |
| 3.4 | Viewport System | 3D viewport with camera modes (FPS, freecam, orbit) | `Editor/Viewport/` |
| 3.5 | Gizmo System | Move, rotate, scale gizmos with snapping | `Editor/Gizmos/` |
| 3.6 | Overlay System | Debug overlays, tool overlays, notifications | `Editor/Overlay/` |
| 3.7 | Editor Modes | Select, place, paint, sculpt mode switching | `Editor/Modes/` |
| 3.8 | Scene Outliner | Hierarchical scene tree panel | `Editor/Panels/SceneOutliner` |
| 3.9 | Inspector Panel | Property editor using reflection system | `Editor/Panels/Inspector` |
| 3.10 | Content Browser | File/asset browser with preview | `Editor/Panels/ContentBrowser` |
| 3.11 | Console Panel | Log output, command input | `Editor/Panels/Console` |
| 3.12 | Node Editor Base | Node graph framework for blueprints/materials | `Editor/NodeEditors/` |

**Dependencies:** Phase 1 (reflection, events, commands), Phase 2 (renderer, input)

**Usable result:** A fully dockable editor with a 3D viewport where you can select and manipulate objects.

---

## Phase 4 – Runtime & Gameplay

**Goal:** Build the gameplay layer so the engine can run a game.

**Milestone:** Entities in a world with components, a player that can move, and basic inventory.

| # | Task | Description | Output |
|---|------|-------------|--------|
| 4.1 | ECS Core | Entity-Component-System framework | `Runtime/ECS/` |
| 4.2 | World / Scene | Scene graph, world management, scene loading | `Runtime/World/` |
| 4.3 | Component Library | Transform, mesh, light, camera, collider, etc. | `Runtime/Components/` |
| 4.4 | System Library | Render system, physics system, input system | `Runtime/Systems/` |
| 4.5 | Player System | Player controller, rig, TAB UI (equipment view) | `Runtime/Player/` |
| 4.6 | Inventory System | Item storage, stacking, categories | `Runtime/Inventory/` |
| 4.7 | Equipment System | Equip/unequip, slots, stats | `Runtime/Equipment/` |
| 4.8 | Crafting System | Recipes, multi-stage crafting, assembler | `Runtime/Crafting/` |
| 4.9 | Damage System | Health, damage types, armor, death | `Runtime/Damage/` |
| 4.10 | Spawn System | Entity spawning, pooling | `Runtime/Spawn/` |
| 4.11 | Save/Load | Serialization of game state | `Runtime/SaveLoad/` |
| 4.12 | Runtime UI (HUD) | HUD, menus, game UI | `Runtime/UI/` |
| 4.13 | Prefab System | Prefab templates, instantiation | `Runtime/Gameplay/Prefabs` |

**Dependencies:** Phase 1 (serialization, events, ECS), Phase 2 (renderer, physics, input)

**Usable result:** A playable prototype — player in a world with objects, inventory, and basic combat.

---

## Phase 5 – AI Agent System

**Goal:** Get a local AI agent running that can read/write project files and generate code.

**Milestone:** Chat with a local LLM, ask it to generate or modify code, and see results in the editor.

| # | Task | Description | Output | Status |
|---|------|-------------|--------|--------|
| 5.1 | LLM Integration | Connect to Ollama / llama.cpp / local models | `AI/ModelManager/` | Stubbed |
| 5.2 | Agent Core | Prompt → Plan → Tool → Result loop | `Agents/CodeAgent/` | Incomplete |
| 5.3 | Tool System | Tool registry, tool runner, permissions | `AI/` + `Agents/` | Stubbed |
| 5.4 | File System Tools | `read_file`, `write_file`, `list_dir`, `search_code` | Agent tools | Stubbed |
| 5.5 | Build Tools | `compile_cpp`, `run_cmake`, `run_tests` | Agent tools | Stubbed |
| 5.6 | Memory System | Long-term context storage, embeddings | `AI/Memory/`, `AI/Embeddings/` | Stubbed |
| 5.7 | Agent Scheduler | Multi-agent orchestration, task queue | `AI/AgentScheduler/` | Stubbed |
| 5.8 | Workspace State | Track open files, build state, errors | `WorkspaceState/` | Stubbed |
| 5.9 | Error Learning | Learn from build failures, auto-fix patterns | `AI/ErrorLearning/` | Stubbed |
| 5.10 | Code Learning | Index codebase for context-aware generation | `AI/CodeLearning/` | Stubbed |
| 5.11 | Prompt Library | Reusable prompt templates | `AI/Prompts/` | Stubbed |
| 5.12 | Sandbox System | AI outputs to temp workspace, user approves/merges | Safety system | Stubbed |

**Current Focus:**
Phase 5 is the active development focus. The following features are stubbed or incomplete and are being prioritized:
- Agent Core (prompt → plan → tool → result loop)
- Tool System (registry, runner, permissions)
- File/Build Tools (read/write/list/search, compile/run/tests)
- Memory System (context, embeddings)
- Agent Scheduler (multi-agent orchestration)
- Workspace State (open files, build state, errors)
- Error/Code Learning (auto-fix, code indexing)
- Prompt Library (reusable templates)
- Sandbox System (safe output, approval workflow)

**Dependencies:** Phase 1 (events, tasks), Phase 0 (file structure)

**Usable result:** An AI assistant that can generate code files, fix compile errors, and modify the project — all offline.

---

## Phase 6 – Builder System

**Goal:** Implement Starbase-style module building with snap points, welding, and assemblies.

**Milestone:** Place, snap, weld, and detach modules in the 3D editor. Parts have hitboxes and damage.

| # | Task | Description | Output |
|---|------|-------------|--------|
| 6.1 | Part Format | Part definition (mesh, collision, snap points, stats) | `Builder/Parts/` |
| 6.2 | Module Format | Module = group of parts, connection rules | `Builder/Modules/` |
| 6.3 | Snap System | Snap point matching, alignment, validation | `Builder/SnapRules/` |
| 6.4 | Assembly System | Assemble/disassemble module groups | `Builder/Assembly/` |
| 6.5 | Builder Physics | Physics data for built structures | `Builder/PhysicsData/` |
| 6.6 | Builder Collision | Collision shapes from assembled parts | `Builder/Collision/` |
| 6.7 | Builder Damage | Per-part damage, destruction, repair | `Builder/Damage/` |
| 6.8 | Builder Editor | Editor integration for building | `Editor/BuilderEditor/` |
| 6.9 | Builder Runtime | Runtime building (in-game) | `Runtime/BuilderRuntime/` |

**Dependencies:** Phase 2 (renderer, physics), Phase 3 (editor, gizmos), Phase 4 (ECS, components)

**Usable result:** A Starbase-like building system where you snap modules together and test damage/physics.

---

## Phase 7 – Procedural Content Generation

**Goal:** Procedurally generate geometry, textures, audio, worlds, and quests.

**Milestone:** Generate a procedural space station or terrain from rules and preview it in-editor.

| # | Task | Description | Output |
|---|------|-------------|--------|
| 7.1 | PCG Rule Engine | Define procedural rules (constraints, weights, seeds) | `PCG/Rules/` |
| 7.2 | Geometry Generator | Procedural meshes, terrain, structures | `PCG/Geometry/` |
| 7.3 | Texture Generator | Procedural textures (noise, patterns) | `PCG/Textures/` |
| 7.4 | Audio Generator | Procedural sound effects, ambient | `PCG/Audio/` |
| 7.5 | World Generator | Procedural worlds, dungeons, stations | `PCG/World/` |
| 7.6 | Quest Generator | Procedural missions, objectives, branching | `PCG/Quests/` |
| 7.7 | PCG Validation | Validate outputs (structural integrity, no overlaps) | `PCG/Validation/` |
| 7.8 | PCG Editor | Node-based rule editor with live preview | `Editor/PCGEditor/` |
| 7.9 | AI-Assisted PCG | AI generates/tunes PCG rules from prompts | `Agents/PCGAgent/` |

**Dependencies:** Phase 1 (serialization), Phase 2 (renderer), Phase 3 (editor, node editor), Phase 5 (AI agents)

**Usable result:** A procedural pipeline that generates content from rules — with AI assistance.

---

## Phase 8 – IDE & Integrated Tooling

**Goal:** Embed a code editor and AI chat panel directly inside the editor.

**Milestone:** Write code, see AI suggestions, chat with AI — all inside the editor.

| # | Task | Description | Output |
|---|------|-------------|--------|
| 8.1 | Code Editor | Embed Monaco or custom editor with syntax highlighting | `IDE/CodeEditor/` |
| 8.2 | Language Support | C++, Lua, Python, shader, JSON, GLSL | LSP integration |
| 8.3 | AI Autocomplete | AI-powered code completion from local LLM | `IDE/CodeEditor/` |
| 8.4 | AI Chat Panel | Chat window connected to agent system | `IDE/AIChat/` |
| 8.5 | Console / Terminal | Integrated terminal in editor | `IDE/Console/` |
| 8.6 | Debugger Integration | Step-through debugging, breakpoints | `IDE/Debugger/` |
| 8.7 | Git UI Panel | Commit, branch, diff, stash from editor | `Tools/` |
| 8.8 | Error Panel | Show build errors, click-to-navigate | `Editor/Panels/` |

**Dependencies:** Phase 3 (editor framework, panels), Phase 5 (AI agent system)

**Usable result:** A fully integrated IDE inside the editor — code, debug, chat with AI, all in one place.

---

## Phase 9 – Asset Pipeline & Blender Integration

**Goal:** Import, export, and procedurally generate assets via Blender and AI.

**Milestone:** Import a model from Blender, generate procedural textures, preview in editor.

| # | Task | Description | Output |
|---|------|-------------|--------|
| 9.1 | Asset Import | Import FBX, GLTF, OBJ via Assimp/TinyGLTF | `Tools/Importer/` |
| 9.2 | Asset Export | Export to engine formats | `Tools/Importer/` |
| 9.3 | Asset Database | Index all assets with metadata, tags, previews | `Core/ResourceManager/` |
| 9.4 | Blender Addon | `blender --python` pipeline for models/rigs/anims | `Scripts/Python/Blender/` |
| 9.5 | AI Asset Agent | Generate assets from text prompts | `Agents/AssetAgent/` |
| 9.6 | Material Editor | Node-based material editor | `Editor/MaterialEditor/` |
| 9.7 | Asset Versioning | Local versioning, snapshot, rollback | `Versions/` |
| 9.8 | Semantic Search | AI-powered asset search (SBERT embeddings) | `AI/Embeddings/` |

**Dependencies:** Phase 2 (renderer), Phase 3 (editor), Phase 5 (AI), Phase 7 (PCG)

**Usable result:** Full asset pipeline from Blender → Engine → Editor with AI-generated assets.

---

## Phase 10 – Deployment, Docs & Polish

**Goal:** Build targets, server deployment, documentation, and final polish.

**Milestone:** Ship a build, deploy a server, generate docs — production-ready.

| # | Task | Description | Output |
|---|------|-------------|--------|
| ✅ 10.1 | Build Targets | Debug, Release, Editor builds via CMake | `Builds/` |
| ✅ 10.2 | Server Manager | Game server deployment (SteamCMD, config editor) | `Tools/ServerManager/` |
| ✅ 10.3 | Performance Profiler | CPU/GPU/memory profiling panels | `Editor/Panels/Profiler` |
| ✅ 10.4 | Replay Timeline | Record and replay simulations | `Tools/` |
| ✅ 10.5 | Doc Generator | Generate docs from code + AI | `Tools/DocGenerator/`, `Docs/` |
| ✅ 10.6 | PDF Manual | Editor-integrated manual with chapters | `Docs/Manual/MANUAL.md` |
| ✅ 10.7 | CI/CD Pipeline | Automated build, test, package | `Scripts/Tools/ci_pipeline.sh` |
| ✅ 10.8 | Cloud Backup | Optional cloud sync for projects | `Scripts/Tools/cloud_backup.sh` |
| ✅ 10.9 | Desktop/Mobile/Web Builds | Cross-platform export | `CMakePresets.json`, `Scripts/Tools/export_builds.sh` |
| ✅ 10.10 | VR/AR Preview | VR/AR preview mode in editor (OpenXR stub) | `Editor/Viewport/VRPreview.h` |
| ✅ 10.11 | Plugin Marketplace | Import/export plugin packs | `Plugins/Marketplace/` |

**Dependencies:** All previous phases

**Usable result:** A polished, deployable system with documentation, builds, and server tools.

---

## Dependency Graph

```
Phase 0 ──────────────────────────────────────────────────────────────
   │
   ▼
Phase 1 (Core) ──────────────────────────────────────────────────────
   │                    │                        │
   ▼                    ▼                        ▼
Phase 2 (Engine)     Phase 5 (AI Agents)      (standalone)
   │                    │
   ▼                    ▼
Phase 3 (Editor) ◄── Phase 5 (AI tools)
   │         │
   ▼         ▼
Phase 4    Phase 8 (IDE)
(Runtime)    │
   │         │
   ▼         ▼
Phase 6 ◄── Phase 7 (PCG) ◄── Phase 5 (AI-assisted PCG)
(Builder)
   │
   ▼
Phase 9 (Assets) ◄── Phase 5 + Phase 7
   │
   ▼
Phase 10 (Deploy)
```

**Key insight:** Phases 1-2-3-4 are the critical path for a usable engine/editor. Phase 5 (AI) can start in parallel after Phase 1, since it mainly needs the file system and core events.

---

## Work Path Summary

There are **three parallel work paths** that can be developed simultaneously:

### Path A: Engine → Editor → Runtime (Core Product)
```
Phase 0 → Phase 1 → Phase 2 → Phase 3 → Phase 4 → Phase 6
```
This is the **critical path** to a usable game engine with an editor.

### Path B: AI Agent System (Intelligence Layer)
```
Phase 0 → Phase 1 → Phase 5 → Phase 7 → Phase 8
```
This gives you the **offline AI assistant** that can generate code and assets.

### Path C: Assets & Tools (Content Pipeline)
```
Phase 0 → Phase 1 → Phase 2 → Phase 9 → Phase 10
```
This gives you the **asset pipeline** and deployment tools.

All three paths converge for the final product.

---

## Rules & Conventions

These rules are **locked in** across all phases (from implement2.md):
=======
## Appendix B — Rules & Conventions
>>>>>>> 7dd3a18993032daf9ba258c374ec661f55943367

| Rule | Description |
|------|-------------|
| **Never Delete** | Only archive — `Archive/` is the graveyard. AI learns from everything. |
| **Atlas Rule** | No ImGui — custom UI only, fully skinnable. |
| **Offline First** | All AI models run locally (Ollama / llama.cpp). |
| **Build Order** | Core → Engine → Runtime → Builder → UI → Editor → AI → Projects |
| **Namespace** | `Engine::` `Core::` `Editor::` `Runtime::` `AI::` `Builder::` `PCG::` |
| **Includes** | Always relative to repo root: `#include "Core/Events/Event.h"` |
| **Vec3 in modules** | Declare `using Vec3 = Engine::Math::Vec3;` inside namespace when needed |
| **AI Sandbox** | AI never writes to live project directly — outputs go to temp, user approves |
| **Workspace State** | Editor writes `WorkspaceState/` after every meaningful state change |

---

## Appendix C — Phases 18–32 (Completed)

<details>
<summary>Click to expand full phase history</summary>

| Phase | Subsystems |
|-------|-----------|
| 18 | RenderPipeline, AudioPipeline, AnimationPipeline, PCGPipeline, MetaLearning, RenderingOptimizer, Validator, Orchestrator, RulesEngine, MetricsDashboard, AdminConsole, CodeScaffolder, TestPipeline, MetricsReporter |
| 19 | NetworkManager, VoiceInput, DecisionVisualizer, DeterministicSeed, StateSync |
| 20 | SceneGraph impl, ConflictResolver, ResourceMonitor, TransactionManager, AnomalyOverlay, NPCController |
| 21 | NetContext, ShaderManager, DialogueSystem, BiomeGenerator, EventLog, PerformanceProfiler |
| 22 | UndoableCommandBus, EntityCommands, AnimationController, DungeonGenerator, CameraController, MemoryStore JSON load |
| 23 | GameSession, LODSystem, ConfigSystem, GoalPlanner, StoryGenerator, BenchmarkRunner |
| 24 | BehaviorTree, ParticleSystem, LuaBinding, VegetationGenerator, AssetPipeline, ProjectExplorer |
| 25 | AchievementSystem, PostProcessPipeline, WeatherSystem, MemoryProfiler, SnippetManager, EventDispatcher |
| 26 | LobbySystem, TerrainSystem, CityGenerator, ScriptRunner, TaskPanel, Signal |
| 27 | InputMapper, VoxelWorld, SpaceLayoutGenerator, DependencyAnalyzer, WatchPanel, RingBuffer |
| 28 | PhysicsWorld, ShadowMapper, CaveGenerator, SymbolLocator, OutputPanel, PoolAllocator |
| 29 | ComponentRegistry, OcclusionCuller, WorldChunkGen, GitHistory, BreakpointManager, ThreadPool |
| 30 | AssetStreamer, DecalSystem, QuestGraphGen, ReplayRecorder, LSPDiagnostics, BinarySerializer |
| 31 | CraftingSystem, LightManager, TextureGenerator, GPUProfiler, CodeSearchEngine, DataCompressor + 13 NFE ports |
| 32 | AudioMixer, AnimBlendTree, StructureGenerator, TextureAtlasPacker, RefactorPanel, LRUCache |

</details>
