# NovaForge Game — Architecture

## Purpose
NovaForge is a voxel-first open-world and open-galaxy survival/building game
built on top of the NovaForge engine (this repository).

## Non-Goals (engine repo scope)
The engine repo does not contain:
- Atlas Suite shell or workspace services
- Generic editor host framework
- Generic content intake or build orchestration
- Generic AI broker workflows
- C# runtime bridge services

## Game Module Layout (`Source/Game/`)
```
Source/Game/
  App/
    Bootstrap/        # startup orchestration (NovaForgeBootstrap)
    Session/          # session lifecycle state machine
    ProjectContext/   # manifest-driven path resolution
    Orchestrator/     # top-level game Init/Tick/Shutdown
    Diagnostics/      # JSON runtime health reporter
    Playtest/         # headless PlaytestSession + TestHarness
  World/              # GameWorld facade over NF::Level
  Gameplay/
    Combat/           # CombatSystem
    Economy/          # TradeMarket, ResourceRegistry, LootResolver
    Factions/         # FactionRegistry
    Missions/         # MissionRegistry, ContractRewardSystem
    Mining/           # MiningSystem
    Exploration/      # ExplorationSystem
    PCG/              # PCGWorldGen
    Progression/      # ProgressionSystem, ProgressionRewardSystem
    PlayerSystems/    # PlayerController
    Builder/          # BuilderSystem
    Salvage/          # SalvageSystem
    Station/          # StationServices
    Manufacturing/    # ManufacturingQueue
    Fleet/            # FleetSystem
    WorldSim/         # WorldSimSystem
    Inventory/        # InventorySystem
    Storage/          # StorageSystem
    Season/           # TitanRaceSystem
    Anomaly/          # AnomalySystem
    WarSector/        # WarSectorSystem
    Characters/       # CharacterSystem, Animation, Equipment, Mech, IK, FPS
```

## Major Domains
- Core
- World (voxel-first; low-poly wrapper deferred)
- Gameplay
- Rig (player suit platform)
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

## Design Locks
- Voxel layer is authoritative for structure, mining, repair, damage, and PCG.
- Low-poly wrapper is deferred until voxel fidelity is stable.
- The player suit platform is the R.I.G. (starts as a minimal exo frame).
- Helmet deploys from the back assembly; initial HUD is minimal.
- Seasons are normally server-authoritative; configurable on client and server.
- Default target season length is approximately 6 months.

## Engine Module Dependencies (`Source/Game/` → engine)
```
NF::Game::App      → NF::Core, NF::Engine
NF::Game::World    → NF::Core, NF::Engine
NF::Game::Gameplay → NF::Core, NF::Engine
NovaForgeGame exe  → NF::Game::*, NF::Renderer, NF::Physics, NF::Audio,
                     NF::Animation, NF::Input, NF::Networking, NF::UI
```
