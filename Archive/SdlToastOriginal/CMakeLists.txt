cmake_minimum_required(VERSION 3.22)

project(HarvestQuest VERSION 1.0.0 LANGUAGES C CXX)

# Atlas Forge convention: C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compile options for better warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Fetch Raylib via FetchContent
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(raylib)

# Include directories for project sources.
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

# Source files
set(SOURCES
    src/main.cpp
    src/engine/Game.cpp
    src/engine/Renderer.cpp
    src/engine/Input.cpp
    src/engine/AssetManager.cpp
    src/engine/AudioManager.cpp
    src/engine/SpriteSheet.cpp
    src/engine/TilesetConfig.cpp
    src/engine/Logger.cpp
    src/entities/Entity.cpp
    src/entities/Player.cpp
    src/entities/Enemy.cpp
    src/entities/NPC.cpp
    src/systems/Combat.cpp
    src/systems/Farming.cpp
    src/systems/Inventory.cpp
    src/systems/Calendar.cpp
    src/systems/Crafting.cpp
    src/systems/Dialogue.cpp
    src/systems/SaveSystem.cpp
    src/systems/Quest.cpp
    src/systems/Energy.cpp
    src/systems/Skills.cpp
    src/systems/Fishing.cpp
    src/systems/Shop.cpp
    src/systems/Mining.cpp
    src/systems/AnimalHusbandry.cpp
    src/systems/SkillBonuses.cpp
    src/world/Map.cpp
    src/world/Tile.cpp
    src/world/Dungeon.cpp
    src/world/DungeonTheme.cpp
    src/world/WorldGenerator.cpp
    src/ui/HUD.cpp
    src/ui/Menu.cpp
    src/ui/DialogueBox.cpp
)

# Header files
set(HEADERS
    src/engine/Game.h
    src/engine/Renderer.h
    src/engine/Input.h
    src/engine/AssetManager.h
    src/engine/AudioManager.h
    src/engine/SpriteSheet.h
    src/engine/TilesetConfig.h
    src/engine/Logger.h
    src/entities/Entity.h
    src/entities/Player.h
    src/entities/Enemy.h
    src/entities/NPC.h
    src/systems/Combat.h
    src/systems/Farming.h
    src/systems/Inventory.h
    src/systems/Calendar.h
    src/systems/Crafting.h
    src/systems/Dialogue.h
    src/systems/SaveSystem.h
    src/systems/Quest.h
    src/systems/Energy.h
    src/systems/Skills.h
    src/systems/Fishing.h
    src/systems/Shop.h
    src/systems/Mining.h
    src/systems/AnimalHusbandry.h
    src/systems/SkillBonuses.h
    src/world/Map.h
    src/world/Tile.h
    src/world/Dungeon.h
    src/world/DungeonTheme.h
    src/world/WorldGenerator.h
    src/ui/HUD.h
    src/ui/Menu.h
    src/ui/DialogueBox.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link Raylib
target_link_libraries(${PROJECT_NAME} raylib)

# Copy assets to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)

# Installation rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

install(DIRECTORY assets/
    DESTINATION share/${PROJECT_NAME}/assets
)

# Test support (Atlas Forge convention)
enable_testing()
add_subdirectory(tests)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "HarvestQuest")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "An Atlas Forge game — Zelda meets Stardew Valley")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
include(CPack)
