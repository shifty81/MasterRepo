add_library(AgentsLib STATIC
    CodeAgent/CodeAgent.cpp
    CodeAgent/ToolSystem.cpp
    BuildAgent/BuildAgent.cpp
    DebugAgent/DebugAgent.cpp
    FixAgent/FixAgent.cpp
    RefactorAgent/RefactorAgent.cpp
    CleanupAgent/CleanupAgent.cpp
    EditorAgent/EditorAgent.cpp
    AssetAgent/AssetAgent.cpp
    PCGAgent/PCGAgent.cpp
    VersionAgent/VersionAgent.cpp
    SelfBuildAgent/SelfBuildAgent.cpp
)
target_include_directories(AgentsLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(AgentsLib PUBLIC cxx_std_20)
target_link_libraries(AgentsLib PUBLIC AILib)
