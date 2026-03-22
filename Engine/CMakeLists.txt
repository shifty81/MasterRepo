find_package(OpenGL REQUIRED)

# ==============================================================================
# GLFW — multi-strategy discovery + FetchContent fallback
#
# Strategy 1: find_package(glfw3)  — Homebrew / vcpkg / system package
# Strategy 2: find_library(glfw)   — bare lib already on the search path
# Strategy 3: FetchContent          — download + build from source (CI / Windows
#                                     without vcpkg, etc.)
# ==============================================================================
find_package(glfw3 3.3 QUIET)

if(glfw3_FOUND)
    set(GLFW_LIBS glfw)
    message(STATUS "  GLFW: found via find_package(glfw3)")
else()
    # Try bare library (works on many Linux distros even without the cmake module)
    find_library(GLFW_LIB NAMES glfw glfw3 QUIET)
    if(GLFW_LIB)
        set(GLFW_LIBS ${GLFW_LIB})
        message(STATUS "  GLFW: found via find_library → ${GLFW_LIB}")
    else()
        # Last resort: download + build GLFW 3.4 from source
        include(FetchContent)
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG        3.4
            GIT_SHALLOW    TRUE
        )
        set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glfw)
        set(GLFW_LIBS glfw)
        message(STATUS "  GLFW: built from source via FetchContent (fallback)")
    endif()
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
    # Phase 28 — cascaded shadow map manager
    Shadows/ShadowMapper/ShadowMapper.cpp
    # Phase 29 — view-frustum + occlusion culling
    Occlusion/OcclusionCuller/OcclusionCuller.cpp
    # Phase 30 — runtime decal projection system
    Decals/DecalSystem/DecalSystem.cpp
    # Phase 31 — dynamic scene light manager
    Lighting/LightManager/LightManager.cpp
)

target_include_directories(AtlasEngine PUBLIC ${CMAKE_SOURCE_DIR})

# Expose GLFW + OpenGL to anything that links AtlasEngine
target_link_libraries(AtlasEngine PUBLIC OpenGL::GL ${GLFW_LIBS})

# Prevent Windows.h min/max macros from interfering with std::min/std::max
if(WIN32)
    target_compile_definitions(AtlasEngine PUBLIC NOMINMAX)
endif()

target_compile_features(AtlasEngine PUBLIC cxx_std_20)
