# Engine

Low-level engine systems. No game logic, no editor code — pure engine.

## Modules

| Module | Description | Status |
|--------|-------------|--------|
| `Render/` | Graphics backend (OpenGL → Vulkan → DX12) | Phase 2 |
| `Physics/` | Physics engine integration (Bullet) | Phase 2 |
| `Audio/` | Audio system (OpenAL) | Phase 2 |
| `Input/` | Input handling (keyboard, mouse, gamepad) | Phase 2 |
| `Window/` | Window management (SDL2/GLFW) | Phase 2 |
| `Platform/` | OS abstraction layer | Phase 2 |
| `Math/` | Math library (vectors, matrices, quaternions) | Phase 2 |

## Dependencies

- Depends on: `Core/`
- Used by: `Editor/`, `Runtime/`, `Builder/`, `PCG/`

## Build Order

This is built **after** `Core/` and **before** `Editor/` and `Runtime/`.
