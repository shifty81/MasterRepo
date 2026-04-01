#include "Physics/Dynamics/PhysicsWorld.h"
#include <algorithm>
#include <limits>

namespace NF {

void PhysicsWorld::AddBody(RigidBody* body) {
    if (body && std::find(m_Bodies.begin(), m_Bodies.end(), body) == m_Bodies.end())
        m_Bodies.push_back(body);
}

void PhysicsWorld::RemoveBody(RigidBody* body) {
    m_Bodies.erase(std::remove(m_Bodies.begin(), m_Bodies.end(), body), m_Bodies.end());
}

void PhysicsWorld::Step(float dt) {
    for (RigidBody* body : m_Bodies)
        body->Integrate(dt);
}

bool PhysicsWorld::Raycast(const Vector3& origin, const Vector3& dir,
                           float maxDist, RaycastHit& hit) const {
    bool found = false;
    float nearest = maxDist;

    for (const RigidBody* body : m_Bodies) {
        const Vector3& pos = body->GetState().Position;
        // Represent each body as a unit AABB centred on its position.
        const AABB bounds{
            {pos.X - 0.5f, pos.Y - 0.5f, pos.Z - 0.5f},
            {pos.X + 0.5f, pos.Y + 0.5f, pos.Z + 0.5f}
        };

        RaycastHit candidate{};
        if (CollisionDetection::Raycast(origin, dir, nearest, bounds, candidate)) {
            nearest = candidate.Distance;
            hit     = candidate;
            found   = true;
        }
    }
    return found;
}

} // namespace NF
