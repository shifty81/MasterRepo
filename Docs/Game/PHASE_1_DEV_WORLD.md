# Phase 1 — Dev World Only

## Goal
Create one stable world used for nearly all early testing.

## World Intent
This is not procedural breadth yet.
This is a controlled development sandbox.

## Required Systems
- dev world definition asset/config
- fixed seed or hand-authored layout
- spawn point definition
- camera and locomotion baseline
- editor load into dev world
- runtime load into dev world
- world debug overlay
- basic save/load hook

## Preferred Folder Targets
- `Source/Game/World/`
- `Source/Game/Gameplay/PlayerSystems/`
- `Content/Worldgen/`
- `Content/Definitions/`
- `Content/Prototypes/`

## Definition of Done
- editor can open the dev world
- client can spawn into the dev world
- the same test world loads repeatedly without drift
- camera, movement, and collision baseline work well enough for testing
- world identity and seed are logged at startup
