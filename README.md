# NovaForge Engine

A modern, Unreal-inspired game engine built in C++20 with a custom editor, rendering pipeline, and gameplay framework.

## Architecture

```
Source/
├── Core/           # Foundation: math, memory, logging, threading, events, reflection, serialization
├── Engine/         # Runtime: ECS, world, components, game framework, assets, game AI
├── Renderer/       # Rendering: RHI (OpenGL), forward pipeline, materials, lighting, post-process
├── Physics/        # Physics: collision detection, rigid bodies, character controller
├── Audio/          # Audio: device, spatial audio, mixer
├── Animation/      # Animation: skeleton, blend tree, state machine, IK
├── Input/          # Input: keyboard, mouse, gamepad, action mappings
├── Networking/     # Networking: sockets, replication, sessions, HTTP
├── UI/             # UI framework: custom widget system (no ImGui)
├── Editor/         # Editor: viewport, gizmos, outliner, inspector, content browser
└── Programs/       # Executables: NovaForgeEditor, NovaForgeGame
```

## Build Requirements

- **CMake** 3.20+
- **C++20** compiler (GCC 11+, Clang 14+, MSVC 2022+)
- **OpenGL** (system)
- **GLFW** (auto-downloaded via FetchContent if not found)

## Quick Start

```bash
# Configure
cmake -B Builds/Debug -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build Builds/Debug --parallel

# Run Editor
./Builds/Debug/bin/NovaForgeEditor

# Run Game
./Builds/Debug/bin/NovaForgeGame
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `NF_BUILD_EDITOR` | ON | Build the NovaForge Editor |
| `NF_BUILD_GAME` | ON | Build the NovaForge Game runtime |
| `NF_BUILD_TESTS` | OFF | Build the test suite |

## CMake Presets

```bash
cmake --preset debug        # Debug build
cmake --preset release      # Release build
cmake --build --preset debug
```

## Module Dependencies

```
NF::Core ──────────────────────────────────────────────────┐
NF::Engine ──── NF::Core                                   │
NF::Renderer ── NF::Core, NF::Engine, OpenGL, GLFW         │
NF::Physics ─── NF::Core, NF::Engine                       │
NF::Audio ───── NF::Core                                   │
NF::Animation ─ NF::Core, NF::Engine                       │
NF::Input ───── NF::Core                                   │
NF::Networking─ NF::Core                                   │
NF::UI ──────── NF::Core, NF::Renderer                     │
NF::Editor ──── NF::Core, NF::Engine, NF::Renderer, ...    │
                                                           │
NovaForgeEditor ── all modules ────────────────────────────┘
NovaForgeGame ──── all modules (except Editor)
```

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE).
