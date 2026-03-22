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
    # Phase 19 — multiplayer state synchronisation
    StateSync/StateSync.cpp
    # Phase 20 — NPC behaviour controller
    NPC/NPCController.cpp
    # Phase 21 — dialogue tree system
    Dialogue/DialogueSystem.cpp
    # Phase 22 — runtime animation controller
    Animation/AnimationController.cpp
    # Phase 23 — game session state machine
    Gameplay/GameSession/GameSession.cpp
    # Phase 24 — behavior tree for NPC AI
    AI/BehaviorTree/BehaviorTree.cpp
    # Phase 25 — achievement tracking system
    Gameplay/AchievementSystem/AchievementSystem.cpp
    # Phase 26 — multiplayer lobby management
    Multiplayer/LobbySystem/LobbySystem.cpp
    # Phase 27 — action-map input layer
    Input/InputMapper/InputMapper.cpp
    # Phase 28 — AABB rigid-body physics simulation
    Physics/PhysicsWorld/PhysicsWorld.cpp
    # Phase 29 — ECS component type registration
    Components/ComponentRegistry/ComponentRegistry.cpp
    # Phase 30 — async asset streaming manager
    Streaming/AssetStreamer/AssetStreamer.cpp
)

target_include_directories(RuntimeLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(RuntimeLib PUBLIC cxx_std_20)
