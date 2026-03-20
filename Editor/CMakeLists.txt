cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# Phase 3: Editor Framework
# ==============================================================================

set(EDITOR_SOURCES
    # Commands
    Commands/EditorCommandBus.cpp

    # Scene Outliner panel
    Panels/SceneOutliner/LayerTagSystem.cpp

    # Inspector panel
    Panels/Inspector/InspectorTools.cpp

    # Content browser panel
    Panels/ContentBrowser/ContentBrowser.cpp

    # Docking & modes
    Docking/DockingSystem.cpp

    # Gizmos & overlays
    Gizmos/GizmoSystem.cpp
)

add_library(EditorLib STATIC ${EDITOR_SOURCES})

target_include_directories(EditorLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(EditorLib PUBLIC cxx_std_20)

message(STATUS "  EditorLib configured with ${CMAKE_CURRENT_SOURCE_DIR}")
