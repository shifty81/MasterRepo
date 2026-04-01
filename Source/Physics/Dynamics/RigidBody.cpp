#include "Physics/Dynamics/RigidBody.h"

namespace NF {

RigidBody::RigidBody(const RigidBodyState& state) noexcept
    : m_State(state) {}

void RigidBody::ApplyForce(const Vector3& force) noexcept {
    if (m_State.IsKinematic) return;
    m_AccumulatedForce += force;
}

void RigidBody::ApplyImpulse(const Vector3& impulse) noexcept {
    if (m_State.IsKinematic) return;
    if (m_State.Mass > 0.f)
        m_State.Velocity += impulse * (1.f / m_State.Mass);
}

void RigidBody::Integrate(float dt) noexcept {
    if (m_State.IsKinematic || dt <= 0.f) return;

    const float invMass = (m_State.Mass > 0.f) ? 1.f / m_State.Mass : 0.f;
    const Vector3 acceleration = m_AccumulatedForce * invMass;

    m_State.Velocity  += acceleration * dt;
    m_State.Position  += m_State.Velocity * dt;

    m_AccumulatedForce = {0.f, 0.f, 0.f};
}

} // namespace NF
