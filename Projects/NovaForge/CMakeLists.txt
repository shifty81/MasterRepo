cmake_minimum_required(VERSION 3.20)

add_executable(NovaForge main.cpp)
target_include_directories(NovaForge PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_features(NovaForge PRIVATE cxx_std_20)
target_link_libraries(NovaForge PRIVATE AtlasEngine CoreLib RuntimeLib)
message(STATUS "  NovaForge game configured")
