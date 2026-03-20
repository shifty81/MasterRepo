cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# UI Library — custom widget framework (Atlas Rule: no ImGui)
# ==============================================================================

add_library(UILib STATIC
    Widgets/Widget.cpp
    Themes/Theme.cpp
    Layouts/Layout.cpp
)

target_include_directories(UILib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(UILib PUBLIC cxx_std_20)

message(STATUS "  UILib configured")
