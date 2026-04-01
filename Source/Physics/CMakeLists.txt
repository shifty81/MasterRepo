add_library(NFPhysics STATIC
    Collision/CollisionDetection.cpp
    Dynamics/RigidBody.cpp
    Dynamics/PhysicsWorld.cpp
    Character/CharacterController.cpp
)

add_library(NF::Physics ALIAS NFPhysics)

target_include_directories(NFPhysics PUBLIC ${CMAKE_SOURCE_DIR}/Source)
target_link_libraries(NFPhysics PUBLIC NF::Core NF::Engine)

message(STATUS "  NF::Physics configured")
