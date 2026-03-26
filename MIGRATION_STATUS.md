# Migration Status — MasterRepo Consolidation

> **Living document** tracking the merge of source repositories into MasterRepo.
> Last updated: 2026-03-26 — standalone shifty81/NovaForge merge initiated.

---

## 1. Source Repos Overview

| Repo | GitHub | Files Archived | Contribution |
|------|--------|---------------|--------------|
| **NovaForge** | shifty81/NovaForge | 40 | Main game engine + editor. Engine core, ECS, physics, audio, camera, world layouts, animation, sim scheduler, editor entry point. |
| **AtlasForge-EveOffline** | shifty81/AtlasForge-EveOffline | 91 | Eve Online–style offline game. Client (rendering, UI, audio, network) + server (40+ gameplay systems, ECS, data persistence). |
| **Nova-Forge-Expeditions** | shifty81/Nova-Forge-Expeditions | 83 | Extended engine + game. AI pipeline, procedural generation, platform layer, tools, UI framework, world streaming, net delta compression. |
| **sdltoast** | shifty81/sdltoast | 31 | SDL-based game prototype. Basic engine systems (input, audio, renderer), entities, gameplay systems (farming, crafting, combat), world generation. |

**Total archived: 245 files**

---

## 2. Archive Locations

All original source files are preserved verbatim under `Archive/`. These are reference copies — never deleted, never modified.

| Archive Directory | Source Repo | Files |
|---|---|---|
| `Archive/NovaForgeOriginal/` | NovaForge | 40 |
| `Archive/AtlasForgeOriginal/` | AtlasForge-EveOffline | 91 |
| `Archive/NovaForgeExpeditions/` | Nova-Forge-Expeditions | 83 |
| `Archive/SdlToastOriginal/` | sdltoast | 31 |

Additional archive subdirectories for legacy/broken/replaced code:
`Archive/Broken/`, `Archive/BuilderOld/`, `Archive/EditorOld/`, `Archive/Generated/`, `Archive/OldSystems/`, `Archive/PioneerForge/`, `Archive/Replaced/`

---

## 3. Migration Mapping

### Legend

| Symbol | Meaning |
|--------|---------|
| ✅ | Done — new implementation exists in MasterRepo |
| 📦 | Archived — source file imported to Archive/ |
| 🔄 | In Progress — partially extracted or being refactored |
| ⬜ | Not Started — awaiting extraction and refactoring |

---

### Core/ — Shared Foundation Systems

| Source Repo | Source File | → MasterRepo Path | Archive | Extract |
|---|---|---|---|---|
| NovaForge | engine/core/EventBus.h/.cpp | `Core/Events/EventBus.h/.cpp` | 📦 | ✅ |
| — | *(new implementation)* | `Core/Events/Event.h` | — | ✅ |
| — | *(new implementation)* | `Core/Messaging/MessageBus.h/.cpp` | — | ✅ |
| — | *(new implementation)* | `Core/Messaging/Message.h` | — | ✅ |
| NovaForge | engine/core/GameStateManager.h/.cpp | `Core/GameState/GameStateManager.h/.cpp` | 📦 | ✅ |
| — | *(new implementation)* | `Core/Reflection/TypeInfo.h/.cpp` | — | ✅ |
| — | *(new implementation)* | `Core/Reflection/Reflect.h` | — | ✅ |
| NovaForge | engine/core/Logger.h/.cpp | `Core/` *(see Engine/Core/Logger)* | 📦 | ⬜ |
| Expeditions | engine/sim/HotReloadConfig.h | `Core/` *(TBD)* | 📦 | ⬜ |
| — | — | `Core/TaskSystem/` | — | ⬜ |
| — | — | `Core/Serialization/` | — | ⬜ |
| — | — | `Core/CommandSystem/` | — | ⬜ |
| — | — | `Core/VersionSystem/` | — | ⬜ |
| — | — | `Core/PluginSystem/` | — | ⬜ |
| — | — | `Core/ArchiveSystem/` | — | ⬜ |
| — | — | `Core/ResourceManager/` | — | ⬜ |
| — | — | `Core/Metadata/` | — | ⬜ |

---

### Engine/ — Low-Level Engine

| Source Repo | Source File | → MasterRepo Path | Archive | Extract |
|---|---|---|---|---|
| NovaForge | engine/core/Engine.h/.cpp | `Engine/Core/Engine.h/.cpp` | 📦 | ✅ |
| NovaForge | engine/core/Logger.h/.cpp | `Engine/Core/Logger.h/.cpp` | 📦 | ✅ |
| NovaForge | engine/core/RuntimeBootstrap.h/.cpp | `Engine/Core/RuntimeBootstrap.h/.cpp` | 📦 | ✅ |
| sdltoast | src/engine/Logger.cpp | *(merged into Engine/Core/Logger)* | 📦 | ⬜ |
| **Input** | | | | |
| NovaForge | engine/input/InputManager.h/.cpp | `Engine/Input/InputManager.h/.cpp` | 📦 | ✅ |
| sdltoast | src/engine/Input.h | *(merge into Engine/Input)* | 📦 | ⬜ |
| Expeditions | engine/input/InputManager.cpp | *(merge into Engine/Input)* | 📦 | ⬜ |
| Atlas | cpp_client/ui/input_handler.cpp | *(merge into Engine/Input)* | 📦 | ⬜ |
| **Physics** | | | | |
| NovaForge | engine/physics/PhysicsWorld.h/.cpp | `Engine/Physics/PhysicsWorld.h/.cpp` | 📦 | ✅ |
| Atlas | cpp_client/core/ship_physics.cpp | *(merge into Engine/Physics)* | 📦 | ⬜ |
| **Audio** | | | | |
| NovaForge | engine/audio/AudioEngine.h/.cpp | `Engine/Audio/AudioEngine.h/.cpp` | 📦 | ✅ |
| Atlas | cpp_client/audio/audio_manager.cpp | *(merge into Engine/Audio)* | 📦 | ⬜ |
| Atlas | cpp_client/audio/audio_generator.cpp | *(merge into Engine/Audio)* | 📦 | ⬜ |
| sdltoast | src/engine/AudioManager.h/.cpp | *(merge into Engine/Audio)* | 📦 | ⬜ |
| **Camera** | | | | |
| NovaForge | engine/camera/Camera.h/.cpp | `Engine/Camera/Camera.h/.cpp` | 📦 | ✅ |
| NovaForge | engine/camera/CameraProjectionPolicy.h | `Engine/Camera/CameraProjectionPolicy.h` | 📦 | ✅ |
| Atlas | cpp_client/rendering/camera.cpp | *(merge into Engine/Camera)* | 📦 | ⬜ |
| **Animation** | | | | |
| NovaForge | engine/animation/AnimationGraph.h/.cpp | `Engine/Animation/AnimationGraph.h/.cpp` | 📦 | ✅ |
| NovaForge | engine/animation/AnimationNodes.h/.cpp | `Engine/Animation/AnimationNodes.h/.cpp` | 📦 | ✅ |
| Expeditions | engine/animation/CharacterAnimationController.cpp | `Engine/Animation/CharacterAnimationController.cpp` | 📦 | ⬜ |
| **Sim** | | | | |
| NovaForge | engine/sim/TickScheduler.h/.cpp | `Engine/Sim/TickScheduler.h/.cpp` | 📦 | ✅ |
| **Rendering** | | | | |
| Atlas | cpp_client/rendering/shader.cpp | `Engine/Render/Shader.h/.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/rendering/lighting.cpp | `Engine/Render/Lighting.h/.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/rendering/shadow_map.cpp | `Engine/Render/ShadowMap.h/.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/rendering/pbr_materials.cpp | `Engine/Render/PBRMaterials.h/.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/rendering/visual_effects.cpp | `Engine/Render/VisualEffects.h/.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/rendering/model.cpp | `Engine/Render/Model.h/.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/rendering/resolution_manager.cpp | `Engine/Render/ResolutionManager.cpp` | 📦 | ⬜ |
| Expeditions | engine/render/SpatialHash.h/.cpp | `Engine/Render/SpatialHash.h/.cpp` | 📦 | ⬜ |
| Expeditions | engine/render/InstancedRenderer.h/.cpp | `Engine/Render/InstancedRenderer.h/.cpp` | 📦 | ⬜ |
| Expeditions | engine/render/PostProcess.cpp | `Engine/Render/PostProcess.cpp` | 📦 | ⬜ |
| Expeditions | engine/render/AtlasShaderIR.h | `Engine/Render/AtlasShaderIR.h` | 📦 | ⬜ |
| sdltoast | src/engine/Renderer.cpp | *(merge into Engine/Render)* | 📦 | ⬜ |
| sdltoast | src/engine/SpriteSheet.cpp | `Engine/Render/SpriteSheet.cpp` | 📦 | ⬜ |
| **Platform / Window** | | | | |
| Atlas | cpp_client/rendering/window.cpp | `Engine/Window/Window.cpp` | 📦 | ⬜ |
| Expeditions | engine/platform/Win32Window.cpp | `Engine/Platform/Win32Window.cpp` | 📦 | ⬜ |
| Expeditions | engine/platform/X11Window.cpp | `Engine/Platform/X11Window.cpp` | 📦 | ⬜ |
| **Networking** | | | | |
| Atlas | cpp_client/network/network_manager.cpp | `Engine/Net/NetworkManager.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/network/tcp_client.cpp | `Engine/Net/TcpClient.cpp` | 📦 | ⬜ |
| Expeditions | engine/net/DeltaCompression.h/.cpp | `Engine/Net/DeltaCompression.h/.cpp` | 📦 | ⬜ |
| Expeditions | engine/net/NetworkInterpolationBuffer.h | `Engine/Net/NetworkInterpolationBuffer.h` | 📦 | ⬜ |
| **Assets** | | | | |
| Expeditions | engine/assets/AssetImporter.cpp | `Engine/Assets/AssetImporter.cpp` | 📦 | ⬜ |
| Expeditions | engine/assets/SocketHttpClient.h/.cpp | `Engine/Assets/SocketHttpClient.h/.cpp` | 📦 | ⬜ |
| Expeditions | engine/asset_graph/AssetGraph.h | `Engine/Assets/AssetGraph.h` | 📦 | ⬜ |
| sdltoast | src/engine/AssetManager.cpp | *(merge into Engine/Assets)* | 📦 | ⬜ |
| sdltoast | src/engine/TilesetConfig.cpp | `Engine/Assets/TilesetConfig.cpp` | 📦 | ⬜ |
| **Misc Engine** | | | | |
| Expeditions | engine/voice/VoiceCommand.h/.cpp | `Engine/Voice/VoiceCommand.h/.cpp` | 📦 | ⬜ |
| Expeditions | engine/mod/ModLoader.cpp | `Engine/Mod/ModLoader.cpp` | 📦 | ⬜ |
| Expeditions | engine/graphvm/GraphCompiler.h | `Engine/GraphVM/GraphCompiler.h` | 📦 | ⬜ |
| Expeditions | engine/graphvm/GraphSerializer.h | `Engine/GraphVM/GraphSerializer.h` | 📦 | ⬜ |
| Expeditions | engine/flow/GameFlowGraph.cpp | `Engine/Flow/GameFlowGraph.cpp` | 📦 | ⬜ |

---

### Runtime/ — Gameplay & ECS

| Source Repo | Source File | → MasterRepo Path | Archive | Extract |
|---|---|---|---|---|
| **ECS** | | | | |
| NovaForge | engine/ecs/ECS.h/.cpp | `Runtime/ECS/ECS.h/.cpp` | 📦 | ✅ |
| NovaForge | engine/ecs/System.h/.cpp | `Runtime/ECS/System.h/.cpp` | 📦 | ✅ |
| NovaForge | engine/ecs/EntityCommands.h | `Runtime/ECS/EntityCommands.h` | 📦 | ✅ |
| NovaForge | engine/ecs/DeltaEditStore.h/.cpp | `Runtime/ECS/DeltaEditStore.h/.cpp` | 📦 | ✅ |
| Expeditions | engine/ecs/ECS.h/.cpp | *(duplicate — audit)* | 📦 | ⬜ |
| Expeditions | engine/ecs/System.cpp | *(duplicate — audit)* | 📦 | ⬜ |
| Expeditions | engine/ecs/DeltaEditStore.h/.cpp | *(duplicate — audit)* | 📦 | ⬜ |
| Atlas | cpp_server/ecs/world.cpp | `Runtime/World/ServerWorld.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/ecs/entity.cpp | `Runtime/ECS/ServerEntity.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/core/entity.cpp | *(merge into Runtime/ECS)* | 📦 | ⬜ |
| Atlas | cpp_client/core/entity_manager.cpp | *(merge into Runtime/ECS)* | 📦 | ⬜ |
| Atlas | cpp_client/core/entity_message_parser.cpp | *(merge into Runtime/ECS)* | 📦 | ⬜ |
| **World** | | | | |
| NovaForge | engine/world/WorldLayout.h | `Runtime/World/WorldLayout.h` | 📦 | ✅ |
| NovaForge | engine/world/VoxelGridLayout.h/.cpp | `Runtime/World/VoxelGridLayout.h/.cpp` | 📦 | ✅ |
| NovaForge | engine/world/CubeSphereLayout.h/.cpp | `Runtime/World/CubeSphereLayout.h/.cpp` | 📦 | ✅ |
| Expeditions | engine/world/WorldNodes.cpp | `Runtime/World/WorldNodes.cpp` | 📦 | ⬜ |
| Expeditions | engine/world/WorldGraph.cpp | `Runtime/World/WorldGraph.cpp` | 📦 | ⬜ |
| Expeditions | engine/world/WorldStreamer.cpp | `Runtime/World/WorldStreamer.cpp` | 📦 | ⬜ |
| Expeditions | engine/world/HeightfieldMesher.cpp | `Runtime/World/HeightfieldMesher.cpp` | 📦 | ⬜ |
| sdltoast | src/world/Tile.h/.cpp | `Runtime/World/Tile.h/.cpp` | 📦 | ⬜ |
| sdltoast | src/world/Map.cpp | `Runtime/World/Map.cpp` | 📦 | ⬜ |
| sdltoast | src/world/WorldGenerator.cpp | `Runtime/World/WorldGenerator.cpp` | 📦 | ⬜ |
| Expeditions | engine/tile/TileGraph.h/.cpp | `Runtime/World/TileGraph.h/.cpp` | 📦 | ⬜ |
| Expeditions | engine/tile/TileChunkBuilder.h | `Runtime/World/TileChunkBuilder.h` | 📦 | ⬜ |
| **Combat & Weapons** | | | | |
| Atlas | cpp_server/systems/combat_system.cpp | `Runtime/Systems/CombatSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/weapon_system.cpp | `Runtime/Systems/WeaponSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/targeting_system.cpp | `Runtime/Systems/TargetingSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/drone_system.cpp | `Runtime/Systems/DroneSystem.cpp` | 📦 | ⬜ |
| sdltoast | src/systems/Combat.cpp | *(merge into Runtime/Systems/CombatSystem)* | 📦 | ⬜ |
| Expeditions | engine/weapon/WeaponNodes.h/.cpp | `Runtime/Systems/WeaponNodes.h/.cpp` | 📦 | ⬜ |
| **Inventory, Loot & Crafting** | | | | |
| Atlas | cpp_server/systems/inventory_system.cpp | `Runtime/Inventory/InventorySystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/loot_system.cpp | `Runtime/Inventory/LootSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/wreck_salvage_system.cpp | `Runtime/Inventory/WreckSalvageSystem.cpp` | 📦 | ⬜ |
| sdltoast | src/systems/Crafting.cpp | `Runtime/Crafting/Crafting.cpp` | 📦 | ⬜ |
| sdltoast | src/systems/Mining.h | `Runtime/Crafting/Mining.h` | 📦 | ⬜ |
| **Ship & Equipment** | | | | |
| Atlas | cpp_server/systems/ship_fitting_system.cpp | `Runtime/Equipment/ShipFittingSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/capacitor_system.cpp | `Runtime/Equipment/CapacitorSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/operational_wear_system.cpp | `Runtime/Equipment/OperationalWearSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/insurance_system.cpp | `Runtime/Equipment/InsuranceSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/rendering/ship_part_library.cpp | `Runtime/Equipment/ShipPartLibrary.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/rendering/ship_generation_rules.cpp | `Runtime/Equipment/ShipGenerationRules.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/rendering/damage_effect_helper.cpp | `Runtime/Damage/DamageEffectHelper.cpp` | 📦 | ⬜ |
| **Mission & Economy** | | | | |
| Atlas | cpp_server/systems/mission_system.cpp | `Runtime/Systems/MissionSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/mission_generator_system.cpp | `Runtime/Systems/MissionGeneratorSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/trade_flow_system.cpp | `Runtime/Systems/TradeFlowSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/bounty_system.cpp | `Runtime/Systems/BountySystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/research_system.cpp | `Runtime/Systems/ResearchSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/skill_system.cpp | `Runtime/Systems/SkillSystem.cpp` | 📦 | ⬜ |
| Expeditions | engine/gameplay/SkillTree.h | `Runtime/Systems/SkillTree.h` | 📦 | ⬜ |
| **NPC & Social** | | | | |
| Atlas | cpp_server/systems/npc_archetype_system.cpp | `Runtime/Systems/NPCArchetypeSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/npc_intent_system.cpp | `Runtime/Systems/NPCIntentSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/captain_personality_system.cpp | `Runtime/Systems/CaptainPersonalitySystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/captain_memory_system.cpp | `Runtime/Systems/CaptainMemorySystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/emotional_arc_system.cpp | `Runtime/Systems/EmotionalArcSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/fleet_norm_system.cpp | `Runtime/Systems/FleetNormSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/fleet_cargo_system.cpp | `Runtime/Systems/FleetCargoSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/rumor_propagation_system.cpp | `Runtime/Systems/RumorPropagationSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/local_reputation_system.cpp | `Runtime/Systems/LocalReputationSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/corporation_system.cpp | `Runtime/Systems/CorporationSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/leaderboard_system.cpp | `Runtime/Systems/LeaderboardSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/chat_system.cpp | `Runtime/Systems/ChatSystem.cpp` | 📦 | ⬜ |
| **Movement & Space** | | | | |
| Atlas | cpp_server/systems/movement_system.cpp | `Runtime/Systems/MovementSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/warp_anomaly_system.cpp | `Runtime/Systems/WarpAnomalySystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/warp_cinematic_system.cpp | `Runtime/Systems/WarpCinematicSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/sector_tension_system.cpp | `Runtime/Systems/SectorTensionSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/star_system_state_system.cpp | `Runtime/Systems/StarSystemStateSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/galactic_response_system.cpp | `Runtime/Systems/GalacticResponseSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/lod_culling_system.cpp | `Runtime/Systems/LODCullingSystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/pi_system.cpp | `Runtime/Systems/PISystem.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/systems/background_simulation_system.cpp | `Runtime/Systems/BackgroundSimulationSystem.cpp` | 📦 | ⬜ |
| **Player & Entities** | | | | |
| sdltoast | src/entities/Player.h/.cpp | `Runtime/Player/Player.h/.cpp` | 📦 | ⬜ |
| sdltoast | src/entities/NPC.h/.cpp | `Runtime/Player/NPC.h/.cpp` | 📦 | ⬜ |
| sdltoast | src/entities/Enemy.cpp | `Runtime/Player/Enemy.cpp` | 📦 | ⬜ |
| **Misc Gameplay** | | | | |
| sdltoast | src/systems/Farming.cpp | `Runtime/Systems/FarmingSystem.cpp` | 📦 | ⬜ |
| sdltoast | src/systems/Fishing.cpp | `Runtime/Systems/FishingSystem.cpp` | 📦 | ⬜ |
| sdltoast | src/systems/AnimalHusbandry.cpp | `Runtime/Systems/AnimalHusbandrySystem.cpp` | 📦 | ⬜ |
| sdltoast | src/systems/Energy.h | `Runtime/Systems/EnergySystem.h` | 📦 | ⬜ |
| sdltoast | src/systems/SaveSystem.h | `Runtime/SaveLoad/SaveSystem.h` | 📦 | ⬜ |
| **Server Infrastructure** | | | | |
| Atlas | cpp_server/server.cpp | `Runtime/Server/Server.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/game_session.cpp | `Runtime/Server/GameSession.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/network/protocol_handler.cpp | `Runtime/Server/ProtocolHandler.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/config/server_config.cpp | `Runtime/Server/ServerConfig.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/auth/steam_auth.cpp | `Runtime/Server/SteamAuth.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/utils/logger.cpp | *(merge into Engine/Core/Logger)* | 📦 | ⬜ |
| Atlas | cpp_server/data/ship_database.cpp | `Runtime/Data/ShipDatabase.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/data/world_persistence.cpp | `Runtime/Data/WorldPersistence.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/data/npc_database.cpp | `Runtime/Data/NPCDatabase.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/core/embedded_server.cpp | `Runtime/Server/EmbeddedServer.cpp` | 📦 | ⬜ |
| **Interaction** | | | | |
| Expeditions | engine/interaction/Interaction.cpp | `Runtime/Interaction/Interaction.cpp` | 📦 | ⬜ |
| Expeditions | engine/interaction/RuleIntentResolver.cpp | `Runtime/Interaction/RuleIntentResolver.cpp` | 📦 | ⬜ |
| Expeditions | engine/interaction/InteractionRouter.h | `Runtime/Interaction/InteractionRouter.h` | 📦 | ⬜ |
| **Character** | | | | |
| Expeditions | engine/character/CharacterNodes.cpp | `Runtime/Player/CharacterNodes.cpp` | 📦 | ⬜ |

---

### Editor/ — Development Tool

| Source Repo | Source File | → MasterRepo Path | Archive | Extract |
|---|---|---|---|---|
| NovaForge | editor/main.cpp | `Editor/main.cpp` | 📦 | ✅ |
| Expeditions | engine/tools/EditorCommandBus.h/.cpp | `Editor/Commands/EditorCommandBus.h/.cpp` | 📦 | ⬜ |
| Expeditions | engine/tools/EditorToolRegistry.h | `Editor/Core/EditorToolRegistry.h` | 📦 | ⬜ |
| Expeditions | engine/tools/MaterialOverrideTool.h | `Editor/Modes/MaterialOverrideTool.h` | 📦 | ⬜ |
| Expeditions | engine/tools/ToolingLayer.cpp | `Editor/Core/ToolingLayer.cpp` | 📦 | ⬜ |
| Expeditions | engine/tools/LayerTagSystem.h | `Editor/Core/LayerTagSystem.h` | 📦 | ⬜ |
| Expeditions | engine/tools/EntityInspectorTool.h | `Editor/Panels/EntityInspectorTool.h` | 📦 | ⬜ |

---

### AI/ — Intelligence Pipeline

| Source Repo | Source File | → MasterRepo Path | Archive | Extract |
|---|---|---|---|---|
| Expeditions | engine/ai/AtlasAICore.cpp | `AI/Core/AtlasAICore.cpp` | 📦 | ⬜ |
| Expeditions | engine/ai/LLMBackend.cpp | `AI/Models/LLMBackend.cpp` | 📦 | ⬜ |
| Expeditions | engine/ai/AIAssetDecisionFramework.h | `AI/AssetLearning/AIAssetDecisionFramework.h` | 📦 | ⬜ |
| Expeditions | engine/ai/ProjectContext.h | `AI/Context/ProjectContext.h` | 📦 | ⬜ |
| Expeditions | engine/sim/AIMinerStateMachine.h/.cpp | `AI/AgentScheduler/AIMinerStateMachine.h/.cpp` | 📦 | ⬜ |
| Expeditions | engine/sim/AIWalletSystem.h/.cpp | `AI/AgentScheduler/AIWalletSystem.h/.cpp` | 📦 | ⬜ |
| Expeditions | engine/sim/SimulationStateAuditor.cpp | `AI/AgentScheduler/SimulationStateAuditor.cpp` | 📦 | ⬜ |

---

### PCG/ — Procedural Content Generation

| Source Repo | Source File | → MasterRepo Path | Archive | Extract |
|---|---|---|---|---|
| Expeditions | engine/procedural/ProceduralMeshNodes.cpp | `PCG/Geometry/ProceduralMeshNodes.cpp` | 📦 | ⬜ |
| Expeditions | engine/procedural/HullMeshGenerator.cpp | `PCG/Geometry/HullMeshGenerator.cpp` | 📦 | ⬜ |
| Expeditions | engine/procedural/InteriorNode.h | `PCG/Geometry/InteriorNode.h` | 📦 | ⬜ |
| Expeditions | engine/procedural/LODBakingGraph.cpp | `PCG/Geometry/LODBakingGraph.cpp` | 📦 | ⬜ |
| Expeditions | engine/procedural/BuildQueue.h | `PCG/Geometry/BuildQueue.h` | 📦 | ⬜ |
| Atlas | cpp_client/rendering/asteroid_field_renderer.cpp | `PCG/World/AsteroidFieldRenderer.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/rendering/station_renderer.cpp | `PCG/World/StationRenderer.cpp` | 📦 | ⬜ |

---

### UI/ — Shared UI Framework

| Source Repo | Source File | → MasterRepo Path | Archive | Extract |
|---|---|---|---|---|
| Expeditions | engine/ui/UIScreenGraph.h | `UI/Layouts/UIScreenGraph.h` | 📦 | ⬜ |
| Expeditions | engine/ui/UIGraph.cpp | `UI/Layouts/UIGraph.cpp` | 📦 | ⬜ |
| Expeditions | engine/ui/UILogicGraph.h | `UI/Layouts/UILogicGraph.h` | 📦 | ⬜ |
| Expeditions | engine/ui/UILayoutSolver.h | `UI/Layouts/UILayoutSolver.h` | 📦 | ⬜ |
| Expeditions | engine/ui/UIStyle.h | `UI/Themes/UIStyle.h` | 📦 | ⬜ |
| Expeditions | engine/ui/TextRenderer.h | `UI/Widgets/TextRenderer.h` | 📦 | ⬜ |
| Expeditions | engine/ui/MenuManager.cpp | `UI/Widgets/MenuManager.cpp` | 📦 | ⬜ |
| Expeditions | engine/ui/GUIInputRecorder.cpp | `UI/Widgets/GUIInputRecorder.cpp` | 📦 | ⬜ |
| Expeditions | engine/ui/GUIDSLParser.cpp | `UI/Widgets/GUIDSLParser.cpp` | 📦 | ⬜ |
| Expeditions | engine/ui/DiagnosticsOverlay.h | `UI/Widgets/DiagnosticsOverlay.h` | 📦 | ⬜ |
| sdltoast | src/ui/DialogueBox.h/.cpp | `UI/Widgets/DialogueBox.h/.cpp` | 📦 | ⬜ |
| sdltoast | src/ui/Menu.cpp | `UI/Widgets/Menu.cpp` | 📦 | ⬜ |
| sdltoast | src/ui/HUD.h | `UI/Widgets/HUD.h` | 📦 | ⬜ |
| Atlas | cpp_client/ui/radial_menu.cpp | `UI/Widgets/RadialMenu.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/ui/layout_manager.cpp | `UI/Layouts/LayoutManager.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/ui/rml_ui_manager.cpp | `UI/Widgets/RmlUIManager.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/ui/star_map.cpp | `UI/Widgets/StarMap.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/rendering/healthbar_renderer.cpp | `UI/Widgets/HealthbarRenderer.cpp` | 📦 | ⬜ |

---

### Projects/NovaForge/ — Game-Specific Code

| Source Repo | Source File | → MasterRepo Path | Archive | Extract |
|---|---|---|---|---|
| Atlas | cpp_client/ui/atlas/atlas_console.cpp | `Projects/NovaForge/UI/AtlasConsole.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/ui/atlas/atlas_widgets.cpp | `Projects/NovaForge/UI/AtlasWidgets.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/ui/atlas/atlas_pause_menu.cpp | `Projects/NovaForge/UI/AtlasPauseMenu.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/ui/atlas/atlas_title_screen.cpp | `Projects/NovaForge/UI/AtlasTitleScreen.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/ui/atlas/atlas_hud.cpp | `Projects/NovaForge/UI/AtlasHUD.cpp` | 📦 | ⬜ |
| Expeditions | engine/ui/atlas/AtlasUIPanels.cpp | `Projects/NovaForge/UI/AtlasUIPanels.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/rendering/reference_model_analyzer.cpp | `Projects/NovaForge/Assets/ReferenceModelAnalyzer.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/core/application.cpp | `Projects/NovaForge/Application.cpp` | 📦 | ⬜ |
| Atlas | cpp_client/core/file_logger.cpp | *(merge into Engine/Core/Logger)* | 📦 | ⬜ |
| Atlas | cpp_client/main.cpp | `Projects/NovaForge/ClientMain.cpp` | 📦 | ⬜ |
| Atlas | cpp_server/main.cpp | `Projects/NovaForge/ServerMain.cpp` | 📦 | ⬜ |
| sdltoast | src/main.cpp | `Projects/NovaForge/SdlMain.cpp` | 📦 | ⬜ |
| Expeditions | engine/sim/AlertStack.cpp | `Projects/NovaForge/Sim/AlertStack.cpp` | 📦 | ⬜ |

---

### Tools/ & Builder/ — Production Pipeline

| Source Repo | Source File | → MasterRepo Path | Archive | Extract |
|---|---|---|---|---|
| Expeditions | engine/production/AssetCooker.cpp | `Tools/AssetTools/AssetCooker.cpp` | 📦 | ⬜ |
| Expeditions | engine/production/BuildManifest.cpp | `Tools/BuildTools/BuildManifest.cpp` | 📦 | ⬜ |
| Expeditions | engine/production/BuildAuditLog.h/.cpp | `Tools/BuildTools/BuildAuditLog.h/.cpp` | 📦 | ⬜ |

---

## 4. Refactoring Notes

### Namespace Conversion
All extracted files must be re-namespaced to MasterRepo conventions:

| Module | Namespace |
|--------|-----------|
| Core/ | `Core::` |
| Engine/ | `Engine::` |
| Runtime/ | `Runtime::` |
| Editor/ | `Editor::` |
| AI/ | `AI::` |
| Builder/ | `Builder::` |
| PCG/ | `PCG::` |
| UI/ | `UI::` |

### Include Path Convention
All `#include` directives must use root-relative paths:
```cpp
#include "Core/Events/Event.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Runtime/ECS/ECS.h"
```

### Atlas Rule — No ImGui
All UI must be custom-built. No ImGui dependency anywhere. The custom UI framework lives under `UI/` and uses `GUIDSLParser`, `UILayoutSolver`, and `UIGraph` from Expeditions as the foundation.

### Duplicate File Audit
Several files exist in multiple source repos and need auditing to pick the best version:

| File | Appears In | Resolution |
|------|-----------|------------|
| ECS.h/.cpp | NovaForge, Expeditions | Take NovaForge base, merge Expeditions improvements |
| System.h/.cpp | NovaForge, Expeditions | Take NovaForge base, merge Expeditions improvements |
| DeltaEditStore.h/.cpp | NovaForge, Expeditions | Take NovaForge base, merge Expeditions improvements |
| InputManager | NovaForge, Expeditions, sdltoast, Atlas | Merge all into single Engine/Input |
| AudioEngine / AudioManager | NovaForge, Atlas, sdltoast | Merge into single Engine/Audio |
| Logger | NovaForge, Atlas (server), Atlas (client), sdltoast | Merge into single Engine/Core/Logger |
| Camera | NovaForge, Atlas | Take NovaForge base, merge Atlas 3D features |
| TickScheduler | NovaForge, Expeditions | Audit both versions |

### Game-Specific vs Shared
- **Shared systems** (combat, inventory, crafting, movement) → `Runtime/Systems/`
- **Game-specific** data (Eve-style economy, captain personalities, fleet norms) → `Projects/NovaForge/` or `Runtime/Systems/` depending on reusability
- **Sim AI state machines** (AIMinerStateMachine, AIWalletSystem) → `AI/AgentScheduler/`
- **Procedural content** (hull mesh, interior nodes) → `PCG/Geometry/`

---

## 5. Priority Order

| Phase | Target | Goal | Status |
|-------|--------|------|--------|
| **1** | Core systems (events, messaging, reflection) | FOUNDATION | ✅ Done |
| **2** | Engine basics (window, input, platform) | HELLO WINDOW | ⬜ Not Started |
| **3** | Runtime/ECS | ENTITY SYSTEM | ✅ Done (base ECS) |
| **4** | Rendering pipeline | VISUAL OUTPUT | ⬜ Not Started |
| **5** | Editor framework | DEVELOPMENT TOOL | 🔄 Entry point only |
| **6** | Game systems (combat, inventory, missions) | GAMEPLAY | ⬜ Not Started |
| **7** | AI pipeline (LLM, agents, asset learning) | INTELLIGENCE | ⬜ Not Started |
| **8** | PCG pipeline (mesh gen, hull gen, LOD) | CONTENT | ⬜ Not Started |

---

## 6. Progress Summary

| Category | Total Files | Archived | Extracted & Refactored | Remaining |
|----------|------------|----------|----------------------|-----------|
| Core/ | ~10 | 📦 All source | ✅ 7 (Events, Messaging, Reflection, GameState) | ⬜ 3+ |
| Engine/ | ~50 | 📦 All source | ✅ 16 (Core, Physics, Audio, Camera, Input, Anim, Sim) | ⬜ 34+ |
| Runtime/ | ~80 | 📦 All source | ✅ 8 (ECS, World layouts) | ⬜ 72+ |
| Editor/ | ~7 | 📦 All source | ✅ 1 (main.cpp) | ⬜ 6 |
| AI/ | ~7 | 📦 All source | ⬜ 0 | ⬜ 7 |
| PCG/ | ~7 | 📦 All source | ⬜ 0 | ⬜ 7 |
| UI/ | ~18 | 📦 All source | ⬜ 0 | ⬜ 18 |
| Projects/ | ~13 | 📦 All source | ⬜ 0 | ⬜ 13 |
| Tools/ | ~3 | 📦 All source | ⬜ 0 | ⬜ 3 |
| **Total** | **~195** | **245 archived** | **✅ 32** | **⬜ ~163** |

> **Next action:** Phase 2 — Extract and refactor window/platform layer from Expeditions (Win32Window, X11Window) into `Engine/Platform/` and `Engine/Window/`.

---

## 5. Standalone NovaForge Merge (shifty81/NovaForge)

> Source: https://github.com/shifty81/NovaForge
> Merge initiated: 2026-03-26

### Completed

| Component | Status | MasterRepo Path |
|-----------|--------|----------------|
| Universe data (Thyrkstad, Solari, Duskfall, Aurendis) | ✅ | `Projects/NovaForge/Data/Universe/systems.json` |
| Gas types catalog | ✅ | `Projects/NovaForge/Data/gas_types.json` |
| Faction definitions (5 factions) | ✅ | `Projects/NovaForge/Data/factions.json` |
| UniverseData C++ structures | ✅ | `Runtime/Universe/UniverseData.h/.cpp` |
| UniverseLoader (ECS population) | ✅ | `Runtime/Universe/UniverseLoader.h` |
| FactionSystem wired to universe | ✅ | `Projects/NovaForge/main.cpp` |
| AI chat project-aware responses | ✅ | `Editor/Render/EditorRenderer.cpp` |
| Design Bible | ✅ | `Docs/NovaForge/DESIGN_BIBLE.md` |

### Pending (future sessions)

| Component | Status | Notes |
|-----------|--------|-------|
| Ship database (102 ships JSON) | ⬜ | `data/ships/*.json` from standalone |
| Module database (159 modules JSON) | ⬜ | `data/modules/` from standalone |
| Skills system (137 skills) | ⬜ | `data/skills/` from standalone |
| Warp mechanic cinematic | ⬜ | `data/universe/warp_mechanics.json` |
| NPC faction AI | ⬜ | `data/npcs/` from standalone |
| Planet surface PCG | ⬜ | Interior = exterior PCG parity |
| Ship builder modular UI | ⬜ | Drag-drop snap-grid builder |
| Multi-deck interior generation | ⬜ | Belly bay + rover + drone bay |
