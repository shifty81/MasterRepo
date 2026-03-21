cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# Phase 7: Procedural Content Generation (PCG)
# ==============================================================================

set(PCG_SOURCES
    # Rules
    Rules/PCGRule.cpp

    # Geometry
    Geometry/MeshNode.cpp
    Geometry/LODGraph.cpp

    # World
    World/HeightField.cpp
    World/WorldNode.cpp

    # Textures
    Textures/TextureNode.cpp

    # Audio
    Audio/AudioGenerator.cpp

    # Quests
    Quests/QuestGenerator.cpp

    # Validation
    Validation/PCGValidator.cpp

    # Noise
    Noise/NoiseGenerator.cpp

    # Blueprint — pcg_pipeline
    Pipeline/PCGPipeline.cpp
    # Phase 21 — biome-aware world generation
    Biomes/BiomeGenerator.cpp
    # Phase 22 — BSP dungeon generator
    Dungeon/DungeonGenerator.cpp
)

add_library(PCGLib STATIC ${PCG_SOURCES})

target_include_directories(PCGLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(PCGLib PUBLIC cxx_std_20)

message(STATUS "  PCGLib configured with ${CMAKE_CURRENT_SOURCE_DIR}")
