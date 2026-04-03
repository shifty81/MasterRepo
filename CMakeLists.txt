cmake_minimum_required(VERSION 3.20)

# Must be set before project() to take effect via CMP0091
cmake_policy(SET CMP0091 NEW)

project(NovaForge VERSION 0.1.0 LANGUAGES CXX)

# Enforce a consistent MSVC runtime library across all targets to avoid LNK4098
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Build options
option(NF_BUILD_EDITOR "Build NovaForge Editor" ON)
option(NF_BUILD_GAME   "Build NovaForge Game Runtime" ON)
option(NF_BUILD_TESTS  "Build test suite" OFF)

# Modules (static libraries)
add_subdirectory(Source/Core)
add_subdirectory(Source/Engine)
add_subdirectory(Source/Renderer)
add_subdirectory(Source/Physics)
add_subdirectory(Source/Audio)
add_subdirectory(Source/Animation)
add_subdirectory(Source/Input)
add_subdirectory(Source/Networking)
add_subdirectory(Source/UI)
add_subdirectory(Source/Game)

if(NF_BUILD_EDITOR)
    add_subdirectory(Source/Editor)
endif()

# Executables
add_subdirectory(Source/Programs)

if(NF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

message(STATUS "")
message(STATUS "=== NovaForge ${PROJECT_VERSION} ===")
message(STATUS "  Build Editor:  ${NF_BUILD_EDITOR}")
message(STATUS "  Build Game:    ${NF_BUILD_GAME}")
message(STATUS "  Build Tests:   ${NF_BUILD_TESTS}")
message(STATUS "")
