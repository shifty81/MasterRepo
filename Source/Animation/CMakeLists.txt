add_library(NFAnimation STATIC
    Skeleton/Skeleton.cpp
    BlendTree/BlendNode.cpp
    StateMachine/AnimStateMachine.cpp
    IK/IKSolver.cpp
)

add_library(NF::Animation ALIAS NFAnimation)

target_include_directories(NFAnimation PUBLIC ${CMAKE_SOURCE_DIR}/Source)
target_link_libraries(NFAnimation PUBLIC NF::Core NF::Engine)

message(STATUS "  NF::Animation configured")
