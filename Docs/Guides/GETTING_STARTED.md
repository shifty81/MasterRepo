# Getting Started with Atlas Engine

Welcome to the Atlas Engine — an offline, AI-powered game engine and editor
written in C++20. This guide walks you through setting up a build environment,
compiling the project, and running your first script.

---

## Prerequisites

| Tool | Minimum Version | Notes |
|------|----------------|-------|
| CMake | 3.20 | `cmake --version` |
| C++ compiler | C++20 support | GCC 11+, Clang 13+, or MSVC 2022 |
| Python | 3.9+ | Required by AI agent scripts |
| Git | 2.30+ | For version control integration |
| (Optional) Ollama | latest | For local LLM inference |

---

## Clone & Build

```bash
git clone https://github.com/your-org/MasterRepo.git
cd MasterRepo
```

### Debug build

```bash
cmake -B /tmp/atlas_debug -DCMAKE_BUILD_TYPE=Debug .
cmake --build /tmp/atlas_debug -- -j$(nproc)
```

### Release build

```bash
cmake -B /tmp/atlas_release -DCMAKE_BUILD_TYPE=Release .
cmake --build /tmp/atlas_release -- -j$(nproc)
```

Or use the provided helper script:

```bash
./Scripts/Tools/build.sh debug
./Scripts/Tools/build.sh release
./Scripts/Tools/build.sh all
```

---

## Running AtlasEditor

```bash
/tmp/atlas_debug/bin/AtlasEditor
```

The editor opens with a default empty scene. Use the **Scene Outliner** panel
(left dock) to manage entities and the **Inspector** panel (right dock) to
edit component properties.

---

## Running NovaForge (game)

```bash
/tmp/atlas_debug/bin/NovaForge
```

NovaForge is the reference game project that exercises all engine subsystems.
It launches directly into gameplay without the editor.

---

## First Scripting Example (Lua)

Create a file `MyLevel/patrol.lua`:

```lua
local api = require("Scripts/Lua/engine_api")

-- Spawn an entity
local id = api.Runtime.spawnEntity("Guard")

-- Attach patrol behaviour via the PatrolComponent
local PatrolComponent = require("Scripts/Lua/component_script")
local patrol = PatrolComponent.new(id, {
    {x=0,  y=0, z=0},
    {x=10, y=0, z=0},
    {x=10, y=0, z=10},
}, 3.0)

patrol:onSpawn()

-- Simulate a few frames
for i = 1, 60 do
    patrol:tick(1/60)
end

patrol:onDespawn()
```

Run it with any Lua 5.4 interpreter:

```bash
lua patrol.lua
```

---

## Where to Go Next

| Topic | Document |
|-------|----------|
| Architecture overview | `Docs/Architecture/ARCHITECTURE.md` |
| Core API reference | `Docs/API/CORE_API.md` |
| AI system | `Docs/AI/AI_SYSTEM.md` |
| Editor guide | `Docs/Editor/EDITOR_GUIDE.md` |
| Builder system | `Docs/Builder/BUILDER_GUIDE.md` |
| Roadmap | `ROADMAP.md` |
