# Atlas Engine — User Manual

> **Phase 10.6 — PDF Manual / Integrated Reference**
>
> This manual provides a complete reference for the Atlas Engine game engine,
> editor, AI agent system, and associated tooling.  All chapters are plain
> Markdown so they can be read in-editor, exported to PDF, or viewed on the web.

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Quick Start](#2-quick-start)
3. [Editor Overview](#3-editor-overview)
4. [Runtime & Gameplay](#4-runtime--gameplay)
5. [AI Agent System](#5-ai-agent-system)
6. [Procedural Content Generation](#6-procedural-content-generation)
7. [Builder System](#7-builder-system)
8. [Asset Pipeline & Blender Integration](#8-asset-pipeline--blender-integration)
9. [Plugin Marketplace](#9-plugin-marketplace)
10. [VR/AR Preview](#10-vrar-preview)
11. [Scripting (Lua & Python)](#11-scripting-lua--python)
12. [Networking & Server](#12-networking--server)
13. [Performance & Profiling](#13-performance--profiling)
14. [Configuration Reference](#14-configuration-reference)
15. [Keyboard Shortcuts](#15-keyboard-shortcuts)

---

## 1. Introduction

**Atlas Engine** is an offline, AI-powered game engine and editor written in
C++20.  It is designed around a single philosophy: *everything runs offline*
— the AI, the asset pipeline, the CI system, and the documentation generator
all function without an internet connection.

### Core Principles

| Principle | Description |
|-----------|-------------|
| Offline-first | All AI inference uses local models (Ollama / llama.cpp) |
| No ImGui | Custom UI framework — the "Atlas Rule" |
| Never delete | Code is archived, never removed |
| Namespace hygiene | `Engine::`, `Core::`, `Runtime::`, `Editor::`, `AI::`, `Builder::`, `PCG::` |

### System Overview

```
┌─────────────────────────────────────────────────────┐
│  Atlas Editor  (EditorLib + UILib + IDELib)          │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐             │
│  │ Viewport │ │ AI Chat  │ │ IDE      │             │
│  └──────────┘ └──────────┘ └──────────┘             │
├─────────────────────────────────────────────────────┤
│  Engine Core  (AtlasEngine + CoreLib)                │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐             │
│  │ Renderer │ │ Physics  │ │ Audio    │             │
│  └──────────┘ └──────────┘ └──────────┘             │
├─────────────────────────────────────────────────────┤
│  AI Layer  (AILib + AgentsLib)                       │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐             │
│  │ LLM      │ │ Agents   │ │ TestGen  │             │
│  └──────────┘ └──────────┘ └──────────┘             │
└─────────────────────────────────────────────────────┘
```

---

## 2. Quick Start

### Prerequisites

#### All platforms

| Tool | Minimum Version | Notes |
|------|----------------|-------|
| CMake | 3.20 | [cmake.org](https://cmake.org/download/) |
| C++20 compiler | GCC 11 / Clang 13 / MSVC 19.30 (VS 2022) | |
| Python | 3.9+ | For AI tools and scripting |
| Blender (optional) | 3.6 LTS | Asset pipeline |
| Ollama (optional) | latest | Local AI inference |

#### Windows additional notes

**GLFW** (the windowing library used by the editor) is fetched automatically by
CMake's `FetchContent` module if it is not already installed — no manual
download is required. On first configure, CMake will clone and compile
GLFW 3.4 from source, which adds ~30 s to the initial configure step.

For faster/offline builds, pre-install GLFW via vcpkg:

```powershell
# One-time vcpkg setup
git clone https://github.com/microsoft/vcpkg C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg install glfw3:x64-windows

# Then configure with the toolchain file:
cmake --preset windows-x64-debug `
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
```

### Build

#### Linux / macOS

```bash
# Clone
git clone https://github.com/your-org/MasterRepo.git
cd MasterRepo

# Debug build (editor + runtime)
cmake -B Builds/debug -DCMAKE_BUILD_TYPE=Debug .
cmake --build Builds/debug --parallel $(nproc)

# Run editor
./Builds/debug/bin/AtlasEditor

# Run game
./Builds/debug/bin/NovaForge
```

#### Windows (Visual Studio 2022)

```powershell
# Run bootstrap first (creates directories, checks tools)
.\bootstrap.ps1

# Configure + build editor (GLFW downloaded automatically if needed)
cmake --preset windows-x64-debug
cmake --build Builds/windows-x64-debug --config Debug --target AtlasEditor

# Run editor
.\Builds\windows-x64-debug\Debug\bin\AtlasEditor.exe
```

Or use the full build script from Git Bash / MSYS2:

```bash
bash Scripts/Tools/build_all.sh
```

### One-command Setup

```bash
./bootstrap.sh          # Linux / macOS
.\bootstrap.ps1         # Windows PowerShell
```

---

## 3. Editor Overview

The Atlas Editor is the main development environment.  It is composed of
dockable panels and communicates with all engine subsystems.

### Main Panels

| Panel | Shortcut | Description |
|-------|----------|-------------|
| Viewport | `F1` | 3D scene view with camera, gizmos, overlays |
| Outliner | `F2` | Hierarchical scene entity tree |
| Inspector | `F3` | Property editor for selected entity/component |
| Content Browser | `F4` | File/asset browser with preview |
| Console | `F5` | Log output and command input |
| AI Chat | `F6` | Chat with the local AI agent |
| IDE | `F7` | Embedded code editor with LSP |
| Profiler | `F8` | CPU/GPU/memory performance panels |

### Viewport Controls

| Action | Binding |
|--------|---------|
| Rotate (Orbit) | Right-drag |
| Pan | Middle-drag |
| Zoom | Scroll wheel |
| FPS mode | Hold `RMB` + WASD |
| Freecam | `F` |
| Focus selected | `Numpad .` |
| Toggle grid | `G` |

### Gizmos

Select an entity and press:
- `W` — Move (translate)
- `E` — Rotate
- `R` — Scale
- `Q` — Back to select mode

Hold `Ctrl` to snap to grid increments.

---

## 4. Runtime & Gameplay

### Entity-Component System (ECS)

All game objects are **entities** — integer IDs with no data of their own.
Data lives in **components** (plain structs), and **systems** iterate over
matching component sets every tick.

```cpp
// Create an entity
auto e = world.CreateEntity();

// Attach components
world.Assign<TransformComponent>(e, position, rotation, scale);
world.Assign<MeshComponent>(e, meshHandle);

// Query
world.ForEach<TransformComponent, MeshComponent>([](auto& t, auto& m) {
    // process...
});
```

### Scene Loading

Scenes are stored as JSON under `Projects/NovaForge/Scenes/`.
Load from code:

```cpp
Runtime::SceneManager::LoadScene("Scenes/docking_bay.scene");
```

### Save / Load

```cpp
// Save game state
Runtime::SaveLoad::SaveGame("save_slot_0.sav");

// Load
Runtime::SaveLoad::LoadGame("save_slot_0.sav");
```

---

## 5. AI Agent System

### Local Model Setup

Install [Ollama](https://ollama.ai) and pull a model:

```bash
ollama pull llama3
ollama pull codellama
```

Or use llama.cpp with a GGUF file:

```bash
./llama-server -m models/mistral-7b.gguf --port 11434
```

### Using the AI Chat Panel

Open the **AI Chat** panel (`F6`) and type naturally:

- `"Generate a C++ inventory component"` — produces code
- `"Fix build errors in Core/Events/EventBus.cpp"` — patches code
- `"Create a JSON recipe for plasma cannon"` — generates game data

### Agent API

```cpp
#include "Agents/CodeAgent/CodeAgent.h"

Agents::CodeAgent agent;
agent.SetModelEndpoint("http://localhost:11434");
auto result = agent.Run("Create a health component");
// result.generatedFiles contains the output
```

### Sandbox Mode

All AI outputs land in `WorkspaceState/Sandbox/` before being applied.
Review and approve from the **Workspace** panel before merging.

---

## 6. Procedural Content Generation

### PCG Rule Engine

Rules are defined as `PCGRule` structs and composed in `PCGRuleEngine`:

```cpp
PCG::PCGRule rule;
rule.name        = "dungeon_room";
rule.minInstances = 4;
rule.maxInstances = 12;
rule.weight      = 1.0f;
engine.AddRule(rule);
```

### Noise-Based Terrain

```cpp
PCG::NoiseGenerator noise;
noise.SetType(NoiseType::Perlin);
noise.SetFrequency(0.02f);
float h = noise.Sample(x, z);
```

### Quest Generator

```cpp
PCG::QuestGenerator qg;
auto quest = qg.Generate("Rescue the engineer", seed);
// quest.objectives, quest.rewards, quest.dialogs
```

---

## 7. Builder System

The Builder System allows players (and the editor) to assemble structures
from modular **Parts** and **Modules**.

### Part Definition (JSON)

```json
{
  "id": "hull_plate_2x1",
  "mesh": "Parts/Meshes/hull_plate_2x1.atlasb",
  "mass": 120.0,
  "snapPoints": [
    { "id": "top",    "offset": [0.0, 0.5, 0.0], "normal": [0,  1, 0] },
    { "id": "bottom", "offset": [0.0,-0.5, 0.0], "normal": [0, -1, 0] }
  ]
}
```

### Snap Rules

```cpp
Builder::SnapRules rules;
rules.RegisterRule("hull_plate", "hull_plate",  true);   // can snap together
rules.RegisterRule("hull_plate", "structural",  true);
rules.RegisterRule("window",     "hull_plate",  true);
```

---

## 8. Asset Pipeline & Blender Integration

See [BLENDER_GUIDE.md](BLENDER_GUIDE.md) for the full Blender addon reference.

### Import an Asset from Blender

1. Export from Blender using the **Atlas Engine** addon.
2. Place the `.json` file in `Projects/NovaForge/Assets/`.
3. Import via `Tools::AssetImporter::ImportFile()`.

### Asset Processor

Compress, optimise, or resize assets after import:

```cpp
Tools::ProcessOptions opts;
opts.op = Tools::ProcessOp::OptimiseMesh;
auto result = Tools::AssetProcessor::Process("Assets/hull.atlasb", opts);
```

---

## 9. Plugin Marketplace

The **Plugin Marketplace** (`Plugins/Marketplace/`) manages installation of
plugin packs from local directories or a remote index.

### Browsing Packs

```cpp
Plugins::PluginMarketplace market;
market.RefreshCatalogue();
auto packs = market.GetPacksByCategory("AI");
for (const auto& p : packs)
    std::cout << p.displayName << " v" << p.version << "\n";
```

### Installing a Pack

```cpp
auto result = market.Install("com.atlas.llm_ollama",
    [](float p){ std::cout << int(p*100) << "%\n"; });
if (result.success)
    std::cout << "Installed to " << result.installedPath << "\n";
```

### Exporting a Pack

```cpp
std::string archive = market.Export("com.atlas.llm_ollama", "Builds/Plugins/");
// Creates Builds/Plugins/com.atlas.llm_ollama-1.0.0.atlasPack
```

---

## 10. VR/AR Preview

The editor viewport includes a **VR/AR Preview** mode backed by OpenXR.
Without an XR runtime installed, a **Simulated HMD** mode is used automatically.

### Activating VR Preview

```cpp
Editor::VRPreview vr;
vr.Init(Editor::XRMode::VR);   // falls back to Simulated if no HMD

Editor::XRFrameInfo frame;
while (vr.IsRunning()) {
    vr.PollEvents();
    if (vr.BeginFrame(frame)) {
        // render left eye using frame.leftEye
        // render right eye using frame.rightEye
        vr.EndFrame(leftTexId, rightTexId);
    }
}
```

### Simulated HMD

Useful for testing VR layouts on any desktop:

```cpp
vr.Init(Editor::XRMode::Simulated);
vr.SetSimulatedIPD(0.064f);
vr.SetSimulatedHeadPose({0, 1.7f, 0}, {0, 0, 0, 1});
```

---

## 11. Scripting (Lua & Python)

### Lua Scripts

Lua scripts live in `Scripts/Lua/`.  The engine API is exposed via
`engine_api.lua`:

```lua
local ent = Engine.CreateEntity()
Engine.AddComponent(ent, "Transform", {x=0, y=0, z=0})
Engine.AddComponent(ent, "Mesh", { mesh="cube.atlasb" })
```

### Python Scripts

Python is used for tool automation.  Run a script via:

```bash
python Scripts/Tools/build.sh debug
```

The Blender addon scripts live in `Scripts/Python/Blender/`.

---

## 12. Networking & Server

### Starting a Dedicated Server

```bash
# Edit Config/Server.json first
./AtlasServer --config Config/Server.json
```

### Config/Server.json

```json
{
  "host": "0.0.0.0",
  "port": 7777,
  "maxConnections": 64,
  "tickRate": 20.0,
  "persistentWorld": true
}
```

### Offline / Embedded Server

For single-player or LAN play the server can run in-process:

```cpp
Tools::ServerManager server;
server.LoadConfig("Config/Server.json");
server.Start();
```

---

## 13. Performance & Profiling

### ProfilerPanel

Open via `F8`.  Displays:
- **CPU** — frame time, system times, job queue depth
- **GPU** — draw calls, triangle count, texture memory
- **Memory** — heap usage, asset cache, ECS pool sizes

### Code-Side Profiling

```cpp
// Scope timer (outputs to ProfilerPanel on next frame)
ATLAS_PROFILE_SCOPE("Physics::Update");
```

### AI Analytics Dashboard

`Tools::AnalyticsDashboard` exposes metrics from AI sessions (token count,
generation latency, error rate) and PCG stats (asset count, seed distribution).

---

## 14. Configuration Reference

All configuration files live in `Config/`.

| File | Description |
|------|-------------|
| `Config/Engine.json` | Window size, renderer settings, tick rate |
| `Config/Editor.json` | Panel layout, theme, shortcuts |
| `Config/AI.json` | Model endpoint, max tokens, temperature |
| `Config/Builder.json` | Snap precision, default material |
| `Config/Server.json` | Network, world persistence, auth |
| `Config/Projects.json` | Active project path |
| `Config/marketplace.json` | Plugin catalogue cache |

---

## 15. Keyboard Shortcuts

### Global

| Shortcut | Action |
|----------|--------|
| `Ctrl+S` | Save scene |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+B` | Build project |
| `Ctrl+\`` | Toggle console |
| `F11` | Toggle fullscreen |

### Viewport

| Shortcut | Action |
|----------|--------|
| `W/E/R` | Translate / Rotate / Scale gizmo |
| `G` | Toggle grid |
| `F` | Focus selected |
| `Alt+Z` | Toggle wireframe |
| `Numpad 5` | Toggle ortho/perspective |
| `Numpad 1/3/7` | Front / Right / Top view |

### AI Chat

| Shortcut | Action |
|----------|--------|
| `Enter` | Send message |
| `Shift+Enter` | Newline in message |
| `Ctrl+L` | Clear chat history |
| `Ctrl+A` | Accept last AI suggestion |
| `Ctrl+R` | Regenerate last response |

---

*Atlas Engine User Manual — auto-generated by the DocGenerator system.*
*Feedback and corrections: open an issue on the project repository.*
