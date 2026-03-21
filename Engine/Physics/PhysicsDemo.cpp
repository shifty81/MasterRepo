#include "Engine/Physics/PhysicsWorld.h"
#include <iostream>

int main() {
    Engine::Physics::PhysicsWorld world;
    world.Init();
    auto id = world.CreateBody(1.0f);
    auto* body = world.GetBody(id);
    if (body) {
        std::cout << "Created body with mass: " << body->mass << "\n";
    }
    world.Shutdown();
    return 0;
}