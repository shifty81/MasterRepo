# NovaForge Game — Reset Task List

## Phase 0 — Bootstrap
- [x] add `Config/novaforge.project.json`
- [x] verify clean configure/build from fresh checkout
- [x] verify editor executable is produced
- [x] verify game executable is produced
- [x] verify project context resolves correctly
- [x] add startup smoke-test checklist

## Phase 1 — Dev World Only
- [x] define single dev world target
- [x] lock spawn path
- [x] wire editor load path into dev world
- [x] wire runtime load path into dev world
- [x] add basic save/load hooks
- [x] add world debug overlay

## Phase 2 — Voxel Runtime Only
- [x] implement chunk schema
- [x] implement voxel storage/indexing
- [x] implement edit API
- [x] implement serialization
- [x] implement voxel debug validation
- [x] add editor voxel inspection path

## Phase 3 — First Interaction Loop Only
- [x] add starter R.I.G. state
- [x] add first usable mining or interaction tool
- [x] add first resource type and pickup path
- [x] add simple inventory baseline
- [x] add place or repair action
- [x] add minimal HUD/status display
- [x] validate loop in editor and standalone client
