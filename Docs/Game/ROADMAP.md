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

### Phase 4 — Voxel Mesh Rendering
Goal: render voxel chunks as lit 3-D geometry in both editor and client.

Deliverables:
- VoxelMesher generates MeshData from Chunk (culled-face, normals, type-palette)
- ChunkMeshCache maintains a GPU Mesh per chunk, rebuilds dirty chunks
- Voxel GLSL shader with Phong lighting and per-type colour palette
- ForwardRenderer wired into EditorViewport and GameClientApp
- Starter terrain (9 chunks) generated at world init
- Tests for mesher correctness (face count, culling, normals, palette)

### Phase 5 — Movement & FPS Camera
Goal: player controller, collision detection, FPS navigation.

Deliverables:
- ChunkMap::IsSolidAt() and ChunkMap::RaycastVoxel() spatial queries
- PlayerMovement with WASD, mouse look, jump, sprint, gravity
- Voxel-aware AABB collision resolution (slide along surfaces)
- FPS camera locked to player eye position in GameClientApp
- RMB mouse look + keyboard movement in standalone client
- HUD position/grounded indicator
- Tests for solidity, raycast, movement, physics, collision

### Phase 6 — Multiplayer Foundation
Goal: server authority, replication, sessions.

Deliverables:
- NetMessage protocol with typed packets and binary serialisation
- NetChannel for framed, reliable message delivery over Socket
- NetReplicator for delta-state snapshot building and application
- GameServer: headless authoritative host, per-client movement, snapshot broadcast
- GameClient: session lifecycle, input submission, snapshot reception
- Replicated data types: NetPlayerState, NetVoxelEdit, NetWorldSnapshot, NetClientInput
- Tests for protocol, channel, replicator, server, and client

### Phase 7 — Platform Networking & Orchestrator Integration
Goal: real networked multiplayer over TCP/UDP sockets; Orchestrator owns net mode.

Deliverables:
- Platform Socket implementation (Win32 Winsock2 / POSIX sockets)
- Orchestrator net-mode switch (solo / listen-server / dedicated / client)
- Orchestrator owns GameServer + GameClient lifecycle
- Local loopback integration test (server + client in same process)
- Listen-server mode: host plays while serving
- Dedicated server headless executable or mode flag
- Connection handshake (hello → welcome → ready) end-to-end over real socket

### Phase 8 — Chunk Streaming & LOD
Goal: dynamic chunk loading/unloading around the player; distance-based LOD.

Deliverables:
- ChunkStreamer: loads/unloads chunks within a configurable radius
- Background thread chunk generation (PCG seed-based)
- Distance-based mesh LOD (full → simplified → skip)
- Memory budget / chunk eviction policy
- Save-on-unload for modified chunks
- Tests for streaming radius, eviction, and LOD transitions

### Phase 9 — Audio Foundation
Goal: spatial audio playback in both editor and client.

Deliverables:
- Platform audio backend (WASAPI on Windows, stub for Linux/macOS)
- AudioDevice init/shutdown with real hardware
- Sound asset loading (WAV/OGG)
- AudioMixer channels with volume/pan
- SpatialAudio 3D positioning + attenuation
- Mining / interaction / ambient placeholder sounds
- Tests for mixer state, spatial attenuation

### Phase 10 — Advanced Gameplay Systems
Goal: begin implementing the deferred gameplay stubs.

Deliverables (prioritised order):
- MiningSystem: full mining progression beyond MiningTool
- BuilderSystem: voxel-based construction / placement
- ProgressionSystem: XP, unlocks, skill tree baseline
- InventorySystem + StorageSystem: full container/transfer logic
- CombatSystem: basic damage model, health, death/respawn
- Remaining stubs iterated as needed

### Phase 11 — Economy & Trade
Goal: establish the resource economy backbone used by all later systems.

Deliverables:
- ResourceRegistry: static catalog of all resource types with base price, mass, tradeable/craftable flags
- TradeMarket: commodity market with supply/demand dynamic pricing (buy/sell, price floor/ceiling)
- ManufacturingQueue: sequential recipe-based crafting queue with ingredient deduction and timed progress
- StationServices: docking lifecycle, repair/refuel services billed in credits, market + factory access
- Tests for registry lookup, trade pricing, manufacturing flow, and station services
