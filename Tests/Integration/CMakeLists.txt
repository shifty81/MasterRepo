cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# Integration Tests — Phase 37.12
# Exercises core game loop, builder, PCG, combat, economy, save/load, etc.
# ==============================================================================

add_executable(IntegrationTests
    IntegrationTestRunner.cpp
    IntegrationTests.cpp
)

target_include_directories(IntegrationTests PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_features(IntegrationTests PRIVATE cxx_std_20)

target_link_libraries(IntegrationTests PRIVATE
    RuntimeLib
    PCGLib
    AtlasEngine
    BuilderLib
    CoreLib
)

add_test(NAME integration COMMAND IntegrationTests)
set_tests_properties(integration PROPERTIES TIMEOUT 60)

message(STATUS "  Integration test target: IntegrationTests")
