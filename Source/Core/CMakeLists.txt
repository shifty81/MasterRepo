add_library(NFCore STATIC
    Memory/Allocator.cpp
    Logging/Log.cpp
    Threading/TaskSystem.cpp
    Events/EventBus.cpp
    Reflection/TypeInfo.cpp
    Serialization/Archive.cpp
    Platform/Platform.cpp
    Config/ConfigSystem.cpp
    Config/ProjectManifest.cpp
    Paths/ProjectPathService.cpp
)

add_library(NF::Core ALIAS NFCore)

target_include_directories(NFCore PUBLIC
    ${CMAKE_SOURCE_DIR}/Source
)

target_compile_features(NFCore PUBLIC cxx_std_20)

message(STATUS "  NF::Core configured")
