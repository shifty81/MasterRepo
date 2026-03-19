# Editor

Unreal-style editor with fully custom UI. **Atlas Rule: No ImGui** — all UI is custom and skinnable.

## Modules

| Module | Description | Status |
|--------|-------------|--------|
| `Core/` | Editor application core | Phase 3 |
| `UI/` | Custom UI framework (widgets, rendering) | Phase 3 |
| `Docking/` | Panel docking system (dock/undock/split/tab) | Phase 3 |
| `Panels/` | Editor panels (inspector, hierarchy, etc.) | Phase 3 |
| `Viewport/` | 3D viewport with camera modes | Phase 3 |
| `Gizmos/` | Move, rotate, scale gizmos with snapping | Phase 3 |
| `Overlay/` | Debug overlays, tool overlays | Phase 3 |
| `Modes/` | Editor modes (select, place, paint, sculpt) | Phase 3 |
| `NodeEditors/` | Blueprint/node graph editors | Phase 3 |
| `BuilderEditor/` | Starbase builder integration | Phase 6 |
| `PCGEditor/` | Procedural generation editor | Phase 7 |
| `MaterialEditor/` | Material/shader node editor | Phase 9 |
| `Commands/` | Editor command bindings | Phase 3 |

## Camera Modes

- **Orbit** — Default editor camera
- **FPS** — First-person fly camera
- **Freecam** — F12 toggle, unrestricted movement
- **VR/AR** — Preview mode (Phase 10)

## Dependencies

- Depends on: `Core/`, `Engine/`
- Used by: Game projects

## Build Order

Built **after** `Core/` and `Engine/`, **before** projects.
