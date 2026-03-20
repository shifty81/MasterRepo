cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# Phase 9.7: Version Management
# ==============================================================================

add_library(VersionLib STATIC
    VersionManager.cpp
)

target_include_directories(VersionLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(VersionLib PUBLIC cxx_std_20)

message(STATUS "  VersionLib configured")
