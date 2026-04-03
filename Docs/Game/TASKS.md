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

## Phase 4 — Voxel Mesh Rendering
- [x] implement VoxelMesher (culled-face generation with normals + type palette)
- [x] implement ChunkMeshCache (GPU mesh per chunk, dirty rebuild)
- [x] create voxel GLSL shader (Phong lighting, per-type colour palette)
- [x] wire ForwardRenderer into EditorViewport
- [x] wire ForwardRenderer into GameClientApp
- [x] generate starter terrain (9 chunks) in GameWorld::Initialize
- [x] add VoxelMesh unit tests (10 tests)

## Phase 5 — Movement & FPS Camera
- [x] add ChunkMap::IsSolidAt() voxel solidity query
- [x] add ChunkMap::RaycastVoxel() DDA voxel raycast
- [x] implement PlayerMovement (WASD, mouse look, jump, sprint, gravity)
- [x] implement voxel-aware AABB collision resolution
- [x] wire PlayerMovement into Orchestrator tick loop
- [x] wire FPS camera into GameClientApp (eye locked to player position)
- [x] wire mouse look (RMB) and WASD input into GameClientApp
- [x] add HUD position/grounded display
- [x] add Phase 5 unit tests (21 tests: IsSolidAt, raycast, movement, collision)

## Phase 6 — Multiplayer Foundation
- [x] implement NetMessage protocol (typed packets, header, serialize/deserialize)
- [x] implement NetChannel (framed reliable message delivery over Socket)
- [x] implement NetReplicator (snapshot build, serialize/deserialize, apply)
- [x] implement GameServer (authoritative host, accepts local clients, ticks world)
- [x] implement GameClient (connect, send input, receive snapshots)
- [x] define NetPlayerState, NetVoxelEdit, NetWorldSnapshot, NetClientInput structs
- [x] wire server-side authoritative PlayerMovement per client
- [x] add Phase 6 unit tests (30 tests: protocol, channel, replicator, server, client)

## Phase 7 — Platform Networking & Orchestrator Integration
- [x] implement platform Socket (Win32 Winsock2 / POSIX)
- [x] add Orchestrator net-mode enum (Solo, ListenServer, Dedicated, Client)
- [x] wire GameServer + GameClient lifecycle into Orchestrator
- [x] add local loopback integration test (server + client in-process)
- [x] implement listen-server mode (host plays while serving)
- [x] implement connection handshake end-to-end over real socket
- [x] add Phase 7 unit + integration tests (18 tests)

## Phase 8 — Chunk Streaming & LOD
- [x] implement ChunkStreamer (load/unload within radius)
- [x] add background-thread chunk generation from seed
- [x] implement distance-based mesh LOD
- [x] add memory budget / chunk eviction policy
- [x] add save-on-unload for modified chunks
- [x] add Phase 8 unit tests (22 tests)

## Phase 9 — Audio Foundation
- [x] implement platform audio backend (WASAPI / stub)
- [x] wire real AudioDevice init with hardware
- [x] add sound asset loading (WAV/OGG)
- [x] implement AudioMixer channels with volume/pan
- [x] implement SpatialAudio 3D positioning + attenuation
- [x] add placeholder sounds for mining/interaction/ambient
- [x] add Phase 9 unit tests (33 tests)

## Phase 10 — Advanced Gameplay Systems
- [x] implement MiningSystem (full mining progression)
- [x] implement BuilderSystem (voxel construction)
- [x] implement ProgressionSystem (XP, unlocks, skills)
- [x] implement InventorySystem + StorageSystem (containers, transfer)
- [x] implement CombatSystem (damage, health, death/respawn)
- [x] add Phase 10 unit tests

## Phase 11 — Economy & Trade
- [x] implement ResourceRegistry (resource definitions, base prices, mass, tradeable/craftable flags)
- [x] implement TradeMarket (buy/sell with supply/demand dynamic pricing)
- [x] implement ManufacturingQueue (recipe-based crafting, ingredient deduction, progress/collect)
- [x] implement StationServices (docking, repair, refuel, trade + manufacturing at station)
- [x] add Phase 11 unit tests
