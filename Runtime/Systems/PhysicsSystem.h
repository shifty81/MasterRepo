#pragma once
#include "Runtime/ECS/System.h"

namespace Runtime::Systems {

class PhysicsSystem : public Runtime::ECS::ISystem {
public:
    const char* Name() const override { return "PhysicsSystem"; }
    int         Priority() const override { return 50; }
    void        Execute(Runtime::ECS::World& world, float dt) override;
};

} // namespace Runtime::Systems
