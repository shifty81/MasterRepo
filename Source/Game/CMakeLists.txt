add_library(NFGame STATIC
    # App
    App/ProjectContext.cpp
    App/Session.cpp
    App/Bootstrap.cpp
    App/Orchestrator.cpp
    App/Diagnostics.cpp
    App/Playtest.cpp

    # World
    World/GameWorld.cpp
    World/DevWorldConfig.cpp
    World/WorldSaveLoad.cpp
    World/WorldDebugOverlay.cpp
    World/WorldFileService.cpp
    World/DevWorldSerializer.cpp

    # Voxel Runtime (Phase 2)
    Voxel/ChunkMap.cpp
    Voxel/VoxelEditApi.cpp
    Voxel/VoxelSerializer.cpp
    Voxel/VoxelDebugOverlay.cpp

    # Voxel Mesh (Phase 4)
    Voxel/VoxelMesher.cpp
    Voxel/ChunkMeshCache.cpp

    # Chunk Streaming (Phase 8)
    Voxel/ChunkStreamer.cpp

    # Interaction Loop (Phase 3)
    Interaction/RigState.cpp
    Interaction/Inventory.cpp
    Interaction/MiningTool.cpp
    Interaction/InteractionLoop.cpp

    # Movement (Phase 5)
    Movement/PlayerMovement.cpp

    # Character model (low-poly trench-coat explorer)
    Character/CharacterMeshBuilder.cpp
    Character/PlayerCharacterRenderer.cpp

    # Multiplayer (Phase 6)
    Net/NetReplicator.cpp
    Net/GameServer.cpp
    Net/GameClient.cpp

    # Gameplay stubs
    Gameplay/Combat/CombatSystem.cpp
    Gameplay/Economy/TradeMarket.cpp
    Gameplay/Economy/ResourceRegistry.cpp
    Gameplay/Factions/FactionRegistry.cpp
    Gameplay/Missions/MissionRegistry.cpp
    Gameplay/Mining/MiningSystem.cpp
    Gameplay/Exploration/ExplorationSystem.cpp
    Gameplay/PCG/PCGWorldGen.cpp
    Gameplay/Progression/ProgressionSystem.cpp
    Gameplay/PlayerSystems/PlayerController.cpp
    Gameplay/Builder/BuilderSystem.cpp
    Gameplay/Salvage/SalvageSystem.cpp
    Gameplay/Season/TitanRaceSystem.cpp
    Gameplay/Characters/CharacterSystem.cpp
    Gameplay/Station/StationServices.cpp
    Gameplay/Manufacturing/ManufacturingQueue.cpp
    Gameplay/Fleet/FleetSystem.cpp
    Gameplay/WorldSim/WorldSimSystem.cpp
    Gameplay/Inventory/InventorySystem.cpp
    Gameplay/Storage/StorageSystem.cpp
    Gameplay/Anomaly/AnomalySystem.cpp
    Gameplay/WarSector/WarSectorSystem.cpp
)

add_library(NF::Game ALIAS NFGame)

target_include_directories(NFGame PUBLIC ${CMAKE_SOURCE_DIR}/Source)

target_link_libraries(NFGame PUBLIC
    NF::Core
    NF::Engine
    NF::Renderer
    NF::Networking
    NF::Audio
)

message(STATUS "  NF::Game configured")
