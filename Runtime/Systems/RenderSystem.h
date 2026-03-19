#pragma once
#include "Runtime/ECS/System.h"

namespace Runtime::Systems {

class RenderSystem : public Runtime::ECS::ISystem {
public:
    const char* Name() const override { return "RenderSystem"; }
    int         Priority() const override { return 200; }
    void        Execute(Runtime::ECS::World& world, float dt) override;
};

} // namespace Runtime::Systems
