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
)

target_compile_features(CoreLib PUBLIC cxx_std_20)

target_include_directories(CoreLib PUBLIC ${CMAKE_SOURCE_DIR})
