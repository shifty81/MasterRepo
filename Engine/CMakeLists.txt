add_library(AtlasEngine STATIC
    Core/Engine.cpp
    Core/Logger.cpp
    Core/RuntimeBootstrap.cpp
    Input/InputManager.cpp
    Physics/PhysicsWorld.cpp
    Camera/Camera.cpp
    Audio/AudioEngine.cpp
    Animation/AnimationGraph.cpp
    Animation/AnimationNodes.cpp
    Sim/TickScheduler.cpp
    Window/Window.cpp
    Platform/Platform.cpp
    Render/Renderer.cpp
)

target_include_directories(AtlasEngine PUBLIC ${CMAKE_SOURCE_DIR})

# Prevent Windows.h min/max macros from interfering with std::min/std::max
if(WIN32)
    target_compile_definitions(AtlasEngine PUBLIC NOMINMAX)
endif()

target_compile_features(AtlasEngine PUBLIC cxx_std_20)
