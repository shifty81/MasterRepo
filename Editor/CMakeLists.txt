cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# Phase 3: Editor Framework
# ==============================================================================

set(EDITOR_SOURCES
    Commands/EditorCommandBus.cpp
    Panels/SceneOutliner/LayerTagSystem.cpp
    Panels/Inspector/InspectorTools.cpp
    Panels/ContentBrowser/ContentBrowser.cpp
    Docking/DockingSystem.cpp
    Gizmos/GizmoSystem.cpp
    # Phase 3 new components
    Viewport/ViewportPanel.cpp
    Panels/Console/ConsolePanel.cpp
    Modes/EditorModes.cpp
    NodeEditors/NodeEditor.cpp
    UI/EditorLayout.cpp
    Overlay/OverlaySystem.cpp
    BuilderEditor/BuilderEditor.cpp
    MaterialEditor/MaterialEditor.cpp
    PCGEditor/PCGEditor.cpp
)

add_library(EditorLib STATIC ${EDITOR_SOURCES})
target_include_directories(EditorLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(EditorLib PUBLIC cxx_std_20)

# AtlasEditor executable
add_executable(AtlasEditor main.cpp)
target_include_directories(AtlasEditor PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_features(AtlasEditor PRIVATE cxx_std_20)
target_link_libraries(AtlasEditor PRIVATE EditorLib AtlasEngine CoreLib RuntimeLib)

message(STATUS "  EditorLib configured with ${CMAKE_CURRENT_SOURCE_DIR}")
