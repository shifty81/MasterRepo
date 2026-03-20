add_library(BuilderLib STATIC
    Parts/PartDef.cpp
    Modules/ModuleDef.cpp
    Assembly/BuildQueue.cpp
    Assembly/Assembly.cpp
    SnapRules/SnapRules.cpp
    Collision/CollisionShape.cpp
    PhysicsData/PhysicsData.cpp
    Damage/PartDamage.cpp
    Core/BuildAuditLog.cpp
)
target_include_directories(BuilderLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(BuilderLib PUBLIC cxx_std_20)
