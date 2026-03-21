#include "Engine/Physics/PhysicsWorld.h"
#include <iostream>

int main() {
    Engine::Physics::PhysicsWorld world;
    world.Init();
    auto id = world.CreateBody(1.0f);
    auto* body = world.GetBody(id);
    if (body) {
        std::cout << "Initial position: (" << body->position.x << ", " << body->position.y << ", " << body->position.z << ")\n";
        world.SetVelocity(id, 0, 1, 0);
        world.Step(1.0f); // Simulate 1 second
        std::cout << "After 1s: (" << body->position.x << ", " << body->position.y << ", " << body->position.z << ")\n";
    }
    world.Shutdown();
    return 0;
}
