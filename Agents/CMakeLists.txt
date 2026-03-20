add_library(AgentsLib STATIC
    CodeAgent/CodeAgent.cpp
    CodeAgent/ToolSystem.cpp
)
target_include_directories(AgentsLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(AgentsLib PUBLIC cxx_std_20)
target_link_libraries(AgentsLib PUBLIC AILib)
