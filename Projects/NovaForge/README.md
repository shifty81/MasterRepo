# NovaForge

Primary game project — space exploration, construction, and combat.

This is the main game project built on the MasterRepo engine. It contains only game-specific content:

## Structure

| Directory | Contents |
|-----------|----------|
| `Assets/` | Game assets (models, textures, sounds) |
| `Prefabs/` | Prefab definitions |
| `Parts/` | Game-specific builder parts |
| `Modules/` | Game-specific builder modules |
| `Ships/` | Ship definitions and templates |
| `Recipes/` | Crafting recipes |
| `Scenes/` | Game scenes / levels |
| `UI/` | Game-specific UI layouts |
| `Config/` | Game configuration |

## Rules

- Game code uses `Runtime/` systems — no engine code here
- Assets stay in the project — not in `Engine/`
- Game-specific UI stays here — shared UI goes to `UI/`
- Builder parts/modules defined here, builder logic in `Builder/`
