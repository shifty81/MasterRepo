# Atlas Engine — Architecture

> This document describes the overall architecture of MasterRepo: an offline,
> AI-powered game engine + editor + coding-agent system written in C++20.

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [Build Order & Dependency Graph](#2-build-order--dependency-graph)
3. [Namespace Conventions](#3-namespace-conventions)
4. [Include Path Conventions](#4-include-path-conventions)
5. [Key Design Principles](#5-key-design-principles)
6. [Module Descriptions](#6-module-descriptions)
7. [Extension Points](#7-extension-points)

---

## 1. System Overview

```
User Prompt / Editor UI / CLI
              │
              ▼
        ┌───────────┐
        │ AGENT CORE │
        └─────┬─────┘
    ┌─────────┼─────────┐
    │         │         │
    ▼         ▼         ▼
 ┌─────┐ ┌────────┐ ┌─────────┐
 │ LLM │ │ TOOLS  │ │ INDEX   │
 └──┬──┘ └───┬────┘ └────┬────┘
    │        │           │
    ▼        ▼           ▼
 Models  Tool Runner  Code DB
    │        │
    ▼        ▼
Files / Build / Blender / Git / Debug
```

**Engine layer stack:**

```
┌─────────────────────────────────────────────┐
│              Projects (NovaForge …)          │
├──────────────────┬──────────────────────────┤
│  Editor (Atlas)  │  IDE (integrated)         │
├──────────────────┴──────────────────────────┤
│                  UI Layer                    │
├──────────┬───────────────┬──────────────────┤
│  Runtime │    Builder    │       PCG         │
├──────────┴───────────────┴──────────────────┤
│                  Engine                      │
├─────────────────────────────────────────────┤
│   AI / Agents           Tools               │
├─────────────────────────────────────────────┤
│                   Core                       │
└─────────────────────────────────────────────┘
```

**Core data flow:** `Prompt → Plan → Tools → Files → Build → Fix → Repeat`

---

## 2. Build Order & Dependency Graph

The CMake subdirectory order mirrors the dependency graph: each module may
only include headers from modules listed above it.

```
Core          (no engine deps — EventBus, MessageBus, TaskSystem,
               CommandSystem, ResourceManager, Serializer, PluginSystem,
               VersionSystem, Reflection, Metadata)
   │
Engine        (depends on Core — Renderer, Physics, Audio, Input,
               Scripting, Animation, Networking)
   │
UI            (depends on Core — Widget system, Themes, Layouts)
   │
Runtime       (depends on Core + Engine — ECS, Systems, EntityCommands,
               DeltaEditStore)
   │
Builder       (depends on Core + Runtime — Parts, SnapPoints, Assembly,
               Damage, Physics, Collision)
   │
PCG           (depends on Core + Engine — TerrainGen, MeshGen, TextureGen,
               AudioGen, WorldGen, QuestGen)
   │
AI            (depends on Core — Agents, Models, Tools, Memory, Sandbox)
Agents        (depends on Core + AI — CodeAgent, BuildAgent, FixAgent …)
   │
Editor        (depends on Core + Engine + UI + Runtime + Builder + PCG
               — Docking, Panels, Gizmos, Viewport, NodeEditors …)
   │
IDE           (depends on Core + Editor — CodeEditor, Debugger, Git …)
Tools         (depends on Core — standalone CLI tools)
   │
Projects      (depends on everything — NovaForge game executable)
```

---

## 3. Namespace Conventions

| Namespace         | Location                    | Purpose                                |
|-------------------|-----------------------------|----------------------------------------|
| `Core::`          | `Core/`                     | Foundational systems (events, tasks …) |
| `Core::PluginSystem::` | `Core/PluginSystem/`  | Plugin registry and IPlugin interface  |
| `Engine::`        | `Engine/`                   | Renderer, physics, audio, input        |
| `Runtime::`       | `Runtime/`                  | Gameplay loop                          |
| `Runtime::ECS::`  | `Runtime/ECS/`              | Entity-component system                |
| `Builder::`       | `Builder/`                  | Module builder                         |
| `PCG::`           | `PCG/`                      | Procedural generation                  |
| `AI::`            | `AI/`                       | AI models and inference                |
| `Editor::`        | `Editor/`                   | Editor panels, gizmos, docking         |
| `IDE::`           | `IDE/`                      | Code editor, debugger                  |
| `UI::`            | `UI/`                       | Widget system, themes, layouts         |
| `Plugins::`       | `Plugins/`                  | Plugin interface types                 |

---

## 4. Include Path Conventions

All `#include` directives use paths **relative to the repository root**, e.g.:

```cpp
#include "Core/Events/EventBus.h"
#include "UI/Widgets/Widget.h"
#include "Editor/Docking/DockingSystem.h"
```

`CMakeLists.txt` adds `${CMAKE_SOURCE_DIR}` as an include directory for every
target, so no install prefix is required during development.

---

## 5. Key Design Principles

### Offline-first AI
All language model inference runs locally via Ollama or llama.cpp. No data
leaves the machine. The AI system is designed to work completely offline,
reading the local codebase, archives, and training data.

### Atlas Rule (no ImGui)
The editor and all tools use a **custom UI framework** (`UI/`) — ImGui is
explicitly banned. This ensures full control over styling, input handling,
and accessibility without third-party widget limitations.

### Never-delete policy
Code and data are never deleted — only **archived**. The `Archive/` directory
and `Core::ArchiveSystem` preserve every removed file. The `VersionSystem`
stores snapshots. AI agents learn from the archive history.

### Command-based mutability
All state mutations that should be undoable go through `Core::CommandSystem`
(undo/redo stack). Direct mutation of editor state is discouraged; prefer
command objects derived from `Core::ICommand`.

### Plugin extensibility
Every major subsystem exposes a plugin interface in `Plugins/` and is
registered via `Core::PluginSystem::PluginSystem`. Third-party and
first-party extensions follow the same `IPlugin` contract.

---

## 6. Module Descriptions

### Core
Foundational, dependency-free systems used by every other module. Provides:
`EventBus` (typed pub/sub), `MessageBus` (decoupled messaging), `TaskSystem`
(thread pool), `CommandSystem` (undo/redo), `ResourceManager` (asset cache),
`Serializer` (JSON), `PluginSystem` (plugin registry), `VersionSystem`
(snapshot versioning), and `Reflection` (runtime type info).

### Engine
The rendering, physics, audio, input, scripting and animation runtime. Sits
directly on top of Core and provides platform abstractions. Does not depend on
Editor or Runtime.

### UI
Custom widget framework (`Widget`, `Button`, `Label`, `TextInput`, etc.),
theming (`ThemeManager`, `Theme`), and layout engines (`StackLayout`,
`GridLayout`, `AnchorLayout`). Used exclusively by Editor and IDE; no ImGui.

### Runtime
ECS-based gameplay loop. `World` owns entities and components. `System` base
class supports priority-ordered per-frame ticks. `DeltaEditStore` records
incremental state changes for replay and AI training.

### Editor
Unreal-style editor application. Features docking (`DockingSystem`), panels
(SceneOutliner, Inspector, ContentBrowser, Console, Profiler, Viewport),
gizmos, node editors, material editor, builder editor, and PCG editor. All
panels use the `UI::` widget system.

### AI
Offline LLM integration and multi-agent orchestration. Agents (CodeAgent,
BuildAgent, FixAgent, RefactorAgent, etc.) use a tool system to read/write
files, run builds, query the codebase index, and call Blender. Memory and
context are persisted to `TrainingData/`.

### Builder
Starbase-style modular construction. Parts are defined by `PartDef` with snap
points; `Assembly` manages the graph of welded instances. Runtime building
maps directly to ECS entities.

### PCG
Procedural content generation pipelines for terrain, meshes, textures, audio,
worlds, and quests. Grammar-based mesh generation and noise-based terrain are
the primary workhorses.

### IDE
Integrated development environment built on `CodeEditor` (syntax highlighting,
breakpoints, completion). Includes a Lua debugger, Git integration panel, and
AI chat sidebar.

### Tools
Standalone CLI tools for asset cooking, format conversion, and build
automation. Depend only on Core.

---

## 7. Extension Points

### Plugin System
Implement `Core::PluginSystem::IPlugin` (or a typed sub-interface in
`Plugins/`) and register with `Core::PluginSystem::PluginSystem`. Plugin
interfaces: `IAIPlugin`, `IEditorPlugin`, `IBuilderPlugin`, `IRuntimePlugin`.

### Custom Agents
Add a new agent class under `Agents/` inheriting the base agent. Register
tools in the agent constructor. The agent automatically participates in the
planning loop.

### Lua Scripting
Component scripts loaded at runtime inherit from the API surface exposed in
`Scripts/Lua/engine_api.lua`. New API tables can be added by binding C++
functions via sol2 and documenting them in `engine_api.lua`.

### Custom Widgets
Derive from `UI::Widget`, override `GetType()` and `OnEvent()`, and register
with the theme system via `ThemeManager::ApplyTo()`. No ImGui involvement.
