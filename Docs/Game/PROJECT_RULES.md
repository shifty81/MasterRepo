# NovaForge Game — Project Rules

## Project Identity
NovaForge is a standalone game repo.
This repo owns the NovaForge game, its engine runtime, and its game-specific editor.

## Hard Boundary
Do not add the following into this repo unless explicitly required as local game code:
- Atlas Suite shell systems
- AtlasAI broker systems
- generic workspace apps
- generic repo tooling
- generic intake pipelines
- generic GitHub control panels
- cross-project shell services
- suite-wide notification systems

## Locked Design Rules
- voxel layer first
- low-poly visual wrapper later
- voxel state is authoritative for structure, mining, repair, damage, and destruction
- player suit system is the R.I.G.
- starter state is a minimal exo frame
- helmet deploys from the back rig assembly
- initial HUD is minimal
- seasons are configurable but normally server-authoritative
- target season length is about 6 months by default
- 3D FPS is the primary play mode
- fleet gameplay is later and secondary

## Build Rules
- editor is built for NovaForge but does not ship with the game
- code generation should stay compile-safe first
- no circular dependencies
- no hidden major systems
- every new system must map to an explicit phase target
