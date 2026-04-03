# NovaForge Test Suite

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

# All tests in one executable
add_executable(NFTests
    Core/MathTests.cpp
    Core/LogTests.cpp
    Core/CoreSystemTests.cpp
    Engine/ECSTests.cpp
    Engine/BehaviorTreeTests.cpp
    Input/GamepadTests.cpp
    Physics/PhysicsTests.cpp
    Animation/AnimationTests.cpp
    Bootstrap/BootstrapSmokeTests.cpp
    DevWorld/DevWorldTests.cpp
    Voxel/VoxelTests.cpp
    VoxelMesh/VoxelMeshTests.cpp
    Interaction/InteractionTests.cpp
    Movement/MovementTests.cpp
    Multiplayer/MultiplayerTests.cpp
    ChunkStreaming/ChunkStreamingTests.cpp
    Audio/AudioTests.cpp
    Gameplay/GameplayTests.cpp
)

target_include_directories(NFTests PRIVATE ${CMAKE_SOURCE_DIR}/Source)

target_link_libraries(NFTests PRIVATE
    NF::Core
    NF::Engine
    NF::Game
    NF::Renderer
    NF::Input
    NF::Physics
    NF::Animation
    NF::Networking
    NF::Audio
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(NFTests)

message(STATUS "  Tests configured (Catch2)")
