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
    # Phase 31 — recipe-based crafting system
    Crafting/CraftingSystem/CraftingSystem.cpp
    # NFE — autonomous AI miner state machine
    Sim/AIMinerStateMachine/AIMinerStateMachine.cpp
    # NFE — NPC credits economy (wallet system)
    Sim/AIWalletSystem/AIWalletSystem.cpp
    # NFE — player skill tree with prerequisites and effects
    Gameplay/SkillTree/SkillTree.cpp
    # Phase 32 — multi-channel audio mixer with bus routing and effects chain
    Audio/AudioMixer/AudioMixer.cpp
    # Phase 33 — component-based damage system with typed damage, resistances, and stacking
    Damage/DamageSystem/DamageSystem.cpp
    # Phase 36 — full builder-NovaForge integration (ships, stations, interiors, damage, crafting gate)
    BuilderRuntime/NovaForgeBuilderIntegration.cpp
)

target_include_directories(RuntimeLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(RuntimeLib PUBLIC cxx_std_20)
