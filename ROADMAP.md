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
| Editor — Integration | 14/14 | 0 | 14 |
| Runtime & Gameplay | 13/13 | 0 | 13 |
| AI & Agents | 12/12 | 0 | 12 |
| Builder System | 9/9 | 0 | 9 |
| PCG | 9/9 | 0 | 9 |
| IDE & Tools | 8/8 | 0 | 8 |
| Asset Pipeline | 8/8 | 0 | 8 |
| Deploy & CI | 11/11 | 0 | 11 |
| Game Project (NovaForge) | 10/10 | 0 | 10 |
| AI Agent System (Phase 5) | 12/12 | 0 | 12 |
| Builder–NovaForge Integration (Phase 36) | 9/9 | 0 | 9 |
| Audit: Undocumented Subsystems (Phase 35) | 37/37 | 0 | 37 |
| Suggested Next Features (Phase 37) | 12/12 | 0 | 12 |
| v10.7 Production Polish & AI Enhancement (Phase 38) | 10/10 | 0 | 10 |
| Shippable Game Foundations (Phase 39) | 10/10 | 0 | 10 |
| Live Services & Advanced AI (Phase 40) | 10/10 | 0 | 10 |
| Developer Experience & World Simulation (Phase 41) | 10/10 | 0 | 10 |
| Immersive Technologies & Live Infrastructure (Phase 42) | 10/10 | 0 | 10 |
| Game Systems & Engine Tooling (Phase 43) | 10/10 | 0 | 10 |
| Advanced Gameplay & Engine Utilities (Phase 44) | 10/10 | 0 | 10 |
| Rendering, AI, Core & Gameplay Plumbing (Phase 45) | 10/10 | 0 | 10 |
| Input, Audio, UI, Animation & Core Utilities (Phase 46) | 10/10 | 0 | 10 |
| Networking, Physics, AI & Platform Utilities (Phase 47) | 10/10 | 0 | 10 |
| **TOTAL** | **301/301** | **0** | **301** |

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

## Section 5 — Editor Integration ✅

> **All 131 tasks are complete. The full AtlasEditor + NovaForge game stack is implemented.**
> but panels show dummy/stub data. These tasks wire real engine systems into the editor UI.
> Work through them in order — each builds on the previous.

- [x] **EI-01 · Logger → Console sink**
      Requires: ED-12 (done), E-04 (done)
      Output: `Engine/Core/Logger.cpp` — add sink callback; `Editor/main.cpp` — register sink
      Steps:
        1. Add `Logger::SetSink(std::function<void(const std::string&)>)` to `Logger.h/.cpp`
        2. In `Editor/main.cpp`, after `renderer.Init()`, call:
           `Engine::Core::Logger::SetSink([&](const std::string& msg){ renderer.AppendConsole(msg); })`
      Why first: Every other integration step produces log output — console must capture it

- [x] **EI-02 · ECS world → scene outliner**
      Requires: EI-01, ED-09 (done), C-03 (done)
      Output: `Editor/Render/EditorRenderer.cpp` — replace `m_sceneObjects` hardcoded list
      Steps:
        1. Pass `Runtime::ECS::World*` pointer into `EditorRenderer` (add member + setter)
        2. In `DrawOutliner()`, iterate `world->GetEntities()` instead of `m_sceneObjects`
        3. Show entity ID + name component (or fallback to "Entity #N")
      Why second: All subsequent editor work assumes live entities are visible

- [x] **EI-03 · Mouse picking → entity selection**
      Requires: EI-02, ED-05 (done)
      Output: `Editor/Render/EditorRenderer.cpp` — `OnMouseButton()` handler
      Steps:
        1. On left-click inside viewport bounds, compute normalised device coords
        2. Call `ECS::World::QueryAtScreenPos()` (or iterate entities + AABB test)
        3. Store selected entity ID; call `GizmoSystem::SelectEntity()` with its transform
        4. Highlight selected entity in outliner

- [x] **EI-04 · Inspector shows real component properties**
      Requires: EI-03, C-03 (done)
      Output: `Editor/Panels/Inspector/InspectorTools.cpp` — wire to `Core::Reflection`
      Steps:
        1. On entity selection, iterate components via `Core::ForEachProperty(typeName, ...)`
        2. Render each property as an editable field (float slider, int box, string input)
        3. Call component setter on value change; record with `UndoableCommandBus`

- [x] **EI-05 · Gizmos rendered in viewport**
      Requires: EI-03, ED-05 (done)
      Output: `Editor/Render/EditorRenderer.cpp` — `DrawViewport()` calls gizmo draw
      Steps:
        1. After drawing the 3D grid, check `GizmoSystem::HasSelection()`
        2. Project entity world position to screen space
        3. Draw X/Y/Z axis arrows at entity position using coloured lines
        4. Highlight active drag axis on `GizmoSystem::GetActiveAxis()`

- [x] **EI-06 · Content browser reads filesystem**
      Requires: EI-01, ED-11 (done)
      Output: `Editor/Panels/ContentBrowser/ContentBrowser.cpp`
      Steps:
        1. On init, call `std::filesystem::recursive_directory_iterator` on asset root
        2. Build a tree of folders and files; filter by extension (.obj, .png, .json, .glsl)
        3. Render as a scrollable icon grid in the content browser panel
        4. Double-click on a scene file calls `SceneManager::Load()`

- [x] **EI-07 · Scene save and load from menu**
      Requires: EI-02, C-05 (done)
      Output: `Editor/main.cpp` — menu bar "File > Save Scene" / "File > Open Scene"
      Steps:
        1. Add menu bar click detection in `EditorRenderer::DrawMenuBar()`
        2. On "Save Scene", serialize `ECS::World` via `BinarySerializer` to `Projects/NovaForge/Scenes/`
        3. On "Open Scene", deserialize and repopulate `ECS::World`; refresh outliner

- [x] **EI-08 · Build button in editor**
      Requires: EI-01
      Output: `Editor/main.cpp` — menu bar "Build > Build All"
      Steps:
        1. On "Build" click, launch `Scripts/Tools/build_all.sh --debug-only` via `std::system()`
        2. Pipe stdout/stderr into `Logger`; errors appear in console and error panel
        3. Parse error lines; populate `ErrorPanel` list with file:line links

- [x] **EI-09 · Error panel click-to-navigate**
      Requires: EI-08
      Output: `Editor/Panels/ErrorPanel/ErrorPanel.cpp`
      Steps:
        1. Store errors as `{file, line, message}` structs
        2. On click, open the file in the IDE code editor panel (EI-12) at the correct line

- [x] **EI-10 · Project new / open dialog**
      Requires: EI-07
      Output: New file: `Editor/Panels/ProjectDialog/ProjectDialog.h/.cpp`
      Steps:
        1. On startup (or "File > New Project"), show a modal overlay
        2. "New" — create `Projects/<name>/` scaffold, default scene, open it
        3. "Open" — native file picker (or typed path) to select a project directory

- [x] **EI-11 · Undo / redo (Ctrl+Z / Ctrl+Y)**
      Requires: EI-04, C-07 (done)
      Output: `Editor/main.cpp` — key handler; `Editor/UndoableCommandBus.cpp`
      Steps:
        1. In `OnKey()`, detect Ctrl+Z → `UndoableCommandBus::Undo()`
        2. Detect Ctrl+Y / Ctrl+Shift+Z → `UndoableCommandBus::Redo()`
        3. Status bar shows last undone action name

- [x] **EI-12 · Code editor panel embedded**
      Requires: ED-02 (done), ED-08 (done)
      Output: `IDE/CodeEditor/CodeEditor.cpp` — wired into editor docking
      Steps:
        1. Add a docked "Code" panel to the editor layout
        2. `CodeEditor::OpenFile(path, line)` renders file content with syntax highlight
        3. Connect to `EI-09` so error clicks open the right file

- [x] **EI-13 · Play-in-editor (PIE)**
      Requires: EI-07, EI-02
      Output: `Editor/main.cpp` — toolbar Play/Stop buttons
      Steps:
        1. On "▶ Play", snapshot world state, then call `Engine::RunClient()` in a sub-context
        2. Render game output in the viewport (or a second window)
        3. On "■ Stop", restore the snapshotted world state

- [x] **EI-14 · AI chat panel connected to Ollama**
      Requires: EI-01
      Output: `IDE/AIChat/AIChat.cpp` — HTTP client to local Ollama API
      Steps:
        1. Add libcurl or a minimal HTTP socket client
        2. POST prompt to `http://localhost:11434/api/generate`
        3. Stream response tokens into the chat panel
        4. "Apply" button sends AI-generated code to the code editor panel

---

## Section 6 — Game Project (NovaForge) ✅

> Requires: EI-07 (scene save/load), EI-10 (project dialog), and EI-13 (PIE) to be useful.

- [x] **NF-01 · Default scene with basic entities**
      Requires: EI-02, EI-07
      Output: `Projects/NovaForge/Scenes/default.scene`
      Steps: Create a scene file with a player entity, a few asteroid entities, and a light

- [x] **NF-02 · Player controller wired up**
      Requires: NF-01, EI-13
      Output: `Runtime/Player/PlayerController.cpp` — WASD + mouse look working in PIE

- [x] **NF-03 · Inventory UI visible in game**
      Requires: NF-02
      Output: `Runtime/UI/HUD.cpp` — Tab key opens inventory overlay

- [x] **NF-04 · Crafting recipes loaded**
      Requires: NF-03, C-08
      Output: `Projects/NovaForge/Recipes/` — JSON recipe files loaded by `CraftingSystem`

- [x] **NF-05 · Builder mode in game**
      Requires: NF-02
      Output: `Runtime/BuilderRuntime/BuilderRuntime.cpp` — place/snap/weld modules

- [x] **NF-06 · PCG space station generated on start**
      Requires: NF-01
      Output: `PCG/Structures/StructureGenerator/` — call `Generate()` at scene load

- [x] **NF-07 · AI miner NPCs active**
      Requires: NF-06
      Output: `Runtime/Sim/AIMinerStateMachine/` — ticking in game world

- [x] **NF-08 · Save / load game state**
      Requires: NF-02, C-05
      Output: `Runtime/SaveLoad/SaveLoad.cpp` — F5 quicksave, F9 quickload

- [x] **NF-09 · Audio playing in game**
      Requires: NF-02
      Output: `Engine/Audio/AudioEngine.cpp` — background music + SFX on events

- [x] **NF-10 · Packaged Windows build**
      Requires: NF-08
      Output: `Builds/release/bin/NovaForge.exe` + asset bundle

---

## Section 7 — AI Agent Live Integration

> Requires: EI-14 (AI chat), and a local Ollama instance running.

- [x] **AI-LV-01 · Ollama connection verified**
      Requires: EI-14
      Output: `WorkspaceState/AgentState.json` — records connected model name
      Steps: Run `ollama pull codellama` or `ollama pull llama3`; confirm chat panel responds

- [x] **AI-LV-02 · Code generation from chat**
      Requires: AI-LV-01, EI-12
      Output: AI response → `Editor/Panels/Code/` — "Apply" inserts code at cursor

- [x] **AI-LV-03 · Build-error auto-fix**
      Requires: AI-LV-01, EI-08
      Output: `Agents/FixAgent/FixAgent.cpp` — on build failure, send errors to LLM, apply patch

- [x] **AI-LV-04 · PCG prompt-to-scene**
      Requires: AI-LV-01, NF-06
      Output: `Agents/PCGAgent/PCGAgent.cpp` — natural language → PCG rules → scene objects

---

## Section 8 — Polish & Advanced

- [x] **PL-01 · Dockable panel drag-resize**
      Requires: ED-02 (done)
      Output: `Editor/Docking/DockingSystem.cpp` — mouse-drag splitter bars

- [x] **PL-02 · Editor settings / preferences panel**
      Requires: EI-02
      Output: `Editor/Panels/Settings/SettingsPanel.h/.cpp`
      Settings: theme colour, key bindings, AI model, GLFW resolution

- [x] **PL-03 · Profiler panel live data**
      Requires: EI-01
      Output: `Editor/Panels/Profiler/ProfilerPanel.cpp` — wire `PerformanceProfiler` callbacks

- [x] **PL-04 · Material editor node graph**
      Requires: ED-08 (done)
      Output: `Editor/MaterialEditor/MaterialEditor.cpp` — node graph using `NodeEditor` framework

- [x] **PL-05 · Animated splash screen**
      Requires: ED-03 (done)
      Output: `Editor/main.cpp` — show splash overlay during `Engine.Init()` calls

- [x] **PL-06 · Ctrl+P quick open**
      Requires: EI-06, EI-12
      Output: Overlay with fuzzy search of assets + entities using `CodeSearchEngine`

- [x] **PL-07 · VR preview (OpenXR)**
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

## Phase 5 – AI Agent System ✅

**Goal:** Get a local AI agent running that can read/write project files and generate code.

**Milestone:** Chat with a local LLM, ask it to generate or modify code, and see results in the editor.

> **Status (March 2026): All 12 tasks implemented.** Files exist in `AI/` and `Agents/`.

| # | Task | Description | Output | Status |
|---|------|-------------|--------|--------|
| ✅ 5.1 | LLM Integration | Connect to Ollama / llama.cpp / local models | `AI/ModelManager/` (ModelManager + LLMBackend) | Done |
| ✅ 5.2 | Agent Core | Prompt → Plan → Tool → Result loop | `Agents/CodeAgent/CodeAgent.h` | Done |
| ✅ 5.3 | Tool System | Tool registry, tool runner, permissions | `Agents/CodeAgent/ToolSystem.h` | Done |
| ✅ 5.4 | File System Tools | `read_file`, `write_file`, `list_dir`, `search_code` | Agent tools wired | Done |
| ✅ 5.5 | Build Tools | `compile_cpp`, `run_cmake`, `run_tests` | `Agents/BuildAgent/BuildAgent.h` | Done |
| ✅ 5.6 | Memory System | Long-term context storage, embeddings | `AI/Memory/`, `AI/Embeddings/` | Done |
| ✅ 5.7 | Agent Scheduler | Multi-agent orchestration, task queue | `AI/AgentScheduler/AgentScheduler.h` | Done |
| ✅ 5.8 | Workspace State | Track open files, build state, errors | `WorkspaceState/` + `AI/Context/ProjectContext.h` | Done |
| ✅ 5.9 | Error Learning | Learn from build failures, auto-fix patterns | `AI/ErrorLearning/ErrorLearning.h` | Done |
| ✅ 5.10 | Code Learning | Index codebase for context-aware generation | `AI/CodeLearning/CodeLearning.h` | Done |
| ✅ 5.11 | Prompt Library | Reusable prompt templates | `AI/Prompts/PromptLibrary.h` | Done |
| ✅ 5.12 | Sandbox System | AI outputs to temp workspace, user approves/merges | `AI/Sandbox/Sandbox.h` | Done |

**Dependencies:** Phase 1 (events, tasks), Phase 0 (file structure)

**Usable result:** An AI assistant that can generate code files, fix compile errors, and modify the project — all offline.

---

## Phase 6 – Builder System ✅

**Goal:** Implement Starbase-style module building with snap points, welding, and assemblies.

**Milestone:** Place, snap, weld, and detach modules in the 3D editor. Parts have hitboxes and damage.

| # | Task | Description | Output |
|---|------|-------------|--------|
| ✅ 6.1 | Part Format | Part definition (mesh, collision, snap points, stats) | `Builder/Parts/PartDef.h` |
| ✅ 6.2 | Module Format | Module = group of parts, connection rules | `Builder/Modules/ModuleDef.h` |
| ✅ 6.3 | Snap System | Snap point matching, alignment, validation | `Builder/SnapRules/SnapRules.h` |
| ✅ 6.4 | Assembly System | Assemble/disassemble module groups | `Builder/Assembly/Assembly.h` |
| ✅ 6.5 | Builder Physics | Physics data for built structures | `Builder/PhysicsData/PhysicsData.h` |
| ✅ 6.6 | Builder Collision | Collision shapes from assembled parts | `Builder/Collision/CollisionShape.h` |
| ✅ 6.7 | Builder Damage | Per-part damage, destruction, repair | `Builder/Damage/PartDamage.h` |
| ✅ 6.8 | Builder Editor | Editor integration for building | `Editor/BuilderEditor/BuilderEditor.h` |
| ✅ 6.9 | Builder Runtime | Runtime building (in-game) | `Runtime/BuilderRuntime/BuilderRuntime.h` |

**Dependencies:** Phase 2 (renderer, physics), Phase 3 (editor, gizmos), Phase 4 (ECS, components)

**Usable result:** A Starbase-like building system where you snap modules together and test damage/physics.

---

## Phase 7 – Procedural Content Generation ✅

**Goal:** Procedurally generate geometry, textures, audio, worlds, and quests.

**Milestone:** Generate a procedural space station or terrain from rules and preview it in-editor.

| # | Task | Description | Output |
|---|------|-------------|--------|
| ✅ 7.1 | PCG Rule Engine | Define procedural rules (constraints, weights, seeds) | `PCG/Rules/PCGRule.h` |
| ✅ 7.2 | Geometry Generator | Procedural meshes, terrain, structures | `PCG/Geometry/MeshNode.h`, `LODGraph.h` |
| ✅ 7.3 | Texture Generator | Procedural textures (noise, patterns) | `PCG/Textures/TextureGenerator/` |
| ✅ 7.4 | Audio Generator | Procedural sound effects, ambient | `PCG/Audio/AudioGenerator.h` |
| ✅ 7.5 | World Generator | Procedural worlds, dungeons, stations | `PCG/World/`, `PCG/Dungeon/`, `PCG/Structures/` |
| ✅ 7.6 | Quest Generator | Procedural missions, objectives, branching | `PCG/Quests/QuestGraphGen/` |
| ✅ 7.7 | PCG Validation | Validate outputs (structural integrity, no overlaps) | `PCG/Validation/PCGValidator.h` |
| ✅ 7.8 | PCG Editor | Node-based rule editor with live preview | `Editor/PCGEditor/PCGEditor.h` |
| ✅ 7.9 | AI-Assisted PCG | AI generates/tunes PCG rules from prompts | `Agents/PCGAgent/PCGAgent.h` |

**Dependencies:** Phase 1 (serialization), Phase 2 (renderer), Phase 3 (editor, node editor), Phase 5 (AI agents)

**Usable result:** A procedural pipeline that generates content from rules — with AI assistance.

---

## Phase 8 – IDE & Integrated Tooling ✅

**Goal:** Embed a code editor and AI chat panel directly inside the editor.

**Milestone:** Write code, see AI suggestions, chat with AI — all inside the editor.

| # | Task | Description | Output |
|---|------|-------------|--------|
| ✅ 8.1 | Code Editor | Custom syntax-highlighted editor | `IDE/CodeEditor/CodeEditor.h` |
| ✅ 8.2 | Language Support | C++, Lua, Python, shader; LSP diagnostics | `IDE/LSP/LSPDiagnostics/` |
| ✅ 8.3 | AI Autocomplete | NL assistant for code completion | `IDE/NLAssistant/NLAssistant.h` |
| ✅ 8.4 | AI Chat Panel | Chat window connected to agent system | `IDE/AIChat/AIChat.h` |
| ✅ 8.5 | Console / Terminal | Filtered console log + output panel | `IDE/Console/ConsoleFilter/`, `IDE/OutputPanel/` |
| ✅ 8.6 | Debugger Integration | Breakpoint manager, step-through hooks | `IDE/Debugger/BreakpointManager/` |
| ✅ 8.7 | Git UI Panel | Commit, branch, diff, history from editor | `Tools/GitPanel/GitHistory/` |
| ✅ 8.8 | Error Panel | Show build errors, click-to-navigate | `Editor/Panels/ErrorPanel/` |

**Dependencies:** Phase 3 (editor framework, panels), Phase 5 (AI agent system)

**Usable result:** A fully integrated IDE inside the editor — code, debug, chat with AI, all in one place.

---

## Phase 9 – Asset Pipeline & Blender Integration ✅

**Goal:** Import, export, and procedurally generate assets via Blender and AI.

**Milestone:** Import a model from Blender, generate procedural textures, preview in editor.

| # | Task | Description | Output |
|---|------|-------------|--------|
| ✅ 9.1 | Asset Import | Import FBX, GLTF, OBJ via Assimp/TinyGLTF | `Tools/Importer/AssetImporter.h` |
| ✅ 9.2 | Asset Export | Export to engine formats | `Tools/Importer/AssetImporter.h` (export path) |
| ✅ 9.3 | Asset Database | Index all assets with metadata, tags, previews | `Core/ResourceManager/ResourceManager.h` |
| ✅ 9.4 | Blender Addon | `blender --python` pipeline for models/rigs/anims | `Scripts/Python/Blender/` |
| ✅ 9.5 | AI Asset Agent | Generate assets from text prompts | `Agents/AssetAgent/AssetAgent.h` |
| ✅ 9.6 | Material Editor | Node-based material editor | `Editor/MaterialEditor/MaterialEditor.h` |
| ✅ 9.7 | Asset Versioning | Local versioning, snapshot, rollback | `Versions/` |
| ✅ 9.8 | Semantic Search | AI-powered asset search (SBERT embeddings) | `AI/Embeddings/EmbeddingEngine.h` |

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

## Phase 35 – Audit: Undocumented Subsystems ✅

> **March 2026 full-repo audit** found 37 implemented subsystems that existed in the repository but had no ROADMAP entry. All are now tracked here. All are marked complete — implementations (`.h` + `.cpp`) verified to exist.

| # | Subsystem | Location | Notes |
|---|-----------|----------|-------|
| ✅ 35.01 | ArchiveLearning | `AI/ArchiveLearning/` | AI learning from archived code/data |
| ✅ 35.02 | AssetLearning | `AI/AssetLearning/` | AI learning patterns from game assets |
| ✅ 35.03 | BugTriage | `AI/BugTriage/` | Automated bug classification and prioritisation |
| ✅ 35.04 | ContextHelp | `AI/ContextHelp/` | Context-sensitive in-editor AI help |
| ✅ 35.05 | KnowledgeIngestion | `AI/KnowledgeIngestion/` | Offline doc/codebase ingestion manager |
| ✅ 35.06 | Tutorial | `AI/Tutorial/` | AI-driven interactive tutorial system |
| ✅ 35.07 | BuildAuditLog | `Builder/Core/BuildAuditLog.h` | Timestamped log of all builder actions |
| ✅ 35.08 | BuildQueue | `Builder/Assembly/BuildQueue.h` | Priority queue for async build operations |
| ✅ 35.09 | InteriorNode | `Builder/InteriorNode/InteriorNode.h` | Ship/station interior module slot management |
| ✅ 35.10 | CrashReport | `Core/CrashReport/` | Crash capture, minidump, auto-report |
| ✅ 35.11 | Database | `Core/Database/` | Local embedded key-value / SQL database |
| ✅ 35.12 | LocalCIPipeline | `Core/LocalCIPipeline/` | Dev-machine CI pipeline runner |
| ✅ 35.13 | NetworkProtocolGen | `Core/NetworkProtocolGen/` | Code-gen for typed network message schemas |
| ✅ 35.14 | PackageManager | `Core/PackageManager/` | Local package install/update/remove |
| ✅ 35.15 | GUIEditor | `Editor/GUIEditor/` | WYSIWYG UI layout editor |
| ✅ 35.16 | EditorToolRegistry | `Editor/Tools/EditorToolRegistry.h` | Tool plug-in registry for editor |
| ✅ 35.17 | MaterialOverrideTool | `Editor/Tools/MaterialOverrideTool.h` | Per-instance material override in viewport |
| ✅ 35.18 | TickScheduler | `Engine/Sim/TickScheduler.h` | Global tick ordering / budget controller |
| ✅ 35.19 | AnimationGraph | `Engine/Animation/AnimationGraph.h` | State-machine animation graph (complements AnimBlendTree) |
| ✅ 35.20 | FractalNoise | `PCG/Noise/FractalNoise/` | Fractal / fBm noise generator for PCG |
| ✅ 35.21 | PCGAdvanced | `PCG/Advanced/PCGAdvanced.h` | Higher-order PCG combinators and post-process |
| ✅ 35.22 | DeltaEditStore | `Runtime/ECS/DeltaEditStore.h` | Per-frame ECS delta recording for replay |
| ✅ 35.23 | SkillTree | `Runtime/Gameplay/SkillTree/` | Player skill tree with XP gates and effects |
| ✅ 35.24 | AIWalletSystem | `Runtime/Sim/AIWalletSystem/` | NPC economy: credits, transactions, market |
| ✅ 35.25 | CubeSphereLayout | `Runtime/World/CubeSphereLayout.h` | Planet-surface cube-sphere world layout |
| ✅ 35.26 | VoxelGridLayout | `Runtime/World/VoxelGridLayout.h` | Voxel-chunk world coordinate helpers |
| ✅ 35.27 | AnalyticsDashboard | `Tools/AnalyticsDashboard/` | Game telemetry + analytics visualisation |
| ✅ 35.28 | AssetProcessor | `Tools/AssetTools/AssetProcessor.h` | Batch asset conversion / compression |
| ✅ 35.29 | BuildAutomation | `Tools/BuildTools/BuildAutomation.h` | CMake project builder automation wrapper |
| ✅ 35.30 | CodeAudit | `Tools/CodeAudit/CodeAudit.h` | Static analysis + code-quality reporting |
| ✅ 35.31 | SimulationPlayback | `Tools/SimulationPlayback/` | Record and replay full simulation state |
| ✅ 35.32 | UIAnimation | `UI/Animation/UIAnimation.h` | Tweening and keyframe animation for UI widgets |
| ✅ 35.33 | UILogicGraph | `UI/GUISystem/UILogicGraph.h` | Node-graph for UI logic / state machine |
| ✅ 35.34 | UILayoutSolver | `UI/Layouts/UILayoutSolver.h` | Constraint-based UI layout solver |
| ✅ 35.35 | UILocalization | `UI/Localization/Localization.h` | UI-layer localization string lookup |
| ✅ 35.36 | Accessibility | `UI/Accessibility/Accessibility.h` | Colour-blind modes, font scaling, A11Y |
| ✅ 35.37 | InteractiveDocs | `IDE/InteractiveDocs/` | Live interactive API documentation browser |

---

## Phase 36 – Builder–NovaForge Integration ✅

**Goal:** Wire every Builder subsystem into NovaForge's gameplay so the player can build ships, stations, rovers, and free-form structures entirely in-game.

> **Implemented March 2026.** `NovaForgeBuilderIntegration` is the single game-facing API.

| # | Task | Description | Output |
|---|------|-------------|--------|
| ✅ NF-BLD-01 | PartLibrary JSON loader | Load `Projects/NovaForge/Parts/*.json` into `Builder::PartLibrary` at startup | `NovaForgeBuilderIntegration::LoadPartLibrary()` |
| ✅ NF-BLD-02 | Ship build sessions | Open/save `BuildSession` per ship; pre-load from `Ships/*.json` templates | `NovaForgeBuilderIntegration::LoadShipTemplates()` |
| ✅ NF-BLD-03 | Station build sessions | Separate `BuildTargetType::Station` sessions for orbital stations & outposts | `OpenBuildSession(Station, …)` |
| ✅ NF-BLD-04 | Interior module management | `Builder::InteriorNode` wired in; Bridge/PowerPlant/Habitat/Storage default slots | `AddInteriorSlot()` / `PlaceModule()` |
| ✅ NF-BLD-05 | Snap compatibility rules | Hull↔hull, engine↔thruster, weapon↔hardpoint, interior↔interior registered | `RegisterSnapCompatibility()` |
| ✅ NF-BLD-06 | Crafting–builder gate | Part placement checks player inventory; consumes materials on success | `SetCraftingGate()` / `SetConsumePart()` |
| ✅ NF-BLD-07 | Runtime damage integration | `BuilderDamageSystem` reinitialised on every PlacePart; `ApplyDamage` / `RepairAll` in-game | `ApplyDamage()` / `RepairAll()` |
| ✅ NF-BLD-08 | Assembly save/load | All dirty sessions saved to `Saves/build_session_N.json` on QuickSave | `SaveSession()` / `LoadBuildSession()` |
| ✅ NF-BLD-09 | Collision shape generation | `CollisionBuilder::BuildForAssembly()` available for physics integration | `BuildCollision()` |

**New file:** `Runtime/BuilderRuntime/NovaForgeBuilderIntegration.h/.cpp`
**Updated:** `Projects/NovaForge/main.cpp` (replaces stub comment with full builder init)

---

## Phase 37 – Suggested Next Features

> Recommended additions to bring NovaForge and the AtlasEditor to a shippable state.
> None implemented yet — these are the next logical steps.

| # | Task | Priority | Description | Suggested Output |
|---|------|----------|-------------|-----------------|
| [x] 37.01 | Multiplayer sync | High | Client–server ECS delta sync, lag compensation, rollback netcode | `Engine/Net/` + `Runtime/StateSync/` |
| [x] 37.02 | Economy system | High | Dynamic market prices, trade routes, scarcity, faction supply chains | `Runtime/Economy/MarketSystem.h` |
| [x] 37.03 | Faction & reputation | High | Allegiances, diplomatic states, AI faction behaviour | `Runtime/Factions/FactionSystem.h` |
| [x] 37.04 | Ship-to-ship combat | High | Targeting, weapon arcs, shield facings, missile guidance | `Runtime/Combat/CombatSystem.h` |
| [x] 37.05 | Player progression | Medium | XP, tech unlocks, talent points feeding `SkillTree` | `Runtime/Gameplay/ProgressionSystem.h` |
| [x] 37.06 | Modding / Lua hooks | Medium | `LuaBinding` hooks for game events, items, PCG rules, AI triggers | `Core/Scripting/ModLoader.h` |
| [x] 37.07 | Environmental hazards | Medium | Radiation zones, asteroid fields, gravity wells, nebula effects | `Runtime/Hazards/HazardSystem.h` |
| [x] 37.08 | AI fleet intelligence | Medium | Formation flying, escort/intercept, patrol routes for NPC fleets | `Runtime/AI/FleetController.h` |
| [x] 37.09 | Procedural narrative | Medium | Dynamic dialogue driven by player actions, reputation, and quests | `PCG/Narrative/NarrativeEngine.h` |
| [x] 37.10 | Full UI theme system | Low | Apply `UIStyle` live; ship-HUD vs station-HUD skins | `UI/Themes/ThemeManager.h` |
| [x] 37.11 | Build-mode preview | Low | Ghost-part snap preview in EditorRenderer viewport | `Editor/BuilderEditor/` extension |
| [x] 37.12 | Integration test suite | Low | CTest target: core game loop, builder, PCG, save/load | `Tests/Integration/` |

---

## Phase 38 – v10.7 Production Polish & AI Enhancement ✅

> Implements the optional enhancements outlined in the MasterRepo v10.7 Ultra Blueprint.
> All 10 tasks implemented March 2026.

| # | Task | Priority | Description | Output |
|---|------|----------|-------------|--------|
| ✅ 38.01 | AI Reasoning Visualizer | High | Show why AI made each suggestion — evidence, decision steps, confidence, alternatives | `AI/ReasoningVisualizer/ReasoningVisualizer.h/.cpp` |
| ✅ 38.02 | Cyclic Dependency Resolver | High | Detect and break cyclic deps in AI suggestion graphs and PCG rule sets (Tarjan SCC) | `AI/CyclicDependencyResolver/CyclicDependencyResolver.h/.cpp` |
| ✅ 38.03 | Session Memory Manager | High | Persistent multi-session key-value + embedding memory for AI agents | `AI/SessionMemory/SessionMemory.h/.cpp` |
| ✅ 38.04 | AI Suggestion Sandbox | High | Multi-step AI suggestion staging with Preview / Commit / Reject / Undo / Redo | `AI/SuggestionSandbox/SuggestionSandbox.h/.cpp` |
| ✅ 38.05 | Anomaly Alerting System | Medium | Threshold + z-score alerting for AI/PCG metric outliers with rate limiting | `AI/AnomalyAlerting/AnomalyAlerting.h/.cpp` |
| ✅ 38.06 | Build Report Generator | Medium | Pull-request-style Markdown reports: metrics, suggestions, anomalies, notes | `AI/BuildReport/BuildReport.h/.cpp` |
| ✅ 38.07 | Deterministic PCG Verifier | Medium | Run any registered PCG generator twice with the same seed and diff outputs | `PCG/DeterministicVerifier/DeterministicVerifier.h/.cpp` |
| ✅ 38.08 | Asset Metadata Enhancer | Medium | Versioned asset records with AI reasoning annotations, tag search, JSON persistence | `Core/AssetMetadata/AssetMetadata.h/.cpp` |
| ✅ 38.09 | Headless Server Runtime | Low | Embedded headless game loop for offline AI + PCG batch processing and CI | `Runtime/HeadlessServer/HeadlessServer.h/.cpp` |
| ✅ 38.10 | AI Profiler Panel | Low | Editor panel: per-frame AI/PCG CPU time, queue depth, peak/avg graphs | `Editor/Panels/AIProfiler/AIProfilerPanel.h/.cpp` |

---

## Phase 39 – Shippable Game Foundations ✅

> Fills the remaining production gaps to make NovaForge and AtlasEditor fully release-ready.
> All 10 tasks implemented March 2026.

| # | Task | Priority | Description | Output |
|---|------|----------|-------------|--------|
| ✅ 39.01 | Runtime Analytics | High | Session-scoped gameplay event tracking, funnel steps, counters, JSONL flush | `Runtime/Analytics/RuntimeAnalytics.h/.cpp` |
| ✅ 39.02 | Cinematic System | High | Catmull-Rom spline camera paths, keyframe FOV/exposure, play/pause/seek/loop | `Engine/Cinematic/CinematicSystem.h/.cpp` |
| ✅ 39.03 | Gamepad Input System | High | Full controller support: buttons, axes, deadzone, rumble, action mapping | `Engine/Input/Gamepad/GamepadSystem.h/.cpp` |
| ✅ 39.04 | Graphics Settings Manager | High | Preset/custom quality, resolution scale, MSAA, shadows, dynamic auto-scaling | `Engine/Graphics/GraphicsSettings.h/.cpp` |
| ✅ 39.05 | Save Game Manager | High | Named slots, schema versioning + migration, auto-save, playtime tracking | `Runtime/SaveGame/SaveGameManager.h/.cpp` |
| ✅ 39.06 | Packaging Pipeline | Medium | Asset collection, compression (LZ4/Zstd), .pak bundle + manifest generation | `Builder/Packaging/PackagingPipeline.h/.cpp` |
| ✅ 39.07 | Telemetry Reporter | Medium | Opt-in crash reports, feature usage, performance outliers, local JSONL storage | `Core/Telemetry/TelemetryReporter.h/.cpp` |
| ✅ 39.08 | Network Replication Layer | Medium | ECS component delta replication, dirty tracking, interest range, full snapshot | `Engine/Net/Replication/NetworkReplication.h/.cpp` |
| ✅ 39.09 | Cloud Sync Manager | Low | Offline-first push/pull/sync for saves and workspaces, conflict resolution | `Core/CloudSync/CloudSyncManager.h/.cpp` |
| ✅ 39.10 | Ship Design Exporter | Low | Export assemblies to Blueprint JSON / OBJ / PAK / GLB; batch export support | `Builder/Export/ShipExporter.h/.cpp` |

---

## Phase 40 – Live Services & Advanced AI ✅

> Closes the remaining gaps in AI tooling, world simulation, and live-game services.
> All 10 tasks implemented March 2026.

| # | Task | Priority | Description | Output |
|---|------|----------|-------------|--------|
| ✅ 40.01 | Prompt Template Engine | High | Parameterised `{{var}}` prompt templates with conditionals, partials, filters | `AI/PromptTemplate/PromptTemplateEngine.h/.cpp` |
| ✅ 40.02 | Multi-Agent Coordinator | High | Fan-out, pipeline, and voting coordination over registered AI agents | `AI/MultiAgent/MultiAgentCoordinator.h/.cpp` |
| ✅ 40.03 | Spatial Audio System | High | 3D positional audio: HRTF panning, distance attenuation, reverb zones, Doppler | `Engine/Audio/Spatial/SpatialAudio.h/.cpp` |
| ✅ 40.04 | Planet Generator | High | Procedural planet via fBm sphere: biome map, albedo, heightmap, PGM/PPM export | `PCG/Planet/PlanetGenerator.h/.cpp` |
| ✅ 40.05 | Leaderboard System | Medium | Local-first named boards, score submission, rank queries, JSON persistence | `Runtime/Leaderboard/LeaderboardSystem.h/.cpp` |
| ✅ 40.06 | Feature Flag System | Medium | Bool / percentage / variant flags, user-bucket hashing, runtime overrides | `Core/FeatureFlags/FeatureFlagSystem.h/.cpp` |
| ✅ 40.07 | Timeline Editor Panel | Medium | Visual timeline for cinematics: tracks, keyframes, playhead scrub, transport | `Editor/Panels/TimelineEditor/TimelineEditorPanel.h/.cpp` |
| ✅ 40.08 | Notification System | Medium | Priority-queued toasts with categories, muting, expiry, and action callbacks | `Runtime/Notification/NotificationSystem.h/.cpp` |
| ✅ 40.09 | NavMesh Pathfinding | High | A* over polygon nav-graph, dynamic obstacles, area costs, smooth paths | `Engine/Pathfinding/NavMesh/NavMesh.h/.cpp` |
| ✅ 40.10 | Sampling Profiler | Low | CPU sampling profiler: flat profile, flame graph JSON export, CSV flat table | `Core/Profiling/SamplingProfiler.h/.cpp` |

---

## Phase 41 – Developer Experience & World Simulation ✅

> Closes gaps in shader authoring, AI editing, simulation fidelity, and runtime tooling.
> All 10 tasks implemented March 2026.

| # | Task | Priority | Description | Output |
|---|------|----------|-------------|--------|
| ✅ 41.01 | Shader Graph | High | Node-based visual shader editor: texture/math/utility nodes, GLSL/HLSL compiler, JSON save/load | `Engine/Shader/ShaderGraph/ShaderGraph.h/.cpp` |
| ✅ 41.02 | Behavior Tree Editor | High | Visual BT editor panel: Selector/Sequence/Leaf nodes, live PIE run-state overlay | `Editor/Panels/BehaviorTreeEditor/BehaviorTreeEditorPanel.h/.cpp` |
| ✅ 41.03 | Fluid Simulation | High | Eulerian Navier-Stokes (2D/3D): velocity advection, pressure projection, smoke density, PGM export | `Engine/Sim/FluidSim/FluidSimulation.h/.cpp` |
| ✅ 41.04 | Event Replay System | High | Deterministic record/replay of all engine events: save, seek, speed control, JSON serialization | `Core/EventReplay/EventReplaySystem.h/.cpp` |
| ✅ 41.05 | Crowd Simulation | High | Boids-style steering + NavMesh goal-seeking for 1000+ agents with formation groups | `Runtime/Crowd/CrowdSimulation.h/.cpp` |
| ✅ 41.06 | Data Table System | Medium | Row-keyed typed tables (CSV/JSON): hot-reload, DLC patch-merge, typed accessors | `Core/DataTable/DataTableSystem.h/.cpp` |
| ✅ 41.07 | Procedural Animation | Medium | Runtime IK (TwoBone + FABRIK), look-at, spring bones, terrain foot placement, ragdoll blend | `Engine/Animation/ProceduralAnim/ProceduralAnimation.h/.cpp` |
| ✅ 41.08 | Blueprint Compiler | Medium | Visual node-graph → Lua or C++ code generation with type validation and API registry | `Builder/Blueprint/BlueprintCompiler.h/.cpp` |
| ✅ 41.09 | Minimap System | Medium | World-to-texture minimap: fog of war, icon layers, zoom, region labels, fog reveal | `Runtime/Minimap/MinimapSystem.h/.cpp` |
| ✅ 41.10 | GPU Particle System | Low | Emitter-based particle sim with forces (gravity/wind/attractor), colour gradients, sub-emitters | `Engine/Particles/GPUParticles/GPUParticleSystem.h/.cpp` |

---

## Phase 42 – Immersive Technologies & Live Infrastructure ✅

> Fills key gaps in splines, weather, dialogue, input, LOD, RPC, light baking, objectives, voice,
> and open-world streaming.  All 10 tasks implemented March 2026.

| # | Task | Priority | Description | Output |
|---|------|----------|-------------|--------|
| ✅ 42.01 | Spline System | High | Catmull-Rom + Bezier splines, arc-length param, Frenet frame, nearest-point, JSON | `Engine/Spline/SplineSystem.h/.cpp` |
| ✅ 42.02 | Weather System | High | Procedural weather (Clear→Blizzard), smooth blend, time-of-day, thunder events | `PCG/Weather/WeatherSystem/WeatherSystem.h/.cpp` |
| ✅ 42.03 | Conversation Graph | High | Node-graph dialogue: conditions, effects, choices, localization, JSON save/load | `Runtime/Dialogue/ConversationGraph/ConversationGraph.h/.cpp` |
| ✅ 42.04 | Input Remapper | High | Runtime rebind: keyboard/mouse/gamepad, axis support, conflict detection, profiles | `Engine/Input/Remapping/InputRemapper.h/.cpp` |
| ✅ 42.05 | LOD Manager | Medium | Distance-based LOD selection, cross-fade blend, screen-size mode, global bias | `Engine/Lod/LODManager/LODManager.h/.cpp` |
| ✅ 42.06 | RPC System | Medium | Typed remote procedure calls: reliable/unreliable, broadcast/unicast, arg serialization | `Engine/Net/RPC/RPCSystem.h/.cpp` |
| ✅ 42.07 | Light Baker | Medium | CPU path-traced lightmap baker: direct shadows, 1-bounce GI, AO, async threading | `Engine/Lighting/LightBaker/LightBaker.h/.cpp` |
| ✅ 42.08 | Objective System | Medium | Quest objectives: conditions, time limits, chaining, persistence, reward descriptors | `Runtime/Gameplay/ObjectiveSystem/ObjectiveSystem.h/.cpp` |
| ✅ 42.09 | Voice Manager | Low | VO line queue with priority, 3D position, subtitles, lip-sync paths, cooldowns | `Runtime/Audio/VoiceManager/VoiceManager.h/.cpp` |
| ✅ 42.10 | Scene Streaming | High | Open-world chunk streaming: load/unload radius, memory budget, soft unload, callbacks | `Runtime/Streaming/SceneStreamingSystem/SceneStreamingSystem.h/.cpp` |

---

## Phase 43 – Game Systems & Engine Tooling ✅

> Core RPG / action gameplay systems plus essential engine utilities.
> All 10 tasks implemented March 2026.

| # | Task | Priority | Description | Output |
|---|------|----------|-------------|--------|
| ✅ 43.01 | Interaction System | High | Proximity/ray interactable registration, candidate sorting, cooldown, one-shot | `Runtime/Gameplay/InteractionSystem/InteractionSystem.h/.cpp` |
| ✅ 43.02 | Damage Pipeline | High | Typed damage (Physical…True), modifiers, resistances, crit roll, entity HP | `Runtime/Combat/DamagePipeline/DamagePipeline.h/.cpp` |
| ✅ 43.03 | Ability System | High | GAS-style: grant/revoke, mana/stamina cost, cooldown, charges, tags, regen | `Runtime/Combat/AbilitySystem/AbilitySystem.h/.cpp` |
| ✅ 43.04 | Inventory System | High | Grid inventory, stacking, weight, equipment slots, transfer, JSON persist | `Runtime/Inventory/InventorySystem/InventorySystem.h/.cpp` |
| ✅ 43.05 | Status Effect System | Medium | Buffs/debuffs: DoT tick, stacks (Refresh/Additive/Max), immunity tags | `Runtime/Combat/StatusEffects/StatusEffectSystem.h/.cpp` |
| ✅ 43.06 | Game Event Bus | Medium | Pub-sub bus: priority, one-shot, filter predicate, deferred (thread-safe) | `Core/Events/GameEventBus/GameEventBus.h/.cpp` |
| ✅ 43.07 | Texture Atlas Builder | Medium | Shelf-packing sprite atlas: RGBA blit, UV map, PPM+JSON export | `Engine/Render/TextureAtlas/TextureAtlasBuilder.h/.cpp` |
| ✅ 43.08 | Debug Draw | Low | Immediate-mode debug overlay: lines, spheres, boxes, text, groups, lifetime | `Engine/Debug/DebugDraw/DebugDraw.h/.cpp` |
| ✅ 43.09 | Constraint Solver | Medium | Sequential-impulse solver: distance, spring, hinge, slider constraints | `Engine/Physics/ConstraintSolver/ConstraintSolver.h/.cpp` |
| ✅ 43.10 | Screenshot System | Low | Frame capture (PNG/JPEG), supersampling, sequence capture, async save | `Engine/Render/Screenshot/ScreenshotSystem.h/.cpp` |

---

## Appendix B — Rules & Conventions

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

## Appendix C — Phases 18–36 (Completed)

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
| 33 | AssetBrowser (`Tools/AssetTools/AssetBrowser`), ConsoleFilter (`IDE/Console/ConsoleFilter`), HotReloadManager (`Core/HotReload/HotReloadManager`), ToolsGUI layer (`Tools/GUI`: IToolPanel + ToolsApp + 8 panels) |
| 34 | LocalizationSystem (`Core/Localization/LocalizationSystem`), RaycastSystem (`Engine/Raycast/RaycastSystem`), BreadcrumbBar (`IDE/Breadcrumb/BreadcrumbBar`), ChangeTracker (`Tools/ChangeTracker/ChangeTracker`), StreamingResponse (`AI/StreamingResponse`) |
| 35 | Audit — 37 undocumented subsystems catalogued (ArchiveLearning, AssetLearning, BugTriage, ContextHelp, KnowledgeIngestion, Tutorial, BuildAuditLog, BuildQueue, InteriorNode, CrashReport, Database, LocalCIPipeline, NetworkProtocolGen, PackageManager, GUIEditor, EditorToolRegistry, MaterialOverrideTool, TickScheduler, AnimationGraph, FractalNoise, PCGAdvanced, DeltaEditStore, SkillTree, AIWalletSystem, CubeSphereLayout, VoxelGridLayout, AnalyticsDashboard, AssetProcessor, BuildAutomation, CodeAudit, SimulationPlayback, UIAnimation, UILogicGraph, UILayoutSolver, UILocalization, Accessibility, InteractiveDocs) |
| 36 | Builder–NovaForge Integration — `NovaForgeBuilderIntegration` (Runtime/BuilderRuntime): PartLibrary loader, ship/station/rover build sessions, InteriorNode slots, snap rules, crafting gate, damage system, collision gen, assembly save/load. `Projects/NovaForge/main.cpp` fully wired. |
| 37 | Suggested Next Features — Multiplayer sync, Economy system, Faction & reputation, Ship-to-ship combat, Player progression, Modding/Lua hooks, Environmental hazards, AI fleet intelligence, Procedural narrative, Full UI theme system, Build-mode preview, Integration test suite. |
| 38 | v10.7 Production Polish & AI Enhancement — ReasoningVisualizer, CyclicDependencyResolver, SessionMemory, SuggestionSandbox, AnomalyAlerting, BuildReport, DeterministicVerifier (PCG), AssetMetadata (Core), HeadlessServer (Runtime), AIProfilerPanel (Editor). |
| 39 | Shippable Game Foundations — RuntimeAnalytics (Runtime), CinematicSystem (Engine), GamepadSystem (Engine/Input), GraphicsSettings (Engine), SaveGameManager (Runtime), PackagingPipeline (Builder), TelemetryReporter (Core), NetworkReplication (Engine/Net), CloudSyncManager (Core), ShipExporter (Builder). |
| 40 | Live Services & Advanced AI — PromptTemplateEngine (AI), MultiAgentCoordinator (AI), SpatialAudio (Engine/Audio), PlanetGenerator (PCG), LeaderboardSystem (Runtime), FeatureFlagSystem (Core), TimelineEditorPanel (Editor/Panels), NotificationSystem (Runtime), NavMesh (Engine/Pathfinding), SamplingProfiler (Core/Profiling). |
| 41 | Developer Experience & World Simulation — ShaderGraph (Engine/Shader), BehaviorTreeEditorPanel (Editor/Panels), FluidSimulation (Engine/Sim), EventReplaySystem (Core), CrowdSimulation (Runtime), DataTableSystem (Core), ProceduralAnimation (Engine/Animation), BlueprintCompiler (Builder), MinimapSystem (Runtime), GPUParticleSystem (Engine/Particles). |
| 42 | Immersive Technologies & Live Infrastructure — SplineSystem (Engine/Spline), WeatherSystem (PCG/Weather), ConversationGraph (Runtime/Dialogue), InputRemapper (Engine/Input/Remapping), LODManager (Engine/Lod), RPCSystem (Engine/Net/RPC), LightBaker (Engine/Lighting), ObjectiveSystem (Runtime/Gameplay), VoiceManager (Runtime/Audio), SceneStreamingSystem (Runtime/Streaming). |
| 43 | Game Systems & Engine Tooling — InteractionSystem (Runtime/Gameplay), DamagePipeline (Runtime/Combat), AbilitySystem (Runtime/Combat), InventorySystem (Runtime/Inventory), StatusEffectSystem (Runtime/Combat), GameEventBus (Core/Events), TextureAtlasBuilder (Engine/Render), DebugDraw (Engine/Debug), ConstraintSolver (Engine/Physics), ScreenshotSystem (Engine/Render). |
| 44 | Advanced Gameplay & Engine Utilities — WaterSystem (Engine/Water), FootstepSystem (Runtime/Audio), ComboSystem (Runtime/Combat), PrefabSystem (Engine/Scene), GameModeManager (Runtime/Gameplay), SurfaceAnalyzer (Engine/Physics), AnimNotifySystem (Engine/Animation), TagSystem (Core/Tags), CameraShakeSystem (Engine/Camera), PickupSystem (Runtime/Gameplay). |
| 45 | Rendering, AI, Core & Gameplay Plumbing — TweenSystem (Engine/Tween), WeaponSystem (Runtime/Combat), MaterialSystem (Engine/Render/Material), TimeManager (Core/Time), AsyncTaskQueue (Core/AsyncTask), StateMachine (Core/StateMachine), EnvironmentQuery (Engine/AI/EQS), RewardSystem (Runtime/Gameplay), PostProcessVolume (Engine/PostProcess), TriggerVolume (Engine/Physics). |
| 46 | Input, Audio, UI, Animation & Core Utilities — InputSystem (Engine/Input/InputSystem), SoundSystem (Engine/Audio/SoundSystem), UISystem (Runtime/UI/UISystem), ObjectPool (Core/Pool/ObjectPool), DialogueManager (Runtime/Dialogue/DialogueManager), MeshBuilder (Engine/Render/MeshBuilder), SceneTransitionSystem (Runtime/Scene/SceneTransitionSystem), SkeletonSystem (Engine/Animation/Skeleton), EventQueue (Core/EventQueue/EventQueue), FontSystem (Engine/Font/FontSystem). |
| 47 | Networking, Physics, AI & Platform Utilities — NetworkManager (Runtime/Network/NetworkManager), RigidBodySystem (Engine/Physics/RigidBody), BehaviorTree (Engine/AI/BehaviorTree), AchievementSystem (Runtime/Achievement/AchievementSystem), AssetImporter (Engine/Asset/AssetImporter), ParticleSystem (Engine/Particles/ParticleSystem), ChunkSystem (Engine/World/ChunkSystem), ConfigSystem (Core/Config/ConfigSystem), AnimationBlendTree (Engine/Animation/BlendTree), ScriptingBridge (Engine/Scripting/ScriptingBridge). |

</details>

### Phase Voxel-Chisel (Mar 2026)
- [x] VoxelChiselSystem — 16×16×16 sub-voxel chisel blocks; VoxelCell, VoxelBlock, VoxelChunk, VoxelVolume; SetCell, FillBox, CarveBox, ChiselSphere ops; op history for undo (PCG/Voxel/VoxelChiselSystem/)
- [x] VoxelPCGGen — PCG driver bridges noise/biome/cave/structure/planet systems into voxel volumes; GenerateTerrain, CarveCaves, StampStructure, GeneratePlanetSurface, GenerateAsteroid, GenerateShipHull (PCG/Voxel/VoxelPCGGen/)

---

## Phase 44 – Advanced Gameplay & Engine Utilities ✅

> Fills in water simulation, footsteps, combo chains, prefabs, game-mode rules, surface
> detection, animation notifies, entity tags, camera shake, and item pickups.
> All 10 tasks implemented March 2026.

| # | Task | Priority | Description | Output |
|---|------|----------|-------------|--------|
| ✅ 44.01 | Water System | High | Gerstner waves, buoyancy impulse, swim-state machine, wetness, JSON save/load | `Engine/Water/WaterSystem/WaterSystem.h/.cpp` |
| ✅ 44.02 | Footstep System | High | Surface-aware footstep audio/VFX, stride-length steps, landing/jump events, blended surfaces | `Runtime/Audio/FootstepSystem/FootstepSystem.h/.cpp` |
| ✅ 44.03 | Combo System | High | Attack combo chains, timing link windows (early/perfect/late), branching, finishers | `Runtime/Combat/ComboSystem/ComboSystem.h/.cpp` |
| ✅ 44.04 | Prefab System | High | Prefab templates, nested prefabs, per-instance overrides, live propagation, JSON | `Engine/Scene/PrefabSystem/PrefabSystem.h/.cpp` |
| ✅ 44.05 | Game Mode Manager | High | Match phases (Countdown→Running→PostGame), score/kill tracking, win conditions, respawn | `Runtime/Gameplay/GameModeManager/GameModeManager.h/.cpp` |
| ✅ 44.06 | Surface Analyzer | Medium | Surface material registry, raycast+contact-point resolution, blend weights, batch query | `Engine/Physics/SurfaceAnalyzer/SurfaceAnalyzer.h/.cpp` |
| ✅ 44.07 | Anim Notify System | Medium | Keyframe-timed animation events, state notifies (enter/tick/exit), per-instance playback | `Engine/Animation/AnimNotify/AnimNotifySystem.h/.cpp` |
| ✅ 44.08 | Tag System | Medium | Hierarchical dot-notation tags, exclusive groups, batch ops, thread-safe query | `Core/Tags/TagSystem/TagSystem.h/.cpp` |
| ✅ 44.09 | Camera Shake System | Medium | Trauma/decay model, Perlin-noise offsets, presets, directional bias, distance scale | `Engine/Camera/CameraShake/CameraShakeSystem.h/.cpp` |
| ✅ 44.10 | Pickup System | Low | Type registry, spawn/despawn, proximity collection, magnet, respawn timer, team lock | `Runtime/Gameplay/PickupSystem/PickupSystem.h/.cpp` |

---

## Phase 45 – Rendering, AI, Core & Gameplay Plumbing ✅

> Adds tweening, weapons, material instances, time management, async tasks,
> hierarchical FSMs, AI environment queries, XP/loot rewards, post-process volumes,
> and trigger volumes. All 10 tasks implemented March 2026.

| # | Task | Priority | Description | Output |
|---|------|----------|-------------|--------|
| ✅ 45.01 | Tween System | High | Value interpolation with 26 easing modes, sequences, loops, yoyo, callbacks | `Engine/Tween/TweenSystem/TweenSystem.h/.cpp` |
| ✅ 45.02 | Weapon System | High | Weapon registry, fire modes (semi/burst/full-auto/charge), ammo/reload lifecycle | `Runtime/Combat/WeaponSystem/WeaponSystem.h/.cpp` |
| ✅ 45.03 | Material System | High | Material instance registry, parameter overrides, layers, change callbacks | `Engine/Render/Material/MaterialSystem/MaterialSystem.h/.cpp` |
| ✅ 45.04 | Time Manager | High | Game-time scale (tween), pause/unpause, fixed-timestep accumulator, per-actor scale, timers | `Core/Time/TimeManager/TimeManager.h/.cpp` |
| ✅ 45.05 | Async Task Queue | High | Thread-pool task queue, priorities, progress, cancellation, batch completion | `Core/AsyncTask/AsyncTaskQueue/AsyncTaskQueue.h/.cpp` |
| ✅ 45.06 | State Machine | Medium | Typed template FSM: states, guarded transitions, enter/tick/exit, hierarchy | `Core/StateMachine/StateMachine/StateMachine.h/.cpp` |
| ✅ 45.07 | Environment Query | Medium | EQS: grid/ring/random generators, LOS/distance/direction/custom scoring, async queries | `Engine/AI/EQS/EnvironmentQuery/EnvironmentQuery.h/.cpp` |
| ✅ 45.08 | Reward System | Medium | XP rules, loot tables, rarity tiers + luck, level-up callbacks, item drop dispatch | `Runtime/Gameplay/RewardSystem/RewardSystem.h/.cpp` |
| ✅ 45.09 | Post-Process Volume | Medium | AABB/sphere volumes blend PP settings (bloom, DOF, fog, LUT) by proximity | `Engine/PostProcess/PostProcessVolume/PostProcessVolume.h/.cpp` |
| ✅ 45.10 | Trigger Volume | Medium | AABB/sphere/capsule overlap detection, enter/stay/exit callbacks, one-shot, cooldown | `Engine/Physics/TriggerVolume/TriggerVolume.h/.cpp` |

---

## Phase 46 – Input, Audio, UI, Animation & Core Utilities ✅

> Adds a unified input system, sound playback, retained-mode UI, generic object pooling,
> dialogue flow management, procedural mesh building, scene transitions, skeleton FK/skinning,
> typed deferred events, and font atlas management.
> All 10 tasks implemented March 2026.

| # | Task | Priority | Description | Output |
|---|------|----------|-------------|--------|
| ✅ 46.01 | Input System | High | Keyboard/mouse/gamepad action + axis bindings, context stack, callbacks | `Engine/Input/InputSystem/InputSystem.h/.cpp` |
| ✅ 46.02 | Sound System | High | 2D/3D sound instances, pooling, fade in/out, groups, listener transform | `Engine/Audio/SoundSystem/SoundSystem.h/.cpp` |
| ✅ 46.03 | UI System | High | Retained-mode widgets: Panel/Label/Button/Slider/Checkbox/TextInput, layout, render hooks | `Runtime/UI/UISystem/UISystem.h/.cpp` |
| ✅ 46.04 | Object Pool | High | Generic typed pre-allocated pool with RAII guard, grow-slab, thread-safe variant | `Core/Pool/ObjectPool/ObjectPool.h/.cpp` |
| ✅ 46.05 | Dialogue Manager | Medium | Conversation graph, flag-gated choices, variable substitution, auto-advance | `Runtime/Dialogue/DialogueManager/DialogueManager.h/.cpp` |
| ✅ 46.06 | Mesh Builder | Medium | Procedural mesh: box/sphere/plane/cylinder/capsule/cone, normals/tangents, flat export | `Engine/Render/MeshBuilder/MeshBuilder/MeshBuilder.h/.cpp` |
| ✅ 46.07 | Scene Transition | Medium | Fade-out/load/fade-in, replace/additive modes, scene stack, progress callbacks | `Runtime/Scene/SceneTransitionSystem/SceneTransitionSystem.h/.cpp` |
| ✅ 46.08 | Skeleton System | Medium | Joint hierarchy, bind pose, FK world matrices, skinning matrices, JSON load | `Engine/Animation/Skeleton/SkeletonSystem/SkeletonSystem.h/.cpp` |
| ✅ 46.09 | Event Queue | Medium | Typed deferred events, per-type listeners, priorities, RAII unsubscribe token | `Core/EventQueue/EventQueue/EventQueue.h/.cpp` |
| ✅ 46.10 | Font System | Low | Font face registry, glyph atlas, UTF-8, text metrics, wrap/truncate helpers | `Engine/Font/FontSystem/FontSystem.h/.cpp` |

---

## Phase 47 – Networking, Physics, AI & Platform Utilities ✅

> Adds network session management, discrete rigid body physics, behavior trees, achievement
> tracking, asset import pipeline, CPU particle emitters, world chunk streaming,
> typed config, animation blend trees, and a scripting bridge.
> All 10 tasks implemented March 2026.

| # | Task | Priority | Description | Output |
|---|------|----------|-------------|--------|
| ✅ 47.01 | Network Manager | High | Client/server sessions, reliable/unreliable channels, message handlers, stats | `Runtime/Network/NetworkManager/NetworkManager.h/.cpp` |
| ✅ 47.02 | Rigid Body System | High | AABB/Sphere bodies, force/impulse/torque, sub-step integration, collision response | `Engine/Physics/RigidBody/RigidBodySystem/RigidBodySystem.h/.cpp` |
| ✅ 47.03 | Behavior Tree | High | Sequence/Selector/Parallel composites, Inverter/Repeater decorators, Blackboard, Builder | `Engine/AI/BehaviorTree/BehaviorTree/BehaviorTree.h/.cpp` |
| ✅ 47.04 | Achievement System | Medium | Achievement registry, progress/unlock, stat bindings, persistence, platform dispatch | `Runtime/Achievement/AchievementSystem/AchievementSystem.h/.cpp` |
| ✅ 47.05 | Asset Importer | Medium | Import pipeline: type detection, hash cache, dependency graph, async queue, manifest | `Engine/Asset/AssetImporter/AssetImporter/AssetImporter.h/.cpp` |
| ✅ 47.06 | Particle System | Medium | CPU particle emitters: shapes, spawn rates, forces, colour/size curves, ground collision | `Engine/Particles/ParticleSystem/ParticleSystem/ParticleSystem.h/.cpp` |
| ✅ 47.07 | Chunk System | Medium | World chunk load/unload, LOD tiers, streaming budget, observer focus, callbacks | `Engine/World/ChunkSystem/ChunkSystem/ChunkSystem.h/.cpp` |
| ✅ 47.08 | Config System | Medium | Typed .ini/.json config, section/key store, change callbacks, CLI overrides | `Core/Config/ConfigSystem/ConfigSystem/ConfigSystem.h/.cpp` |
| ✅ 47.09 | Animation Blend Tree | Medium | 1D/2D blend spaces, additive layers, per-bone masks, parameter-driven evaluation | `Engine/Animation/BlendTree/AnimationBlendTree/AnimationBlendTree.h/.cpp` |
| ✅ 47.10 | Scripting Bridge | Low | C++↔script function registry, ScriptValue marshal, event dispatch, hot-reload, stub backend | `Engine/Scripting/ScriptingBridge/ScriptingBridge/ScriptingBridge.h/.cpp` |
