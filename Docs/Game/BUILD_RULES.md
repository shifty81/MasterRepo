# NovaForge Game — Build Rules

## C++ Game Build (CMake)

The game source lives under `Source/Game/` and is built as part of the main
CMake project:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DNF_BUILD_GAME=ON
cmake --build build
./build/bin/NovaForgeGame
```

## Module Dependency Graph (`Source/Game/`)
- `NF::Game::Gameplay` — depends on NF::Core, NF::Engine only
- `NF::Game::World`    — depends on NF::Core, NF::Engine only
- `NF::Game::App`      — depends on NF::Game::World, NF::Game::Gameplay, NF::Core

## Rules
- no circular references
- no tooling modules in game source
- no engine-only dependencies inside gameplay modules
- prefer compile-safe incremental implementation
- validation runs before packaging
- packaging fails if validation fails
