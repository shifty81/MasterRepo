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
    Panels/Profiler/ProfilerPanel.cpp
    Panels/ErrorPanel/ErrorPanel.cpp
    GUIEditor/GUIEditor.cpp
    ${CMAKE_SOURCE_DIR}/WorkspaceState/WorkspaceState.cpp
    # Phase 10.10 — VR/AR Preview
    Viewport/VRPreview.cpp
    # Phase 10.11 — Plugin Marketplace
    ${CMAKE_SOURCE_DIR}/Plugins/Marketplace/PluginMarketplace.cpp
    # GUI renderer (OpenGL + GLFW)
    Render/EditorRenderer.cpp
    # Phase 20 — anomaly/constraint visual overlay
    Overlay/AnomalyOverlay.cpp
    # Phase 22 — undoable command bus (undo/redo stack)
    UndoableCommandBus.cpp
    # NFE — editor tool registry (lifecycle, activate/deactivate)
    Tools/EditorToolRegistry.cpp
    # NFE — entity inspector overlay tool
    Tools/EntityInspectorTool.cpp
    # NFE — material parameter override tool
    Tools/MaterialOverrideTool.cpp
    # EI-10 — project new/open dialog
    Panels/ProjectDialog/ProjectDialog.cpp
)

add_library(EditorLib STATIC ${EDITOR_SOURCES})
target_include_directories(EditorLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(EditorLib PUBLIC cxx_std_20)
target_link_libraries(EditorLib PUBLIC UILib)

# AtlasEditor executable
# Links every subsystem library so the editor has full access to all engine,
# runtime, AI, IDE, tools and builder features.
add_executable(AtlasEditor main.cpp)
target_include_directories(AtlasEditor PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_features(AtlasEditor PRIVATE cxx_std_20)
target_link_libraries(AtlasEditor PRIVATE
    EditorLib
    AtlasEngine
    CoreLib
    VersionLib
    RuntimeLib
    BuilderLib
    PCGLib
    IDELib
    ToolsLib
    ToolsGUI
    AILib
    AgentsLib
    TrainingDataLib
)

message(STATUS "  EditorLib configured with ${CMAKE_CURRENT_SOURCE_DIR}")
