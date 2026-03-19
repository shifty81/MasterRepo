# Nova-Forge: Expeditions — Root CMakeLists.txt
#
# Three-way merge of: AtlasForge + NovaForge + Atlas-NovaForge
# See AUDIT.md for full merge decisions and overlap resolutions.
#
# Architecture:
#   engine/             → AtlasEngine static library
#                           + engine/procedural/  (PCGManager, ConstraintSolver, HullMeshGenerator — from AtlasForge)
#                           + engine/render/      (GBuffer, PBR, ShadowMap, PostProcess — from AtlasForge)
#                           + engine/tools/       (ITool, EditorEventBus, UndoableCommandBus — from AtlasForge)
#                           + engine/ui/atlas/    (AtlasUI widget set — from AtlasForge)
#   modules/            → Engine modules (atlas_gameplay: FactionSystem, CombatFramework, EconomySystem)
#   editor/             → AtlasEditor with full ToolingLayer (26 tools)
#   runtime/            → Atlas standalone runtime
#   client/             → Atlas client wrapper
#   server/             → Atlas server wrapper
#   cpp_common/         → Shared client/server common library (from NovaForge)
#   cpp_client/         → Nova Forge game client (311 files)
#   cpp_server/         → Nova Forge game server (449 systems, 548 tests, decomposed GameSession)
#   projects/           → Example game modules (eveoffline, arena2d, novaforge)
#   tileeditor/         → Tile editor tool
#   tests/              → Engine unit tests (including AtlasForge PCG/render/UI tests)
#   atlas_tests/        → Atlas engine integration test suite

cmake_minimum_required(VERSION 3.22)
project(NovaForgeExpeditions VERSION 0.1.0 LANGUAGES C CXX)

# Project identity
set(NFE_ENGINE_NAME "AtlasEngine")
set(NFE_GAME_NAME   "Nova-Forge: Expeditions")

# C++20 standard (Atlas-NovaForge requirement)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Unified output directories so all executables land together
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(BUILD_CLIENT "Build C++ OpenGL client" ON)
option(BUILD_SERVER "Build C++ dedicated server" ON)
option(BUILD_COMMON "Build cpp_common shared library" ON)
option(BUILD_ATLAS_ENGINE "Build Atlas Engine library" ON)
option(BUILD_ATLAS_EDITOR "Build Atlas Editor" ON)
option(BUILD_ATLAS_TESTS "Build Atlas Engine tests" ON)
option(ATLAS_INCLUDE_TOOLS "Include dev tools (diagnostics overlay, namespace rewriter)" ON)
option(BUILD_BLENDER_ADDON_DOCS "Include Blender addon documentation in install" ON)

# --- Atlas Engine (core library) ---
# Includes: engine/procedural, engine/render, engine/tools, engine/ui/atlas (all from AtlasForge merge)
if(BUILD_ATLAS_ENGINE)
    message(STATUS "Adding Atlas Engine library...")
    add_subdirectory(engine)
endif()

# --- Engine Modules ---
if(BUILD_ATLAS_ENGINE)
    add_subdirectory(modules/atlas_gameplay)
endif()

# --- Atlas Editor ---
if(BUILD_ATLAS_EDITOR AND BUILD_ATLAS_ENGINE)
    message(STATUS "Adding Atlas Editor...")
    add_subdirectory(editor)
endif()

# --- Atlas Runtime ---
add_subdirectory(runtime)

# --- Atlas Client & Server wrappers ---
add_subdirectory(client)
add_subdirectory(server)

# --- Example game modules ---
add_subdirectory(projects/eveoffline/module)
add_subdirectory(projects/arena2d/module)
add_subdirectory(projects/novaforge/module)

# --- Tile Editor ---
add_subdirectory(tileeditor)

# --- Nova Forge Shared Common Library (from NovaForge) ---
# TODO (AUDIT.md §5): wire cpp_common link into cpp_client and cpp_server targets
if(BUILD_COMMON)
    message(STATUS "Adding Nova Forge common library...")
    add_subdirectory(cpp_common)
endif()

# --- Nova Forge Game Client ---
if(BUILD_CLIENT)
    message(STATUS "Adding C++ Client to solution...")
    add_subdirectory(cpp_client)
endif()

# --- Nova Forge Game Server ---
# Contains: 449 game systems, 548 test files, decomposed GameSession (from 3-way merge)
if(BUILD_SERVER)
    message(STATUS "Adding C++ Server to solution...")
    add_subdirectory(cpp_server)
endif()

# --- Atlas Engine Tests ---
# Includes AtlasForge-sourced: test_atlas_ui, test_editor_tools, test_pcg_framework,
#   test_procedural_pipeline, test_rendering_pipeline
if(BUILD_ATLAS_TESTS AND BUILD_ATLAS_ENGINE)
    enable_testing()
    message(STATUS "Adding Atlas Engine tests...")
    add_subdirectory(atlas_tests)
endif()

# --- Legacy / Engine Tests ---
# Only build when atlas_tests is disabled (both declare "AtlasTests" target)
if(NOT BUILD_ATLAS_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "")
message(STATUS "Nova-Forge: Expeditions Build Configuration:")
message(STATUS "  Project:       ${NFE_GAME_NAME} v${PROJECT_VERSION}")
message(STATUS "  C++ Standard:  ${CMAKE_CXX_STANDARD}")
message(STATUS "  Atlas Engine:  ${BUILD_ATLAS_ENGINE}")
message(STATUS "  Atlas Editor:  ${BUILD_ATLAS_EDITOR}")
message(STATUS "  Atlas Tests:   ${BUILD_ATLAS_TESTS}")
message(STATUS "  Common Lib:    ${BUILD_COMMON}")
message(STATUS "  Game Client:   ${BUILD_CLIENT}")
message(STATUS "  Game Server:   ${BUILD_SERVER}")
message(STATUS "  Include Tools: ${ATLAS_INCLUDE_TOOLS}")
message(STATUS "")
message(STATUS "See AUDIT.md for merge decisions and known overlap issues.")
message(STATUS "See ROADMAP.md for the 7-phase development plan.")
message(STATUS "")
