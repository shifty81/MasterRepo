add_library(PCGAdvanced STATIC
    PCGAdvanced.cpp
)

target_include_directories(PCGAdvanced PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
