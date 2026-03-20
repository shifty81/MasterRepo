add_library(RuntimeLib STATIC
    ECS/ECS.cpp
    ECS/System.cpp
    ECS/DeltaEditStore.cpp
    World/CubeSphereLayout.cpp
    World/VoxelGridLayout.cpp
    World/SceneManager.cpp
    Systems/Systems.cpp
    Player/PlayerController.cpp
    Inventory/Inventory.cpp
    Equipment/Equipment.cpp
    Crafting/Crafting.cpp
    Damage/Damage.cpp
    Spawn/Spawn.cpp
    SaveLoad/SaveLoad.cpp
    UI/HUD.cpp
    BuilderRuntime/BuilderRuntime.cpp
    Collision/Collision.cpp
    Gameplay/Prefabs/PrefabSystem.cpp
)

target_include_directories(RuntimeLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(RuntimeLib PUBLIC cxx_std_20)
