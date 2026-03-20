# ==============================================================================
# MasterRepo — Root CMake Build File
# ==============================================================================
# Build order: Core → Engine → Runtime → Builder → UI → Editor → AI → Projects
# ==============================================================================

cmake_minimum_required(VERSION 3.20)
project(MasterRepo
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "Offline AI-powered game engine + editor + coding agent system"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Options
# ==============================================================================
option(BUILD_EDITOR     "Build the editor application"          ON)
option(BUILD_RUNTIME    "Build the runtime / game application"  ON)
option(BUILD_AI         "Build AI agent system"                 ON)
option(BUILD_TOOLS      "Build standalone tools"                OFF)
option(BUILD_TESTS      "Build unit tests"                      OFF)

# ==============================================================================
# Output directories
# ==============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ==============================================================================
# Include paths (all includes are relative to project root)
# ==============================================================================
include_directories(${CMAKE_SOURCE_DIR})

# ==============================================================================
# Subdirectories (build order matters — dependencies first)
# ==============================================================================

# Phase 1: Core systems (everything depends on this)
add_subdirectory(Core)

# Phase 2: Engine foundation
add_subdirectory(Engine)

# Phase 3: Editor framework
# if(BUILD_EDITOR)
#     add_subdirectory(Editor)
# endif()

# Phase 4: Runtime / gameplay
if(BUILD_RUNTIME)
    add_subdirectory(Runtime)
endif()

# Phase 5: AI agent system
if(BUILD_AI)
    add_subdirectory(AI)
    add_subdirectory(Agents)
endif()

# Phase 6: Builder system
add_subdirectory(Builder)

# Phase 7: PCG
# add_subdirectory(PCG)

# Phase 8: IDE integration
# add_subdirectory(IDE)

# Phase 9: Tools
# if(BUILD_TOOLS)
#     add_subdirectory(Tools)
# endif()

# Phase 10: Game projects
# add_subdirectory(Projects/NovaForge)

# ==============================================================================
# Status
# ==============================================================================
message(STATUS "")
message(STATUS "=== MasterRepo v${PROJECT_VERSION} ===")
message(STATUS "  Build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "  Editor:        ${BUILD_EDITOR}")
message(STATUS "  Runtime:       ${BUILD_RUNTIME}")
message(STATUS "  AI:            ${BUILD_AI}")
message(STATUS "  Tools:         ${BUILD_TOOLS}")
message(STATUS "  Tests:         ${BUILD_TESTS}")
message(STATUS "")
