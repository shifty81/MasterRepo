cmake_minimum_required(VERSION 3.20)

add_library(CoreLib STATIC
    Events/EventBus.cpp
    Messaging/MessageBus.cpp
    Reflection/TypeInfo.cpp
    Serialization/Serializer.cpp
    TaskSystem/TaskSystem.cpp
    CommandSystem/CommandSystem.cpp
    GameState/GameStateManager.cpp
    ResourceManager/ResourceManager.cpp
    PluginSystem/PluginSystem.cpp
    ArchiveSystem/ArchiveSystem.cpp
    VersionSystem/VersionSystem.cpp
    HotReload/HotReload.cpp
    CrashReport/CrashReport.cpp
    Database/Database.cpp
    LocalCIPipeline/LocalCIPipeline.cpp
    PackageManager/PackageManager.cpp
    NetworkProtocolGen/NetworkProtocolGen.cpp
    JobSystem/JobSystem.cpp
    CodeIntelligence/CodeIntelligence.cpp
    # Phase 19 — deterministic PCG seed manager
    DeterministicSeed/DeterministicSeed.cpp
    # Phase 20 — transaction-safe AI suggestion staging
    TransactionManager/TransactionManager.cpp
    # Phase 21 — persistent event/audit log
    EventLog/EventLog.cpp
)

target_compile_features(CoreLib PUBLIC cxx_std_20)

target_include_directories(CoreLib PUBLIC ${CMAKE_SOURCE_DIR})
