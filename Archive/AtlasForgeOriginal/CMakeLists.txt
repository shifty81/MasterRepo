# EVEOFFLINE - Root CMakeLists.txt
# Game project build: Client and Server
#
# Architecture:
#   cpp_client/   → EVEOFFLINE game client (OpenGL)
#   cpp_server/   → EVEOFFLINE game server (dedicated)
#
# The Atlas Engine is developed separately at https://github.com/shifty81/Atlas
# When building client/server standalone, each has its own CMakeLists.txt.

cmake_minimum_required(VERSION 3.15)
project(EveOffline VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard (C++20 to match Atlas Engine requirements)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_CLIENT "Build C++ OpenGL client" ON)
option(BUILD_SERVER "Build C++ dedicated server" ON)

# --- EVEOFFLINE Game Client ---
if(BUILD_CLIENT)
    message(STATUS "Adding EVEOFFLINE Game Client...")
    add_subdirectory(cpp_client)
endif()

# --- EVEOFFLINE Game Server ---
if(BUILD_SERVER)
    message(STATUS "Adding EVEOFFLINE Game Server...")
    add_subdirectory(cpp_server)
endif()

message(STATUS "")
message(STATUS "EVEOFFLINE Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Game Client:  ${BUILD_CLIENT}")
message(STATUS "  Game Server:  ${BUILD_SERVER}")
message(STATUS "")
