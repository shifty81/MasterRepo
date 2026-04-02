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
- [ ] implement chunk schema
- [ ] implement voxel storage/indexing
- [ ] implement edit API
- [ ] implement serialization
- [ ] implement voxel debug validation
- [ ] add editor voxel inspection path

## Phase 3 — First Interaction Loop Only
- [ ] add starter R.I.G. state
- [ ] add first usable mining or interaction tool
- [ ] add first resource type and pickup path
- [ ] add simple inventory baseline
- [ ] add place or repair action
- [ ] add minimal HUD/status display
- [ ] validate loop in editor and standalone client
