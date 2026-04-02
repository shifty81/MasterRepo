# NovaForge Game — Reset Roadmap

## Repo Identity

NovaForge is a standalone repo for the game, engine, and game-specific editor.
The editor exists to build NovaForge.
The editor does not ship with the game.
The repo does not own Atlas Suite or generic tooling platform features.

## Reset Order

### Phase 0 — Bootstrap
Goal: get the repo structurally clean and bootable.

Deliverables:
- compile-safe engine core baseline
- compile-safe game bootstrap path
- compile-safe editor bootstrap path
- project manifest and path resolution
- minimal dev profile/config loading
- editor can open and render its shell state
- game can launch a null-render or stubbed runtime loop
- clean module graph for engine, editor, and game

### Phase 1 — Dev World Only
Goal: one stable sandbox world for all early testing.

Deliverables:
- single dev solar system or dev zone
- controlled spawn path
- fixed test terrain/world seed
- basic camera and movement
- editor load path for the dev world
- save/load hooks for the dev world
- basic world debug visualization

### Phase 2 — Voxel Runtime Only
Goal: establish authoritative voxel structure.

Deliverables:
- chunk data model
- chunk streaming rules
- voxel storage and indexing
- voxel edit operations
- voxel damage/mining hooks
- voxel save serialization
- editor voxel inspection and authoring hooks
- runtime validation for chunk integrity

### Phase 3 — First Interaction Loop Only
Goal: prove the game is actually playable in a narrow loop.

Deliverables:
- player spawn into dev world
- starter R.I.G. frame state
- basic mining or interaction tool
- resource pickup/storage baseline
- simple place/repair/build feedback loop
- minimal HUD for health/status/resource feedback
- smoke test from editor into play session and standalone client

## Deferred Until After Phase 3

- low-poly wrapper implementation
- fleet gameplay
- advanced factions
- advanced AI behavior
- broad galaxy simulation
- generic tooling layers
- Atlas-linked workspace features
