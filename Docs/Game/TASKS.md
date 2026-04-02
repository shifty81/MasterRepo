# NovaForge Game — Reset Task List

## Phase 0 — Bootstrap
- [ ] add `Config/novaforge.project.json`
- [ ] verify clean configure/build from fresh checkout
- [ ] verify editor executable is produced
- [ ] verify game executable is produced
- [ ] verify project context resolves correctly
- [ ] add startup smoke-test checklist

## Phase 1 — Dev World Only
- [ ] define single dev world target
- [ ] lock spawn path
- [ ] wire editor load path into dev world
- [ ] wire runtime load path into dev world
- [ ] add basic save/load hooks
- [ ] add world debug overlay

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
