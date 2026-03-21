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
)

target_include_directories(AtlasEngine PUBLIC ${CMAKE_SOURCE_DIR})

# Prevent Windows.h min/max macros from interfering with std::min/std::max
if(WIN32)
    target_compile_definitions(AtlasEngine PUBLIC NOMINMAX)
endif()

target_compile_features(AtlasEngine PUBLIC cxx_std_20)
