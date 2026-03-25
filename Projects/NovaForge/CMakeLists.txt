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

# ── NF-10: Packaging (CPack) ───────────────────────────────────────────────
# Run from repo root: cmake --build Builds/release --target NovaForge
# Then: cpack -G ZIP -C Release --config Builds/release/CPackConfig.cmake
install(TARGETS NovaForge RUNTIME DESTINATION bin)

# Copy game assets alongside the binary
install(DIRECTORY ${CMAKE_SOURCE_DIR}/Projects/NovaForge/Assets/
        DESTINATION bin/Assets
        OPTIONAL)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/Projects/NovaForge/Scenes/
        DESTINATION bin/Scenes)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/Projects/NovaForge/Recipes/
        DESTINATION bin/Recipes)

if(NOT CPACK_INITIALIZED)
    set(CPACK_PACKAGE_NAME            "NovaForge")
    set(CPACK_PACKAGE_VERSION         "0.1.0")
    set(CPACK_PACKAGE_VENDOR          "AtlasForge Studios")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NovaForge — space-sim built on AtlasEngine")
    set(CPACK_GENERATOR               "ZIP")
    set(CPACK_PACKAGE_FILE_NAME       "NovaForge-0.1.0-win64")
    set(CPACK_INSTALL_CMAKE_PROJECTS  "${CMAKE_BINARY_DIR};NovaForge;ALL;/")
    include(CPack)
    set(CPACK_INITIALIZED TRUE CACHE BOOL "CPack already configured" FORCE)
endif()
