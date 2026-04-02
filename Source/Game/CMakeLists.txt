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

    # Voxel Runtime (Phase 2)
    Voxel/ChunkMap.cpp
    Voxel/VoxelEditApi.cpp
    Voxel/VoxelSerializer.cpp
    Voxel/VoxelDebugOverlay.cpp

    # Interaction Loop (Phase 3)
    Interaction/RigState.cpp
    Interaction/Inventory.cpp
    Interaction/MiningTool.cpp
    Interaction/InteractionLoop.cpp

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
)

add_library(NF::Game ALIAS NFGame)

target_include_directories(NFGame PUBLIC ${CMAKE_SOURCE_DIR}/Source)

target_link_libraries(NFGame PUBLIC
    NF::Core
    NF::Engine
    NF::Renderer
)

message(STATUS "  NF::Game configured")
