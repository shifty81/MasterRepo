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
    # Phase 23 — procedural story/narrative generator
    Story/StoryGenerator.cpp
    # Phase 24 — biome-aware vegetation placement
    Vegetation/VegetationGenerator.cpp
    # Phase 25 — dynamic weather simulation
    Weather/WeatherSystem.cpp
    # Phase 26 — procedural city generator
    Cities/CityGenerator.cpp
    # Phase 27 — space station interior layout
    SpaceLayout/SpaceLayoutGenerator.cpp
    # Phase 28 — cellular-automata cave generator
    Cave/CaveGenerator/CaveGenerator.cpp
    # Phase 29 — chunk-based procedural world generator
    World/WorldChunkGen/WorldChunkGen.cpp
    # Phase 30 — procedural quest graph generator
    Quests/QuestGraphGen/QuestGraphGen.cpp
    # Phase 31 — procedural texture generation
    Textures/TextureGenerator/TextureGenerator.cpp
    # Phase 32 — procedural building and structure generation
    Structures/StructureGenerator/StructureGenerator.cpp
    # Advanced PCG: AI rule generation, validation, live preview
    Advanced/PCGAdvanced.cpp
)

add_library(PCGLib STATIC ${PCG_SOURCES})

target_include_directories(PCGLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(PCGLib PUBLIC cxx_std_20)

message(STATUS "  PCGLib configured with ${CMAKE_CURRENT_SOURCE_DIR}")
