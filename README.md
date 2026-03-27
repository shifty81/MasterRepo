# MasterRepo

> **Offline, AI-powered game engine + editor + game project.**
> Everything runs locally. No cloud. No subscriptions.

---

## ✅ What Is Done Right Now

### 🎮 AtlasEditor (the editor)
- **3D orbit viewport** — RMB orbit, MMB pan, scroll zoom, F to reset camera
- **ECS scene outliner** — live entity list from the runtime ECS world
- **Inspector** — shows Transform, MeshRenderer, RigidBody, Tags for selected entity
- **Gizmo manipulation** — W/E/R to switch Move/Rotate/Scale; drag X/Y/Z axes
- **Grid snap** — G key toggles snapping, configurable grid size
- **PIE (Play In Editor)** — P to start, ESC to stop; planets visibly orbit in simulation; green viewport border while playing
- **Full undo/redo** — Ctrl+Z / Ctrl+Y; Create, Delete, Move entity all undoable
- **Copy / Paste / Duplicate** — Ctrl+C, Ctrl+V, Ctrl+D
- **Delete** — Delete key (undoable)
- **Scene save/load** — Ctrl+S / Ctrl+O; saves to `Projects/NovaForge/Scenes/editor_save.scene`; tags round-tripped correctly
- **New Entity / Duplicate Entity** — via toolbar or Scene menu (undoable)
- **Add Component popup** — Transform, MeshRenderer, RigidBody, Tag
- **Console panel** — live log output, command input
- **Content browser** — file tree panel
- **Code editor panel** — syntax-highlighted editor panel
- **F1 keybinds panel** — full shortcut reference sheet overlay
- **AI Chat panel** — Ctrl+Space; real async Ollama calls (codellama @ localhost:11434)
- **ProjectAI panel** — ChatGPT-style floating panel shown at startup; project-context-aware
- **Build button** — F5 / Ctrl+B triggers project build
- **File menu** — Save Scene, Open Scene, Exit (via menu only — ESC never closes the editor)

### 🚀 NovaForge (the game)
- **3D FPS camera** — WASD movement, mouse look, crosshair
- **Star system** — Sol + all planets + asteroid belts + stations rendered as 3D spheres/boxes
- **Approach prompts** — land/orbit UI when near celestial bodies
- **AI miners** — autonomous ships that fly to deposits and earn credits
- **Builder mode** — F2 to toggle ship/station construction
- **HUD** — FPS counter, entity count, miner count, earnings, status bar
- **Quick save/load** — F5 / F9
- **Editor round-trip** — loads `editor_save.scene` at startup if present, so editor changes appear in-game automatically

### 🤖 Atlas AI Server (`AI/Server/`)
- **Standalone local web server** — FastAPI on `http://localhost:8080`
- **ChatGPT-style web UI** — dark theme, markdown rendering, code blocks
- **Project-aware** — deep-indexes the entire repo (C++ symbols, file tree, docs) on startup
- **File browser sidebar** — browse and view any project file from the browser
- **`/read path`** — type `/read Engine/Core/Engine.h` to view a file in chat
- **Chat history** — persists across requests, clearable
- **Re-index button** — force a full re-scan
- Refactored from [Arbiter](https://github.com/shifty81/Arbiter)'s FastAPI + archive manager

### 🏗️ Engine & Systems (all compile clean)
All 16 CMake targets build with zero errors:
`VersionLib` · `TrainingDataLib` · `UILib` · `AtlasEngine` · `CoreLib` · `BuilderLib` · `AILib` · `RuntimeLib` · `PCGLib` · `IDELib` · `AgentsLib` · `ToolsLib` · `EditorLib` · `NovaForge` · `ToolsGUI` · `AtlasEditor`

Full subsystem list (261 tasks completed): see **[ROADMAP.md](ROADMAP.md)**

---

## 🚀 Quick Start

### Build the editor + game
```bash
mkdir -p Builds/Debug && cd Builds/Debug
cmake ../.. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --target AtlasEditor NovaForge
```

### Run the editor
```
Builds/Debug/bin/AtlasEditor.exe
```

### Run the game
```
Builds/Debug/bin/NovaForge.exe
```

### Start the AI server (local web UI)
```bash
cd AI/Server
./start.sh          # Linux/macOS/Git Bash
start.bat           # Windows

# Then open: http://localhost:8080
# Requires: ollama serve && ollama pull codellama
```

---

## 🎮 Editor Keybinds

| Key | Action |
|-----|--------|
| **W / E / R** | Move / Rotate / Scale gizmo |
| **G** | Toggle grid snap |
| **P** | Start PIE simulation |
| **ESC** (in PIE) | Stop PIE |
| **F** | Reset camera to scene |
| **F1** | Keybinds reference panel |
| **Ctrl+Z / Y** | Undo / Redo |
| **Ctrl+C / V / D** | Copy / Paste / Duplicate entity |
| **Delete** | Delete selected entity (undoable) |
| **Ctrl+S / O** | Save / Open scene |
| **Ctrl+B / F5** | Build project |
| **Ctrl+Space** | AI Chat panel |
| **RMB drag** | Orbit camera |
| **MMB drag** | Pan camera |
| **Scroll** | Zoom |
| **LMB click** | Select entity |

---

## 📁 Project Structure

| Directory | Purpose |
|-----------|---------|
| `Engine/` | Low-level engine (render, physics, audio, input) |
| `Core/` | Shared systems (events, reflection, serialization, ECS) |
| `Editor/` | Custom editor — viewport, gizmos, outliner, inspector |
| `Runtime/` | Gameplay — ECS world, player, inventory, combat |
| `AI/` | AI systems — OllamaClient, agents, multi-agent coordinator |
| `AI/Server/` | **Standalone AI web server** (FastAPI, port 8080) |
| `Agents/` | AI agent implementations (code, fix, build, asset, PCG) |
| `Builder/` | Starbase-style module builder (parts, snap, weld) |
| `PCG/` | Procedural content generation (geometry, worlds, quests) |
| `IDE/` | Integrated IDE (code editor, AI chat panel) |
| `Projects/NovaForge/` | The game — 3D space / FPS built on the engine |
| `Archive/` | Archived code for AI learning (never deleted) |
| `Docs/` | Documentation and guides |

---

## 🗺️ Planned / In Progress

| Area | Status |
|------|--------|
| Scene editing → game round-trip | ✅ Done (editor_save.scene) |
| Undo/redo for all operations | ✅ Done |
| PIE simulation | ✅ Done (orbital animation) |
| Standalone AI web UI | ✅ Done (AI/Server) |
| Multiplayer sync | 🔜 Planned (Phase 37) |
| Economy / faction system | 🔜 Planned |
| Ship-to-ship combat | 🔜 Planned |
| Player progression | 🔜 Planned |
| Modding / Lua hooks | 🔜 Planned |
| Full UI theme system | 🔜 Planned |
| Integration test suite | 🔜 Planned |

---

## Rules

- **Never Delete** — Old code goes to `Archive/`. AI learns from everything.
- **Atlas Rule** — No ImGui. All editor UI is custom and fully skinnable.
- **Offline First** — All AI models run locally. No cloud dependency.
- **Sandbox Rule** — AI never modifies live project directly.

## License

See [LICENSE](LICENSE) for details.

