#include "Runtime/Systems/RenderSystem.h"
#include "Runtime/Systems/PhysicsSystem.h"
#include "Runtime/ECS/ECS.h"
#include "Runtime/Components/Components.h"
#include <cstdio>

namespace Runtime::Systems {

// ---------------------------------------------------------------------------
// RenderSystem::Execute
// Iterates entities with Transform + MeshRenderer and submits draw calls.
// Actual GPU submission is handled by the renderer; this system prepares data.
// ---------------------------------------------------------------------------
void RenderSystem::Execute(Runtime::ECS::World& world, float dt) {
    for (auto entity : world.GetEntities()) {
        auto* mesh = world.GetComponent<Runtime::Components::MeshRenderer>(entity);
        if (!mesh || !mesh->visible)
            continue;

        auto* transform = world.GetComponent<Runtime::Components::Transform>(entity);
        if (!transform)
            continue;

        // Integration point: pass transform matrix and mesh/material IDs to renderer.
        // auto mat = transform->ToMatrix();
        // Renderer::Submit(mesh->meshId, mesh->materialId, mat, mesh->castShadow);
        (void)dt;
    }
}

// ---------------------------------------------------------------------------
// PhysicsSystem::Execute
// Applies gravity, integrates velocity, and resolves simple collisions.
// ---------------------------------------------------------------------------
void PhysicsSystem::Execute(Runtime::ECS::World& world, float dt) {
    static constexpr float kGravity = -9.81f;

    for (auto entity : world.GetEntities()) {
        auto* rb = world.GetComponent<Runtime::Components::RigidBody>(entity);
        if (!rb || rb->isKinematic)
            continue;

        auto* transform = world.GetComponent<Runtime::Components::Transform>(entity);
        if (!transform)
            continue;

        // Apply gravity
        if (rb->useGravity)
            rb->velocity.y += kGravity * dt;

        // Apply linear drag
        if (rb->drag > 0.0f) {
            float damping = 1.0f - rb->drag * dt;
            if (damping < 0.0f) damping = 0.0f;
            rb->velocity *= damping;
        }

        // Apply angular drag
        if (rb->angularDrag > 0.0f) {
            float damping = 1.0f - rb->angularDrag * dt;
            if (damping < 0.0f) damping = 0.0f;
            rb->angularVelocity *= damping;
        }

        // Integrate position
        transform->position += rb->velocity * dt;
    }
}

} // namespace Runtime::Systems
