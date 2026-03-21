# MasterRepo Implementation Roadmap

> **Synthesized from:** `implement.md`, `implement2.md`, `implement3.md`
>
> This roadmap consolidates the complete vision from all three design documents into
> a phased, dependency-ordered plan that always prioritizes **getting to something usable**
> at each milestone. Every phase produces a working, testable artifact.

---

## Table of Contents

1. [Vision Summary](#vision-summary)
2. [Architecture Overview](#architecture-overview)
3. [Master Directory Structure](#master-directory-structure)
4. [Phase 0 – Foundation & Scaffolding](#phase-0--foundation--scaffolding)
5. [Phase 1 – Core Systems](#phase-1--core-systems)
6. [Phase 2 – Engine Foundation](#phase-2--engine-foundation)
7. [Phase 3 – Editor Framework](#phase-3--editor-framework)
8. [Phase 4 – Runtime & Gameplay](#phase-4--runtime--gameplay)
9. [Phase 5 – AI Agent System](#phase-5--ai-agent-system)
10. [Phase 6 – Builder System](#phase-6--builder-system)
11. [Phase 7 – Procedural Content Generation](#phase-7--procedural-content-generation)
12. [Phase 8 – IDE & Integrated Tooling](#phase-8--ide--integrated-tooling)
13. [Phase 9 – Asset Pipeline & Blender Integration](#phase-9--asset-pipeline--blender-integration)
14. [Phase 10 – Deployment, Docs & Polish](#phase-10--deployment-docs--polish)
15. [Dependency Graph](#dependency-graph)
16. [Work Path Summary](#work-path-summary)
17. [Rules & Conventions](#rules--conventions)

---

## Vision Summary

This project is an **offline, AI-powered game engine + editor + coding agent system** that combines:

- A **custom game engine** (rendering, physics, audio, input, scripting)
- An **Unreal-style editor** with custom UI (no ImGui), docking, viewports, gizmos
- A **multi-agent AI system** running local LLMs for code generation, asset creation, and refactoring
- A **Starbase-style builder** with module snapping, welding, frames, and assemblies
- **Procedural content generation** (geometry, textures, audio, worlds, quests)
- An **integrated IDE** with syntax highlighting, AI autocomplete, and AI chat
- A **Blender addon** pipeline for 3D asset generation
- **Offline-first** — all AI models run locally via Ollama/llama.cpp

The system reads and learns from its own codebase, archives, and project files. Nothing is ever deleted — only archived for AI learning.

---

## Architecture Overview

```
User Prompt / Editor UI / CLI / API
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
   Models   Tool Runner  Code DB
      │        │
      ▼        ▼
  Files / Build / Blender / Git / Debug / Pipeline
```

**Core data flow:** `Prompt → Plan → Tools → Files → Build → Fix → Repeat`

**Engine architecture:**

```
┌──────────────────────────────────────────────────┐
│                    MasterRepo                     │
├──────────┬───────────┬───────────┬───────────────┤
│  Engine  │   Core    │  Editor   │    Runtime     │
│ (render, │ (events,  │ (UI, dock,│ (ECS, world,   │
│  physics,│  reflect, │  viewport,│  player, inv,  │
│  audio,  │  serial,  │  gizmos,  │  crafting,     │
│  input)  │  tasks)   │  nodes)   │  builder)      │
├──────────┴───────────┴───────────┴───────────────┤
│  AI / Agents  │  Builder  │  PCG  │  Tools / IDE  │
├───────────────┴───────────┴───────┴──────────────┤
│  Archive │ WorkspaceState │ Versions │ TrainingData│
└──────────┴────────────────┴──────────┴────────────┘
```

---

## Master Directory Structure

```
MasterRepo/
│
├── Engine/                  # Low-level engine (no game logic)
│   ├── Render/              #   Graphics backend (OpenGL/Vulkan/DX)
│   ├── Physics/             #   Physics engine (Bullet/custom)
│   ├── Audio/               #   Audio system
│   ├── Input/               #   Input handling (keyboard, mouse, gamepad)
│   ├── Window/              #   Window/platform management
│   ├── Platform/            #   OS abstraction layer
│   └── Math/                #   Math library (vectors, matrices, etc.)
│
├── Core/                    # Shared systems (everything depends on this)
│   ├── Reflection/          #   Type reflection system
│   ├── Metadata/            #   Metadata tagging
│   ├── Serialization/       #   Save/load/convert
│   ├── Messaging/           #   Message bus
│   ├── Events/              #   Event system
│   ├── TaskSystem/          #   Multi-threaded job system
│   ├── CommandSystem/       #   Undo/redo command pattern
│   ├── ResourceManager/     #   Asset loading and caching
│   ├── ArchiveSystem/       #   Archive management
│   ├── VersionSystem/       #   Version snapshots and frames
│   └── PluginSystem/        #   Plugin loading and management
│
├── Editor/                  # Unreal-style editor (custom UI, no ImGui)
│   ├── Core/                #   Editor application core
│   ├── UI/                  #   Custom UI framework
│   ├── Docking/             #   Panel docking system
│   ├── Panels/              #   Editor panels (inspector, hierarchy, etc.)
│   ├── Viewport/            #   3D viewport system
│   ├── Gizmos/              #   Move/rotate/scale gizmos
│   ├── Overlay/             #   Debug and tool overlays
│   ├── Modes/               #   Editor modes (select, place, paint, etc.)
│   ├── NodeEditors/         #   Blueprint/node graph editors
│   ├── BuilderEditor/       #   Starbase builder integration
│   ├── PCGEditor/           #   Procedural generation editor
│   ├── MaterialEditor/      #   Material/shader node editor
│   └── Commands/            #   Editor command bindings
│
├── Runtime/                 # Gameplay and runtime systems
│   ├── World/               #   Scene/world management
│   ├── ECS/                 #   Entity-Component-System
│   ├── Components/          #   Game components
│   ├── Systems/             #   Game systems
│   ├── Gameplay/            #   Gameplay logic
│   ├── Player/              #   Player rig and controller
│   ├── Inventory/           #   Inventory management
│   ├── Equipment/           #   Equipment/loadout system
│   ├── Crafting/            #   Crafting and recipes
│   ├── BuilderRuntime/      #   Runtime builder (Starbase-style)
│   ├── Damage/              #   Damage and health
│   ├── Collision/           #   Collision data
│   ├── Spawn/               #   Spawning system
│   ├── SaveLoad/            #   Save/load game state
│   └── UI/                  #   Runtime/game UI (HUD, menus)
│
├── AI/                      # AI brain and learning
│   ├── Models/              #   Local LLM model files
│   ├── Memory/              #   Long-term AI memory
│   ├── Embeddings/          #   Code/asset embeddings
│   ├── Training/            #   Training data processing
│   ├── Context/             #   Session context management
│   ├── Prompts/             #   Prompt templates and library
│   ├── ArchiveLearning/     #   Learning from archived code
│   ├── CodeLearning/        #   Learning from codebase
│   ├── AssetLearning/       #   Learning from assets
│   ├── ErrorLearning/       #   Learning from build errors
│   ├── ModelManager/        #   Model loading and switching
│   └── AgentScheduler/      #   Multi-agent scheduling
│
├── Agents/                  # AI agent implementations
│   ├── CodeAgent/           #   Code generation agent
│   ├── FixAgent/            #   Bug fix / compile fix agent
│   ├── BuildAgent/          #   Build automation agent
│   ├── CleanupAgent/        #   Workspace cleanup agent
│   ├── AssetAgent/          #   Asset generation agent
│   ├── PCGAgent/            #   Procedural content agent
│   ├── EditorAgent/         #   Editor automation agent
│   ├── RefactorAgent/       #   Code refactoring agent
│   ├── DebugAgent/          #   Debug assistance agent
│   └── VersionAgent/        #   Version management agent
│
├── Builder/                 # Starbase-style module builder
│   ├── Core/                #   Builder core logic
│   ├── Parts/               #   Part definitions
│   ├── Modules/             #   Module definitions
│   ├── PhysicsData/         #   Physics data for parts
│   ├── Collision/           #   Collision shapes for parts
│   ├── Damage/              #   Damage model for parts
│   ├── Assembly/            #   Assembly system
│   └── SnapRules/           #   Snap point / weld rules
│
├── PCG/                     # Procedural content generation
│   ├── Geometry/            #   Procedural geometry
│   ├── Textures/            #   Procedural textures
│   ├── Audio/               #   Procedural audio
│   ├── World/               #   World generation
│   ├── Quests/              #   Quest/mission generation
│   ├── Rules/               #   PCG rule definitions
│   └── Validation/          #   Output validation
│
├── UI/                      # Shared UI components
│   ├── Widgets/             #   Reusable UI widgets
│   ├── Layouts/             #   Layout primitives
│   └── Themes/              #   Skinning / theming
│
├── IDE/                     # Integrated development environment
│   ├── CodeEditor/          #   Code editor component
│   ├── Console/             #   Console / terminal
│   ├── AIChat/              #   AI chat panel
│   └── Debugger/            #   Integrated debugger
│
├── Tools/                   # Standalone tools and utilities
│   ├── ServerManager/       #   Game server deployment
│   ├── AssetTools/          #   Asset conversion / optimization
│   ├── BuildTools/          #   Build automation
│   └── Importer/            #   Format importers/exporters
│
├── Projects/                # Game projects using the engine
│   └── NovaForge/           #   Primary game project
│       ├── Assets/          #     Game assets
│       ├── Prefabs/         #     Prefab definitions
│       ├── Parts/           #     Game-specific parts
│       ├── Modules/         #     Game-specific modules
│       ├── Ships/           #     Ship definitions
│       ├── Recipes/         #     Crafting recipes
│       ├── Scenes/          #     Game scenes
│       ├── UI/              #     Game-specific UI
│       └── Config/          #     Game configuration
│
├── Archive/                 # NEVER DELETE — archive for AI learning
│   ├── NovaForgeOriginal/   #   Original repo snapshot
│   ├── PioneerForge/        #   PioneerForge archive
│   ├── BuilderOld/          #   Old builder systems
│   ├── EditorOld/           #   Old editor prototypes
│   ├── OldSystems/          #   Legacy code
│   ├── Broken/              #   Failed builds / broken code
│   ├── Replaced/            #   Replaced implementations
│   └── Generated/           #   AI-generated code history
│
├── WorkspaceState/          # Editor/AI workspace tracking
│   ├── Layout.json          #   Editor layout state
│   ├── OpenFiles.json       #   Currently open files
│   ├── LastBuild.json       #   Last build result
│   ├── LastErrors.json      #   Last error log
│   ├── SceneState.json      #   Current scene state
│   └── AgentState.json      #   AI agent states
│
├── Versions/                # Snapshot and frame system
│   ├── Frames/              #   Version frames
│   ├── Snapshots/           #   Full snapshots
│   ├── ObjectFrames/        #   Per-object frames
│   ├── SystemFrames/        #   System-level frames
│   └── SceneFrames/         #   Scene-level frames
│
├── TrainingData/            # AI training pipeline data
│   ├── Code/                #   Code samples for training
│   ├── Assets/              #   Asset samples
│   ├── Errors/              #   Error patterns
│   └── Diffs/               #   Code diff history
│
├── External/                # External repo references
│   ├── SwissAgent/          #   SwissAgent reference
│   ├── AtlasForge/          #   AtlasForge reference
│   └── NovaForgeOriginal/   #   Original NovaForge reference
│
├── Plugins/                 # Editor and engine plugins
│   ├── Editor/              #   Editor plugins
│   ├── Runtime/             #   Runtime plugins
│   ├── AI/                  #   AI plugins
│   └── Builder/             #   Builder plugins
│
├── Experiments/             # Sandbox / prototypes
│   ├── TestSystems/         #   Test implementations
│   ├── Prototypes/          #   Feature prototypes
│   └── Temp/                #   Temporary experiments
│
├── Builds/                  # Build outputs
│   ├── Debug/
│   ├── Release/
│   └── Editor/
│
├── Logs/                    # System logs
│   ├── Build/
│   ├── Agent/
│   ├── AI/
│   └── Server/
│
├── Config/                  # Project-wide configuration
│   ├── Engine.json
│   ├── Editor.json
│   ├── AI.json
│   ├── Builder.json
│   ├── Server.json
│   └── Projects.json
│
├── Scripts/                 # Automation scripts
│   ├── Lua/
│   ├── Python/
│   └── Tools/
│
├── Docs/                    # Documentation
│   ├── Architecture/
│   ├── API/
│   ├── Builder/
│   ├── Editor/
│   ├── AI/
│   ├── Runtime/
│   └── Guides/
│
├── CMakeLists.txt           # Root build file
├── bootstrap.sh             # Unix bootstrap script
├── bootstrap.ps1            # Windows bootstrap script
├── .gitignore
├── README.md
└── ROADMAP.md               # This file
```

---

## Phase 0 – Foundation & Scaffolding

**Goal:** Create the repo skeleton so all future work has a home. Get a "hello world" build compiling.

**Milestone:** Repository compiles, runs an empty window, directory structure in place.

| # | Task | Description | Output |
|---|------|-------------|--------|
| 0.1 | Create directory structure | All folders from the master layout above | Empty scaffold |
| 0.2 | Set up CMake build system | Root `CMakeLists.txt` with `add_subdirectory` for each module | Compilable project |
| 0.3 | Create `.gitignore` | Exclude builds, caches, models, large assets | Clean repo |
| 0.4 | Create `bootstrap.sh` / `bootstrap.ps1` | Auto-create dirs, install deps, set up venv | One-command setup |
| 0.5 | Create placeholder README files | Each major folder gets a `README.md` describing its purpose | Self-documenting |
| 0.6 | Create `Config/` defaults | Stub JSON configs for Engine, Editor, AI, Builder | Working defaults |
| 0.7 | "Hello Window" entry point | Minimal `main.cpp` that opens a window via SDL2/GLFW | First runnable |
| 0.8 | Set up Archive rule | Document "never delete" policy, create archive metadata format | Policy in place |

**Dependencies:** None (this is the root of the graph)

**Usable result:** A compiling project with an organized codebase and an empty window.

---

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

| # | Task | Description | Output |
|---|------|-------------|--------|
| 5.1 | LLM Integration | Connect to Ollama / llama.cpp / local models | `AI/ModelManager/` |
| 5.2 | Agent Core | Prompt → Plan → Tool → Result loop | `Agents/CodeAgent/` |
| 5.3 | Tool System | Tool registry, tool runner, permissions | `AI/` + `Agents/` |
| 5.4 | File System Tools | `read_file`, `write_file`, `list_dir`, `search_code` | Agent tools |
| 5.5 | Build Tools | `compile_cpp`, `run_cmake`, `run_tests` | Agent tools |
| 5.6 | Memory System | Long-term context storage, embeddings | `AI/Memory/`, `AI/Embeddings/` |
| 5.7 | Agent Scheduler | Multi-agent orchestration, task queue | `AI/AgentScheduler/` |
| 5.8 | Workspace State | Track open files, build state, errors | `WorkspaceState/` |
| 5.9 | Error Learning | Learn from build failures, auto-fix patterns | `AI/ErrorLearning/` |
| 5.10 | Code Learning | Index codebase for context-aware generation | `AI/CodeLearning/` |
| 5.11 | Prompt Library | Reusable prompt templates | `AI/Prompts/` |
| 5.12 | Sandbox System | AI outputs to temp workspace, user approves/merges | Safety system |

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

| Rule | Description |
|------|-------------|
| **Never Delete** | Only archive — `Archive/` is the graveyard. AI learns from everything. |
| **Atlas Rule** | No ImGui — custom UI only. The editor UI must be fully custom and skinnable. |
| **Offline First** | All AI models run locally. No cloud dependency for core features. |
| **Archive Metadata** | Every archived item gets: reason, date, AI-generated flag, source path. |
| **Build Order** | Core → Engine → Runtime → Builder → UI → Editor → AI → Projects |
| **Namespace Convention** | `Engine::`, `Core::`, `Editor::`, `Runtime::`, `AI::`, `Builder::`, `PCG::` |
| **Include Convention** | `#include "Core/Events/Event.h"` — always relative to MasterRepo root |
| **AI Learning Pipeline** | `Archive + Projects + Runtime + Builder + Logs → TrainingData → Embeddings → Memory → Agents` |
| **Sandbox Rule** | AI never modifies live project directly — outputs go to temp, user approves |
| **1-Hour Sessions** | AI iteration sessions capped at 1 hour, then report results |
| **Workspace State** | Editor always writes `WorkspaceState/` so AI has context |
| **Frame System** | Every significant state change creates a version frame |

---

## Getting Started

```bash
# Clone and bootstrap
git clone https://github.com/shifty81/MasterRepo.git
cd MasterRepo
chmod +x bootstrap.sh
./bootstrap.sh

# Build (after Phase 0 is complete)
mkdir -p Builds/Debug && cd Builds/Debug
cmake ../.. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

---

## Appendix A: Existing Repo Migration Plan

If you already have code in **NovaForge**, **PioneerForge**, **AtlasForge**, or **SwissAgent**, follow this step-by-step migration to bring usable code into the MasterRepo structure.

> **Source:** implement2.md Steps 0–15

### Step 0 — Assess Current Code

Inventory what exists in each repo. Categorize files as: engine, game, editor, assets, experiments, broken, or deprecated.

### Step 1 — Import as Project

Copy the existing repo into `Projects/NovaForge/` (or appropriate project folder). Do NOT restructure yet.

### Step 2 — Archive Originals

Copy original repos into both:
- `Archive/NovaForgeOriginal/` (for AI learning)
- `External/NovaForgeOriginal/` (for reference)

### Step 3 — Extract Engine Code → `Engine/`

Move pure engine systems out of the project:

| From | To |
|------|----|
| `*/renderer/`, `*/render/` | `Engine/Render/` |
| `*/input/` | `Engine/Input/` |
| `*/math/` | `Engine/Math/` |
| `*/window/` | `Engine/Window/` |
| `*/platform/` | `Engine/Platform/` |
| `*/physics/` (core) | `Engine/Physics/` |
| `*/audio/` | `Engine/Audio/` |

### Step 4 — Extract Shared Systems → `Core/`

Move shared/reusable systems:

| From | To |
|------|----|
| `*/serialization/` | `Core/Serialization/` |
| `*/events/` | `Core/Events/` |
| `*/reflection/` | `Core/Reflection/` |
| `*/resources/` | `Core/ResourceManager/` |
| `*/messaging/` | `Core/Messaging/` |
| `*/commands/` | `Core/CommandSystem/` |

### Step 5 — Extract Gameplay → `Runtime/`

Move gameplay logic:

| From | To |
|------|----|
| `*/player/` | `Runtime/Player/` |
| `*/inventory/` | `Runtime/Inventory/` |
| `*/equipment/` | `Runtime/Equipment/` |
| `*/crafting/` | `Runtime/Crafting/` |
| `*/world/` | `Runtime/World/` |
| `*/components/` | `Runtime/Components/` |
| `*/systems/` | `Runtime/Systems/` |
| `*/saveload/` | `Runtime/SaveLoad/` |

### Step 6 — Extract Builder → `Builder/`

Move builder/construction code:

| From | To |
|------|----|
| `*/builder/` | `Builder/Core/` |
| `*/damage/` | `Builder/Damage/` |
| `*/collision/` | `Builder/Collision/` |
| `*/physicsdata/` | `Builder/PhysicsData/` |
| `*/snap*/` | `Builder/SnapRules/` |

### Step 7 — Move Assets → Project

Keep game assets inside the project:
- `Projects/NovaForge/Assets/`
- `Projects/NovaForge/Prefabs/`
- `Projects/NovaForge/Ships/`
- `Projects/NovaForge/Recipes/`
- `Projects/NovaForge/Scenes/`

### Step 8 — Split UI

| UI Type | Destination |
|---------|-------------|
| Editor UI | `Editor/UI/` |
| Runtime/HUD UI | `Runtime/UI/` |
| Game-specific UI | `Projects/NovaForge/UI/` |
| Shared widgets | `UI/` |

### Step 9 — Move Editor Code → `Editor/`

Move panels, viewport, gizmos, overlays, node editors into `Editor/`.

### Step 10 — Move Experiments → `Experiments/` or `Archive/`

All test systems, prototypes, temp code goes to `Experiments/`. Broken/old code goes to `Archive/`.

### Step 11 — Fix Include Paths

Update all includes from:
```cpp
#include "player.h"
```
to:
```cpp
#include "Runtime/Player/player.h"
#include "Core/Events/Event.h"
```

### Step 12 — Build and Verify

Build order: `Core → Engine → Runtime → Builder → UI → Editor → AI → Projects/NovaForge`

Fix all compiler errors from moved files. This is the most tedious step but ensures everything works.

### Step 13 — Set Up AI Training Pipeline

After refactor, AI can read:
- `Archive/` — all old code
- `Projects/NovaForge/` — live game
- `Runtime/` + `Builder/` + `PCG/` — systems
- `Logs/` + `Versions/` — history

This feeds: `TrainingData/ → Embeddings/ → Memory/ → Agents`

### External Repo Feature Extraction

| Repo | Primary Features to Extract |
|------|-----------------------------|
| **SwissAgent** | AI agents, offline learning, automation scripts, memory systems |
| **AtlasForge** | Engine core, runtime foundations, platform abstraction |
| **Nova-Forge-Expeditions** | Gameplay modules, PCG, quests, builder extensions |
| **Atlas-NovaForge** | Networking, client/server, editor, combined systems |

---

## Appendix B: Extended Phases (Future Expansion)

These phases expand the system beyond the core 0–10 roadmap, adding advanced capabilities as the system matures.

> **Source:** implement3.md Phases 3–14

### Phase 11 — Multi-Repo Reverse Engineering & Asset Extraction

- ✅ Analyse external repos for reusable patterns (`Tools/CodeAudit/CodeAudit.h`)
- ✅ Extract and refactor usable systems into MasterRepo — `CodeAudit::GetMigrationCandidates()`
- ✅ AI-assisted code audit and merge suggestions — `CodeAudit::BuildReport()` + `onMigrationCandidate` hook
- ✅ Automated conflict detection across repos — `CodeAudit::DetectDuplicates()`

### Phase 12 — GUI Systems (Advanced)

- In-game GUI system (beyond HUD)
- ✅ In-game GUI system — element factory, screens, input routing (`UI/GUISystem/GUISystem.h`)
- ✅ GUI editor/designer tool (`Editor/GUIEditor/GUIEditor.h`)
- ✅ Animation system for UI transitions (`UI/Animation/UIAnimation.h`)
- ✅ Accessibility features (`UI/Accessibility/Accessibility.h`)
- ✅ Localization support (`UI/Localization/Localization.h`)

### Phase 13 — Blender Addon Integration (Deep)

- ✅ Full Blender addon for bidirectional asset pipeline (`Scripts/Python/Blender/atlas_blender_addon.py`) — import + export for meshes, scenes, rigs, animations, materials, batch
- ✅ Procedural geometry generation via `bpy` (`Scripts/Python/Blender/atlas_procedural_gen.py`) — hull plates, corridors, pipes, modular rooms
- ✅ Rig/animation export pipeline — armature bones + action keyframe channels
- ✅ Material conversion (Blender → Engine) (`Scripts/Python/Blender/atlas_material_converter.py`) — Principled BSDF → Atlas PBR JSON (interactive + CLI batch)
- ✅ Batch asset processing — `BatchExportAtlas` operator exports all selected objects in one step

### Phase 14 — AI-Driven Testing, Simulation & Analytics

- ✅ AI generates test cases from code (`AI/TestGen/TestGen.h`)
- ✅ Automated regression testing (`AI/RegressionTest/RegressionTest.h`)
- ✅ Simulation playback and comparison (`Tools/SimulationPlayback/SimulationPlayback.h`)
- ✅ Performance analytics dashboard (`Tools/AnalyticsDashboard/AnalyticsDashboard.h`)
- ✅ AI-assisted bug triage (`AI/BugTriage/BugTriage.h`)

### Phase 15 — Onboard Natural Language AI Assistant

- ✅ Natural language command interface in editor (`IDE/NLAssistant/NLAssistant.h`)
- ✅ Voice-to-action pipeline (`IDE/VoiceInput/VoiceInput.h`) — implemented in Phase 19
- ✅ Context-aware help system (`AI/ContextHelp/ContextHelp.h`)
- ✅ AI tutorials and walkthroughs (`AI/Tutorial/Tutorial.h`)
- ✅ Interactive documentation (`IDE/InteractiveDocs/InteractiveDocs.h`)

### Phase 16 — Core Systems Expansion

Additional core systems for production use:

| System | Description |
|--------|-------------|
| ✅ Hot Reload | Live code/asset reload without restart |
| ✅ Local CI Pipeline | Automated build/test on file save (`Core/LocalCIPipeline/`) |
| ✅ Package Manager | Dependency management for plugins (`Core/PackageManager/`) |
| ✅ Crash Report System | Crash dumps, telemetry, log analysis |
| ✅ Multi-Thread Job System | Advanced CPU task scheduling with priorities and dependencies (`Core/JobSystem/`) |
| ✅ Code Intelligence | LSP-like symbol index, go-to-definition, completion (`Core/CodeIntelligence/`) |
| ✅ Database Support | SQLite/custom for asset DB, save data |
| ✅ Network Protocol Gen | Auto-generate serialization for multiplayer (`Core/NetworkProtocolGen/`) |

### Phase 17 — Offline Self-Build Automation

The ultimate goal: AI can expand the project semi-autonomously.

1. User gives high-level goal (e.g., "Build a docking bay module")
2. AI generates code, assets, and procedural rules
3. AI validates via physics simulation and PCG rules
4. AI stores iterations in `Archive/`
5. User reviews and approves/merges best iteration
6. Repeat

✅ `Agents/SelfBuildAgent/SelfBuildAgent.h` — agent scaffolding implemented.

This is the "self-building" mode described in implement3.md, where the AI uses scaffolds, archive templates, and procedural rules to progressively build out the project offline.

---

### Phase 18 — Real GUI Window (OpenGL + GLFW)

**Goal:** Replace the console-only editor with a real graphical window.

- ✅ `Engine/Window/Window.cpp` — Real GLFW 3 backend; creates an OpenGL 2.1 context, handles resize, mouse, and keyboard input via typed callbacks
- ✅ `Editor/Render/EditorRenderer.h/.cpp` — Custom OpenGL immediate-mode renderer draws the full editor layout:
  - Title bar (app name, window controls)
  - Menu bar (File, Edit, View, Tools, Scene, AI, Build, Help) with hover highlights
  - Viewport panel — dark background, perspective grid, dummy scene objects with colour-coded outlines, XYZ axis widget, FPS overlay
  - Scene Outliner — entity list with icons, selection highlight, indent for child nodes
  - Inspector — entity header, Transform / Mesh / RigidBody / Tags component sections, Add Component button
  - Console — scrollable output log with colour-coded severity, command-input line
  - Status bar — scene name, entity count, FPS
- ✅ `External/stb/stb_easy_font.h` — embedded public-domain vector text (no texture atlas needed)
- ✅ `Editor/main.cpp` — proper GLFW render loop with delta-time; ESC to quit
- ✅ `Engine/CMakeLists.txt` — `find_package(OpenGL)` + GLFW linked into AtlasEngine
- ✅ `Editor/CMakeLists.txt` — EditorRenderer added to EditorLib

**Rules followed:** No ImGui (Atlas Rule) — UI drawn entirely with custom OpenGL primitives.

---

### Phase 19 — Networking Core, Voice Input & AI Decision Visualiser

**Goal:** Add multiplayer networking foundations, complete the voice-to-action pipeline from Phase 15, introduce a deterministic PCG seed manager, multiplayer state-sync with desync detection, and an AI reasoning visualiser for editor overlays.

- ✅ `Engine/Network/NetworkManager.h/.cpp` — Lightweight ENet-style session manager; supports host/connect/broadcast; stub-ready for ENet / LiteNetLib / asio
- ✅ `IDE/VoiceInput/VoiceInput.h/.cpp` — Voice-to-action pipeline (completes Phase 15); keyword → action mapping; stub-ready for Whisper / Vosk offline STT backends
- ✅ `AI/DecisionVisualizer/DecisionVisualizer.h/.cpp` — Records AI reasoning chains (BeginTrace / AddStep / CommitTrace); editor overlays query this to show "why the AI made this choice"
- ✅ `Core/DeterministicSeed/DeterministicSeed.h/.cpp` — Global singleton seed registry; `Derive(domain)` / `Derive(domain, index)` give every PCG subsystem reproducible seeds; serialise/deserialise for save-files
- ✅ `Runtime/StateSync/StateSync.h/.cpp` — World-snapshot ring buffer with deterministic diff-based desync detection and rollback; fires conflict callbacks for resolution strategies
- ✅ Windows build fix: `Agents/CodeAgent/ToolSystem.cpp` and `Agents/SelfBuildAgent/SelfBuildAgent.cpp` now include `<stdio.h>` inside `#ifdef _WIN32` so `_popen` / `_pclose` are properly declared on MSVC

---

### Phase 20 — AI Safety, Conflict Resolution & Performance Monitoring

**Goal:** Complete the v10.7 Ultra Blueprint remaining enhancements: transaction-safe AI suggestion staging, multi-model conflict resolution, resource monitoring, NPC behaviour controller, viewport anomaly overlays, and the SceneGraph implementation.

- ✅ `Engine/Scene/SceneGraph.cpp` — Implements the existing `SceneGraph.h`; add/remove/find/traverse/count operations on the node tree
- ✅ `AI/ConflictResolver/ConflictResolver.h/.cpp` — Arbitrates competing AI suggestions from PCG, Shader, and Gameplay pipelines; priority × pipeline-weight scoring; cyclic dependency detection and breaking
- ✅ `AI/ResourceMonitor/ResourceMonitor.h/.cpp` — Samples CPU/RAM/GPU usage during AI sessions; configurable thresholds fire alert callbacks so the orchestrator can throttle; 1-hour session limit enforced
- ✅ `Core/TransactionManager/TransactionManager.h/.cpp` — Transaction-safe AI suggestion staging; `Begin`/`Stage`/`Commit`/`Rollback` with nested savepoints; nothing touches live project until committed
- ✅ `Editor/Overlay/AnomalyOverlay.h/.cpp` — Manages `OverlayAnnotation` objects (info/warning/error) for in-viewport PCG constraint violations, AI feedback, and anomaly highlights; per-source visibility toggling
- ✅ `Runtime/NPC/NPCController.h/.cpp` — Runtime NPC behaviour controller; supports `Idle/Patrol/Guard/Aggressive/Friendly/Fleeing` archetypes; AI Chat can call `TuneParams()` mid-session for interactive behaviour adjustment

---

### Phase 21 — Engine Integration, Dialogue, Biomes & Profiling

**Goal:** Wire the long-pending `Engine/Net/NetContext` TODO into the engine core, add a biome-aware world generator, NPC dialogue system, persistent event audit log, shader asset manager, and a frame-time profiler.

- ✅ `Engine/Net/NetContext.h/.cpp` — Adapter that wires `NetworkManager` into `Engine::Core::Engine`; role-aware `Start()`/`Stop()`/`Poll()` helpers; resolves all `#if 0 TODO` stubs in `Engine/Core/Engine.h` and `Engine.cpp`
- ✅ `Engine/Shader/ShaderManager.h/.cpp` — Shader asset loader/cache; `LoadFromFiles`, `LoadFromSource`, `Reload` (hot-reload), `Bind`/`Unbind`; GPU stub-ready for OpenGL / Vulkan backends
- ✅ `Runtime/Dialogue/DialogueSystem.h/.cpp` — Dialogue-tree engine for NPC conversations; `StartDialogue` / `Advance` / `SelectChoice` / `End`; branches, conditions, and action tags; complements `NPCController`
- ✅ `PCG/Biomes/BiomeGenerator.h/.cpp` — Biome-aware world generation; classifies each cell by moisture × temperature into registered `BiomeDef` ranges; deterministic noise seeding; vegetation/rock asset spawn lists per cell
- ✅ `Core/EventLog/EventLog.h/.cpp` — Persistent rolling audit log (default 10 000 entries); severity/source/type filters; `FlushToDisk()` writes JSONL for AI training-data collection; bridges `EventBus` and AI meta-learning
- ✅ `Tools/Profiler/PerformanceProfiler.h/.cpp` — Frame-time and subsystem profiler; `BeginScope`/`EndScope` with RAII `ProfileScope` helper; rolling frame-time history; `GetAllStats()` feeds `AnalyticsDashboard`

---

### Phase 22 — Editor Undo Stack, Animation Controller, Dungeon Generator & Camera Controller

**Goal:** Unblock the long-standing `EntityCommands.h` TODO blocks by implementing the undoable command bus; add runtime per-entity animation control, a BSP dungeon generator for PCG, a smooth multi-mode camera controller, and complete the `MemoryStore` JSON persistence stub.

- ✅ `Editor/UndoableCommandBus.h/.cpp` — `IUndoableCommand` / `ICommand` interfaces + thread-safe `Execute`/`Undo`/`Redo` stack (max-depth capped, callbacks fired on history change)
- ✅ `Runtime/ECS/EntityCommands.h` — All `#if 0 TODO` blocks removed; `CreateEntityCommand`, `DestroyEntityCommand`, `SetComponentCommand<T>`, `RemoveComponentCommand<T>`, `UndoableSetComponentCommand<T>`, `UndoableRemoveComponentCommand<T>` now live code
- ✅ `Runtime/Animation/AnimationController.h/.cpp` — Per-entity animation state manager; supports multiple layers; `Once`/`Loop`/`PingPong`/`Hold` modes; fires `AnimEvent` on loop/finish; `Update(dt)` integrates with engine tick
- ✅ `PCG/Dungeon/DungeonGenerator.h/.cpp` — BSP dungeon generator; configurable depth / room sizes / split ratio; deterministic LCG seed; L-shaped corridor connection; tags start/boss rooms; streaming `GenerateWithCallback()`
- ✅ `Engine/Camera/CameraController.h/.cpp` — Three-mode camera driver: `Orbit` (editor/third-person), `Fly` (free-flight), `Follow` (smooth-lag entity tracking); per-mode smooth damping; `TargetQuery` callback for Follow mode
- ✅ `AI/Memory/MemoryStore::LoadFromFile()` — Completed hand-rolled JSON deserialization matching `SaveToFile()` output; round-trip save/load now fully functional

---

### Phase 23 — Game Session, LOD System, Config System, Goal Planner, Story Generator & Benchmark Runner

**Goal:** Cover the remaining Blueprint gaps: multiplayer session bookkeeping, distance-driven LOD for the render pipeline, hierarchical project configuration, GOAP AI decision-making for NPCs/agents, procedural narrative generation, and a benchmark framework for CI performance regression.

- ✅ `Runtime/Gameplay/GameSession/GameSession.h/.cpp` — Session state machine (`Lobby → Active → Paused → Ended`); `AddPlayer`/`RemovePlayer`; per-player score/kills/deaths/ping/play-time; time-limit and score-limit end conditions; `Tick(dt)` advances clock; `Leaderboard()` sorted view; session event callbacks
- ✅ `Engine/Lod/LODSystem.h/.cpp` — Distance-driven LOD manager; per-object `LODBand` array; hysteresis margin to prevent band ping-pong; configurable hard-cull distance; `Evaluate()` returns sorted `LODResult` list; `OnLODChanged` callbacks
- ✅ `Core/Config/ConfigSystem.h/.cpp` — Multi-layer hierarchical config (defaults → base → platform → user); INI and flat-JSON parsers; typed `Get*`/`Set*` API; file-system `PollReload()` live-reload via `std::filesystem::last_write_time`; `SaveLayer()` export; `OnChanged` callbacks
- ✅ `AI/GoalPlanner/GoalPlanner.h/.cpp` — GOAP goal-oriented action planner; forward-chaining BFS search with cycle avoidance (string-hash visited set); `WorldState` as flat `unordered_map`; `PlanFor(goal)` and `PlanBest()` (highest-priority achievable); `CanExecute`/`ApplyAction` helpers
- ✅ `PCG/Story/StoryGenerator.h/.cpp` — Procedural narrative arc generator; 7-beat story template (`Intro → Climax → Resolution`); deterministic LCG seed; quest-weaving from `WorldContext`; `CompleteBeat()`/`NextBeat()` for runtime progression; optional side-quest beat
- ✅ `Tools/BenchmarkRunner/BenchmarkRunner.h/.cpp` — Micro-benchmark runner; warmup + timed iteration loop; per-run stats (min/max/mean/median/stddev/total); CSV/JSON/Markdown export; `RunFilter()` by name substring; `Compare()` diff table for regression CI

---

### Phase 24 — Behavior Tree, Particle System, Lua Binding, Vegetation Generator, Asset Pipeline & Project Explorer

**Goal:** Fill the remaining high-value Blueprint gaps: NPC BT-based decision-making, visual effects particles, script engine binding, biome-driven foliage, unified asset pipeline orchestration, and the IDE project tree.

- ✅ `Runtime/AI/BehaviorTree/BehaviorTree.h/.cpp` — Complete BT system: `ActionNode`, `ConditionNode`, `SequenceNode` (AND), `SelectorNode` (OR), `ParallelNode` (RequireAll/RequireOne), `InvertNode`, `RepeatNode` (N times or infinite), `LimitNode` (max runs); `Blackboard` typed key-value context; `BehaviorTree` owner drives tick; auto-reset on completion
- ✅ `Engine/Particles/ParticleSystem.h/.cpp` — CPU particle system; `EmitterConfig` with cone spread, speed range, gravity, drag, lifetime, color gradient, size-over-life; `ParticleEmitter` pool-based allocation; `ParticleSystem` multi-emitter manager; `CollectDrawData()` feeds renderer; render callback decouples from GPU
- ✅ `Core/Scripting/LuaBinding.h/.cpp` — Dependency-free script binding registry; `ScriptValue` variant (nil/bool/int64/double/string); `ScriptModule` namespaced function map; `LuaBinding` global+namespaced dispatch; `Call("ns.fn", args)`, `CanCall()`, `AllFunctions()`, `Dump()` introspection; designed for drop-in Lua/wasm backend
- ✅ `PCG/Vegetation/VegetationGenerator.h/.cpp` — Biome-aware foliage placement; per-biome `BiomeVegetationConfig` with asset weight table, density, maxSlope; 2D value noise for cluster modulation; `VegetationInstance` with world pos/rotY/scale; streaming `GenerateWithCallback()`; `GlobalDensityScale` tuning knob
- ✅ `Tools/AssetPipeline/AssetPipeline.h/.cpp` — Unified import→process→export pipeline; wraps static `AssetImporter::Import` + `AssetProcessor::Process`; job queue with `Enqueue`/`Cancel`/`RunAll`/`RunNext`; `PipelineProgressFn` + `PipelineCompleteFn` callbacks; `History()` of all results; `Process()` one-shot convenience
- ✅ `IDE/ProjectExplorer/ProjectExplorer.h/.cpp` — Virtual project file tree; `Refresh()` recursive directory scan; `ExplorerFilter` (extensions/showHidden/nameContains); expand/collapse/ExpandAll; selection with `OnSelectionChanged`; `PollChanges()` file-watch via `last_write_time`; `GetExpandedPaths()`/`RestoreExpandedPaths()` state persistence

---

### Phase 25 — Achievement System, Post-Process Pipeline, Weather System, Memory Profiler, Snippet Manager & Event Dispatcher

**Goal:** Fill remaining gameplay, rendering, world-simulation, developer-tooling, and IDE gaps with polished, fully-wired subsystems.

- ✅ `Runtime/Gameplay/AchievementSystem/AchievementSystem.h/.cpp` — Achievement definitions with four condition types (Cumulative, SingleEvent, Streak, TimedAccumulation); `Update(eventKey, delta)` drives all matching achievements in one call; `ForceUnlock()`, `ResetStreak()`; `ExportState()`/`ImportState()` for save/load; `OnUnlock`/`OnProgress` callbacks; rewardXP + rewardItemId per def
- ✅ `Engine/PostProcess/PostProcessPipeline.h/.cpp` — Seven configurable post-process passes: Bloom (threshold/blur), Tonemapping (ACES/Reinhard/Uncharted2), Vignette, SSAO (hemisphere kernel), Chromatic Aberration, ColorGrading (lift/gamma/gain/saturation/contrast), FXAA; `Execute(fbo, w, h)` runs enabled passes in correct order; `PassResult` timing per pass; `EnablePass()` per-name override
- ✅ `PCG/Weather/WeatherSystem.h/.cpp` — Seven built-in presets (Clear/Cloudy/Rain/Storm/Fog/Snow/Blizzard); full `WeatherState` (cloud/rain/snow/fog/wind/temp/humidity/lightning/visibility/fog-colour); smooth `Lerp()` blending between presets; auto-transition with weighted random selection after hold duration; time-of-day temperature modulation; `OnWeatherChange`/`OnTick` callbacks
- ✅ `Tools/MemoryProfiler/MemoryProfiler.h/.cpp` — Explicit `Track(tag,bytes,callsite)`/`Untrack(id)` instrumentation; per-tag `TagStats` (live/count/peak/totalAlloc/totalFreed); RAII `ScopeTracker`; `Snapshot()` capture; `DiffSnapshots()` for leak detection; `Report()` aligned text + `ReportCSV()`; `ResetPeaks()` for per-frame high-water tracking
- ✅ `IDE/SnippetManager/SnippetManager.h/.cpp` — Snippet library with `${VAR}` placeholder substitution; `ExpandByPrefix()` for trigger-based expansion (e.g. "forr<tab>"); search by tag/language/substring; `LoadFromFile()`/`SaveToFile()` in pipe-delimited format; seven built-in C++/Lua/Python snippets; `useCount` tracking for MRU sorting
- ✅ `Core/EventDispatcher/EventDispatcher.h/.cpp` — Typed `Subscribe<T>(event, fn, priority, oneShot)`; `Dispatch<T>` synchronous sorted-by-priority dispatch; `Defer<T>` + `FlushDeferred()` for deferred queue; `Unsubscribe(ListenerID)`/`UnsubscribeAll(event)`; thread-safe ID generation with `std::atomic`; copy-before-dispatch avoids iterator invalidation from recursive unsubscribe

---

## Appendix C: v10.7 Ultra Blueprint Systems Map

The final comprehensive systems map from implement3.md showing all subsystems and their relationships:

```
MasterRepo (v10.7 Ultra Blueprint)
│
├── Atlas (Engine Layer)
│   ├── engine_core (physics, GPU compute, serialization)
│   ├── editor (Custom GUI, Code/Script, NodeGraph, Shader, PCG Rule, AI Chat, Metrics)
│   ├── plugins (procedural mesh, AI tools, asset importers, VR/AR)
│   ├── rendering_pipeline — ✅ `Engine/Render/RenderPipeline.h` (LOD, GI, RayTracing, Volumetrics, Particles)
│   ├── pcg_pipeline — ✅ `PCG/Pipeline/PCGPipeline.h` (Starbase generators, constraint solver, sandbox sim)
│   ├── audio_pipeline — ✅ `Engine/Audio/AudioPipeline.h` (procedural audio, SuperCollider, FluidSynth)
│   └── animation_pipeline — ✅ `Engine/Animation/AnimationPipeline.h` (rigging, mocap, procedural animation)
│
├── NovaForge (Game Layer)
│   ├── gameplay (player, ships, crafting, NPC AI, environment, networking)
│   ├── pcg_assets (noise generators, dungeon maps, Blender addon)
│   └── pcg_validation (OpenCV, anomaly detection, AI-guided fixes)
│
├── AI (Intelligence Layer)
│   ├── models (LLaMA, MPT, ONNX, DeepSpeed, GPT4All)
│   ├── orchestrator — ✅ `AI/Orchestrator/Orchestrator.h` (pipeline scheduler, priority/dependency management)
│   ├── validator — ✅ `AI/Validator/Validator.h` (automated debugging, pull-request reports)
│   ├── rules_engine — ✅ `AI/RulesEngine/RulesEngine.h` (constraint checking)
│   ├── metrics_dashboard — ✅ `AI/MetricsDashboard/MetricsDashboard.h` (PCG, rendering, gameplay metrics)
│   ├── rendering_optimizer — ✅ `AI/RenderingOptimizer/RenderingOptimizer.h` (visual optimization suggestions)
│   └── meta_learning_layer — ✅ `AI/MetaLearning/MetaLearning.h` (cross-project knowledge transfer)
│
├── Server (optional multiplayer)
│   ├── engine_runtime, networking, pcg_pipeline, ai_integration
│   └── admin_console — ✅ `Tools/AdminConsole/AdminConsole.h` (metrics, validation, Prometheus endpoint)
│
├── ClientOffline (standalone offline mode)
│   ├── engine_runtime (embedded headless server)
│   ├── pcg_pipeline, ai_integration, hud_interface
│
└── dev_tools
    ├── code_scaffolding_pipeline — ✅ `Tools/Scaffolding/CodeScaffolder.h`
    ├── automated_testing_pipeline — ✅ `Tools/TestPipeline/TestPipeline.h`
    ├── build_deployment_pipeline (CI scripts in Scripts/Tools/)
    └── metrics_reporting_pipeline — ✅ `Tools/MetricsReporting/MetricsReporter.h`
```

---

*This roadmap is a living document. Update it as phases are completed and new requirements emerge.*
