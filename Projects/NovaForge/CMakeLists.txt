cmake_minimum_required(VERSION 3.20)

add_executable(NovaForge main.cpp)
target_include_directories(NovaForge PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_features(NovaForge PRIVATE cxx_std_20)
# Links the full engine + runtime stack needed for a playable game build.
target_link_libraries(NovaForge PRIVATE
    AtlasEngine
    CoreLib
    VersionLib
    RuntimeLib
    BuilderLib
    PCGLib
    AILib
    AgentsLib
    TrainingDataLib
)
message(STATUS "  NovaForge game configured")
