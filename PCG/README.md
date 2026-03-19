# PCG — Procedural Content Generation

Procedural generation systems for geometry, textures, audio, worlds, and quests.

## Modules

| Module | Description | Status |
|--------|-------------|--------|
| `Geometry/` | Procedural meshes, terrain, structures | Phase 7 |
| `Textures/` | Procedural textures (noise, patterns) | Phase 7 |
| `Audio/` | Procedural sound effects, ambient | Phase 7 |
| `World/` | Procedural worlds, dungeons, stations | Phase 7 |
| `Quests/` | Procedural missions, objectives, branching | Phase 7 |
| `Rules/` | PCG rule definitions (constraints, seeds) | Phase 7 |
| `Validation/` | Output validation (no overlaps, integrity) | Phase 7 |

## AI Integration

The `PCGAgent` can generate and tune PCG rules from natural language prompts. AI-assisted PCG allows:
- Text-to-geometry generation
- Rule optimization based on user feedback
- Deterministic outputs via seed management

## Dependencies

- Depends on: `Core/` (serialization), `Engine/` (renderer)
- Used by: `Editor/PCGEditor/`, `Runtime/`, `Agents/PCGAgent/`
