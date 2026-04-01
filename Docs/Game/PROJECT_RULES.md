# NovaForge Game — Project Rules

## Project Identity
NovaForge is a voxel-first open-world and open-galaxy survival/building game.

## Hard Boundary
Do not generate Atlas Suite shell features, generic tooling, generic editor host
frameworks, generic asset libraries, generic intake systems, repo migration
tooling, workspace services, or Atlas-wide AI broker systems unless explicitly
required as game-local project code.

## Locked Design Rules
- voxel layer first
- low-poly visual layer later
- low-poly is a visual and readability wrapper over voxel state
- voxel state drives structure, mining, repair, damage, destruction, and PCG
- player suit system is called the R.I.G.
- R.I.G. starts as a minimal exo frame
- life support is built into the R.I.G.
- helmet deploys from the back assembly
- initial HUD is minimal
- R.I.G. supports modular upgrades and swappable parts
- backpack crafting unlocks through progression and module attachment
- seasons are configurable on client and server, normally enforced by server
- default target season length is about 6 months

## Allowed Domains
- Core
- World
- Gameplay
- Rig
- Building
- Salvage
- Seasons
- Factions
- AI
- UI
- Client
- Server
- Validation
- Tests

## Forbidden Domains (in game source)
- Suite shell code
- generic editor hosts
- generic repo migration engines
- generic Git tooling
- generic project intake ownership
- generic workspace control panels
- generic notification center ownership

## Output Behavior
When generating code:
1. create compile-safe scaffolds first
2. keep dependencies one-directional
3. keep TODO markers explicit
4. do not create undeclared major systems
5. stay compatible with NF engine hosting
6. propose changes as files to create, files to edit, why, dependency impact, and next compile-safe step
