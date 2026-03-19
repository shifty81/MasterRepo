# Runtime

Gameplay and runtime systems. Used by the game in play mode and by the editor in preview mode.

## Modules

| Module | Description | Status |
|--------|-------------|--------|
| `World/` | Scene/world management | Phase 4 |
| `ECS/` | Entity-Component-System framework | Phase 4 |
| `Components/` | Game components (transform, mesh, light, etc.) | Phase 4 |
| `Systems/` | Game systems (render, physics, input) | Phase 4 |
| `Gameplay/` | Gameplay logic and prefabs | Phase 4 |
| `Player/` | Player controller, rig, TAB UI | Phase 4 |
| `Inventory/` | Inventory management | Phase 4 |
| `Equipment/` | Equipment/loadout system | Phase 4 |
| `Crafting/` | Recipes, multi-stage crafting, assembler | Phase 4 |
| `BuilderRuntime/` | Runtime builder (in-game Starbase-style) | Phase 6 |
| `Damage/` | Health, damage types, armor, death | Phase 4 |
| `Collision/` | Collision data | Phase 4 |
| `Spawn/` | Entity spawning, pooling | Phase 4 |
| `SaveLoad/` | Save/load game state | Phase 4 |
| `UI/` | HUD, menus, game UI | Phase 4 |

## Dependencies

- Depends on: `Core/`, `Engine/`
- Used by: `Editor/` (play mode), game projects
