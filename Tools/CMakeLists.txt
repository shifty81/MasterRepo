cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# Phase 9: Asset Pipeline & Tools
# ==============================================================================

set(TOOLS_SOURCES
    Importer/AssetImporter.cpp
    AssetTools/AssetProcessor.cpp
    BuildTools/BuildAutomation.cpp
    ServerManager/ServerManager.cpp
    ReplayTimeline/ReplayTimeline.cpp
    GitPanel/GitPanel.cpp
    AnalyticsDashboard/AnalyticsDashboard.cpp
    SimulationPlayback/SimulationPlayback.cpp
    DocGenerator/DocGenerator.cpp
    CodeAudit/CodeAudit.cpp
)

add_library(ToolsLib STATIC ${TOOLS_SOURCES})

target_include_directories(ToolsLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(ToolsLib PUBLIC cxx_std_20)

message(STATUS "  ToolsLib configured with ${CMAKE_CURRENT_SOURCE_DIR}")
