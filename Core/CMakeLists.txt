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
    # Phase 23 — hierarchical config system
    Config/ConfigSystem.cpp
    # Phase 24 — script engine binding registry
    Scripting/LuaBinding.cpp
    # Phase 25 — typed event dispatcher
    EventDispatcher/EventDispatcher.cpp
    # Phase 26 — typed signal/slot (header-only, compiled stub)
    Signal/Signal.cpp
    # Phase 27 — fixed-capacity ring buffer (header-only, compiled stub)
    RingBuffer/RingBuffer.cpp
    # Phase 28 — fixed-size pool allocator (header-only, compiled stub)
    Allocator/PoolAllocator/PoolAllocator.cpp
    # Phase 29 — fixed-thread-count thread pool with priority queue
    Threading/ThreadPool/ThreadPool.cpp
)

target_compile_features(CoreLib PUBLIC cxx_std_20)

target_include_directories(CoreLib PUBLIC ${CMAKE_SOURCE_DIR})
