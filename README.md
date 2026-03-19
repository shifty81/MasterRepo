# MasterRepo

An offline, AI-powered game engine + editor + coding agent system.

## What Is This?

MasterRepo is a unified project combining:

- **Custom Game Engine** — Rendering, physics, audio, input, scripting (C++)
- **Unreal-Style Editor** — Custom UI with docking, viewports, gizmos, node editors
- **Multi-Agent AI System** — Local LLMs for code generation, asset creation, refactoring
- **Starbase-Style Builder** — Module snapping, welding, frames, assemblies
- **Procedural Content Generation** — Geometry, textures, audio, worlds, quests
- **Integrated IDE** — Syntax highlighting, AI autocomplete, AI chat panel
- **Blender Integration** — 3D asset generation pipeline
- **Fully Offline** — All AI runs locally via Ollama / llama.cpp

## Quick Start

```bash
# Bootstrap the project (installs dependencies, sets up environment)
chmod +x bootstrap.sh
./bootstrap.sh

# Build
mkdir -p Builds/Debug && cd Builds/Debug
cmake ../.. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

## Project Structure

| Directory | Purpose |
|-----------|---------|
| `Engine/` | Low-level engine (render, physics, audio, input) |
| `Core/` | Shared systems (events, reflection, serialization, tasks) |
| `Editor/` | Unreal-style editor (custom UI, docking, viewport, gizmos) |
| `Runtime/` | Gameplay systems (ECS, world, player, inventory, combat) |
| `AI/` | AI brain (models, memory, embeddings, learning pipelines) |
| `Agents/` | AI agent implementations (code, fix, build, asset, PCG) |
| `Builder/` | Starbase-style module builder (parts, snap, weld, damage) |
| `PCG/` | Procedural content generation (geometry, textures, worlds) |
| `IDE/` | Integrated development environment (code editor, AI chat) |
| `Tools/` | Standalone tools (server manager, asset tools, build tools) |
| `Projects/` | Game projects using the engine (e.g., NovaForge) |
| `Archive/` | Archived code for AI learning (never deleted) |
| `Docs/` | Documentation and guides |

## Roadmap

See **[ROADMAP.md](ROADMAP.md)** for the complete phased implementation plan with dependency graphs and work paths.

## Design Documents

The original design documents that this project is based on:

- **[implement.md](implement.md)** — Offline coding agent architecture, tool system, module design
- **[implement2.md](implement2.md)** — Engine/editor/AI/runtime architecture, master repo layout, migration plan
- **[implement3.md](implement3.md)** — Merged agent design, extended phases, v10.7 ultra blueprint

## Rules

- **Never Delete** — Old code goes to `Archive/`, never deleted. AI learns from everything.
- **Atlas Rule** — No ImGui. All editor UI is custom and skinnable.
- **Offline First** — All AI models run locally. No cloud dependency for core features.
- **Sandbox Rule** — AI never modifies live project directly. Outputs go to temp, user approves.

## License

See [LICENSE](LICENSE) for details.
