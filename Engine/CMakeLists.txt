find_package(OpenGL REQUIRED)

# GLFW — prefer system package, fall back gracefully
find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
    find_library(GLFW_LIB glfw REQUIRED)
    set(GLFW_LIBS ${GLFW_LIB})
else()
    set(GLFW_LIBS glfw)
endif()

add_library(AtlasEngine STATIC
    Core/Engine.cpp
    Core/Logger.cpp
    Core/RuntimeBootstrap.cpp
    Input/InputManager.cpp
    Physics/PhysicsWorld.cpp
    Camera/Camera.cpp
    Audio/AudioEngine.cpp
    # Blueprint — audio_pipeline
    Audio/AudioPipeline.cpp
    Animation/AnimationGraph.cpp
    Animation/AnimationNodes.cpp
    # Blueprint — animation_pipeline
    Animation/AnimationPipeline.cpp
    Sim/TickScheduler.cpp
    Window/Window.cpp
    Platform/Platform.cpp
    Render/Renderer.cpp
    # Blueprint — rendering_pipeline
    Render/RenderPipeline.cpp
    # Phase 19 — networking core
    Network/NetworkManager.cpp
    # Phase 20 — scene graph implementation
    Scene/SceneGraph.cpp
    # Phase 21 — net context + shader manager
    Net/NetContext.cpp
    Shader/ShaderManager.cpp
    # Phase 22 — camera controller
    Camera/CameraController.cpp
    # Phase 23 — level-of-detail manager
    Lod/LODSystem.cpp
    # Phase 24 — CPU particle system
    Particles/ParticleSystem.cpp
    # Phase 25 — post-process pipeline
    PostProcess/PostProcessPipeline.cpp
    # Phase 26 — heightmap terrain system
    Terrain/TerrainSystem.cpp
    # Phase 27 — chunk-based voxel world
    Voxel/VoxelWorld.cpp
)

target_include_directories(AtlasEngine PUBLIC ${CMAKE_SOURCE_DIR})

# Expose GLFW + OpenGL to anything that links AtlasEngine
target_link_libraries(AtlasEngine PUBLIC OpenGL::GL ${GLFW_LIBS})

# Prevent Windows.h min/max macros from interfering with std::min/std::max
if(WIN32)
    target_compile_definitions(AtlasEngine PUBLIC NOMINMAX)
endif()

target_compile_features(AtlasEngine PUBLIC cxx_std_20)
