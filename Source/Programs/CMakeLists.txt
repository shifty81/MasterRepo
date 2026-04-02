# NovaForge Editor executable
if(NF_BUILD_EDITOR)
    add_executable(NovaForgeEditor NovaForgeEditor/main.cpp)
    target_include_directories(NovaForgeEditor PRIVATE ${CMAKE_SOURCE_DIR}/Source)
    target_link_libraries(NovaForgeEditor PRIVATE
        NF::Editor
        NF::Game
        NF::Engine
        NF::Renderer
        NF::Physics
        NF::Audio
        NF::Animation
        NF::Input
        NF::Networking
        NF::UI
        NF::Core
    )
    message(STATUS "  NovaForgeEditor executable configured")
endif()

# NovaForge Game executable
if(NF_BUILD_GAME)
    add_executable(NovaForgeGame
        NovaForgeGame/main.cpp
        NovaForgeGame/GameClientApp.cpp
    )
    target_include_directories(NovaForgeGame PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/Programs/NovaForgeGame
    )
    target_link_libraries(NovaForgeGame PRIVATE
        NF::Game
        NF::Engine
        NF::Renderer
        NF::Physics
        NF::Audio
        NF::Animation
        NF::Input
        NF::Networking
        NF::UI
        NF::Core
    )
    message(STATUS "  NovaForgeGame executable configured")
endif()
