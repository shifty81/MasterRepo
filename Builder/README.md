# Builder

Starbase-style module building system. Snap, weld, assemble, damage.

## Modules

| Module | Description | Status |
|--------|-------------|--------|
| `Core/` | Builder core logic | Phase 6 |
| `Parts/` | Part definitions (mesh, collision, snap points) | Phase 6 |
| `Modules/` | Module = group of parts, connection rules | Phase 6 |
| `PhysicsData/` | Physics data for built structures | Phase 6 |
| `Collision/` | Collision shapes from assembled parts | Phase 6 |
| `Damage/` | Per-part damage, destruction, repair | Phase 6 |
| `Assembly/` | Assemble/disassemble module groups | Phase 6 |
| `SnapRules/` | Snap point matching, alignment, validation | Phase 6 |

## Part Format

Each part defines:
- Mesh reference
- Collision shape
- Snap points (position, rotation, type)
- Stats (mass, health, cost)
- Connection rules

## Dependencies

- Depends on: `Core/`, `Engine/` (renderer, physics)
- Used by: `Editor/BuilderEditor/`, `Runtime/BuilderRuntime/`
