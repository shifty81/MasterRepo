add_library(NFEditor STATIC
    Application/EditorApp.cpp
    Viewport/EditorViewport.cpp
    Viewport/TransformGizmo.cpp
    Panels/SceneOutliner.cpp
    Panels/Inspector.cpp
    Panels/ContentBrowser.cpp
    Panels/ConsolePanel.cpp
    Panels/DockingSystem.cpp
    Panels/PreferencesPanel.cpp
    Panels/LiveProfilerBackend.cpp
    Panels/MaterialEditorPanel.cpp
    Panels/VoxelInspector.cpp
    Panels/HUDPanel.cpp
    Commands/EditorCommand.cpp
)

add_library(NF::Editor ALIAS NFEditor)

target_include_directories(NFEditor PUBLIC ${CMAKE_SOURCE_DIR}/Source)

target_link_libraries(NFEditor PUBLIC
    NF::Core
    NF::Engine
    NF::Renderer
    NF::Game
    NF::Physics
    NF::Audio
    NF::Animation
    NF::Input
    NF::UI
)

message(STATUS "  NF::Editor configured")
