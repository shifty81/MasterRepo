#include "Animation/IK/IKSolver.h"
#include <cmath>

namespace NF {

static float Vec3Length(const Vector3& v) noexcept {
    return std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
}

static Vector3 Vec3Lerp(const Vector3& a, const Vector3& b, float t) noexcept {
    return {a.X + t * (b.X - a.X),
            a.Y + t * (b.Y - a.Y),
            a.Z + t * (b.Z - a.Z)};
}

void IKSolver::SetChain(std::vector<Vector3> joints) {
    m_Joints = std::move(joints);
    m_SegmentLengths.clear();

    for (std::size_t i = 1; i < m_Joints.size(); ++i) {
        Vector3 diff{m_Joints[i].X - m_Joints[i - 1].X,
                     m_Joints[i].Y - m_Joints[i - 1].Y,
                     m_Joints[i].Z - m_Joints[i - 1].Z};
        m_SegmentLengths.push_back(Vec3Length(diff));
    }
}

void IKSolver::Solve(const Vector3& target, int maxIterations) {
    const std::size_t n = m_Joints.size();
    if (n < 2) return;

    const Vector3 root = m_Joints[0];

    // Total chain length.
    float totalLength = 0.f;
    for (float l : m_SegmentLengths) totalLength += l;

    // Distance from root to target.
    Vector3 rootToTarget{target.X - root.X,
                         target.Y - root.Y,
                         target.Z - root.Z};
    const float dist = Vec3Length(rootToTarget);

    // Target is unreachable — stretch chain toward target.
    if (dist >= totalLength) {
        for (std::size_t i = 0; i < n - 1; ++i) {
            Vector3 d{target.X - m_Joints[i].X,
                      target.Y - m_Joints[i].Y,
                      target.Z - m_Joints[i].Z};
            const float len = Vec3Length(d);
            const float t   = (len > 0.f) ? m_SegmentLengths[i] / len : 0.f;
            m_Joints[i + 1] = Vec3Lerp(m_Joints[i], target, t);
        }
        return;
    }

    // FABRIK iterations.
    constexpr float kTolerance = 1e-4f;
    for (int iter = 0; iter < maxIterations; ++iter) {
        // --- Forward pass: pull end-effector to target ---
        m_Joints[n - 1] = target;
        for (std::size_t i = n - 2; ; --i) {
            Vector3 d{m_Joints[i].X - m_Joints[i + 1].X,
                      m_Joints[i].Y - m_Joints[i + 1].Y,
                      m_Joints[i].Z - m_Joints[i + 1].Z};
            const float len = Vec3Length(d);
            const float t   = (len > 0.f) ? m_SegmentLengths[i] / len : 0.f;
            m_Joints[i] = Vec3Lerp(m_Joints[i + 1], m_Joints[i], t);
            if (i == 0) break;
        }

        // --- Backward pass: re-anchor root ---
        m_Joints[0] = root;
        for (std::size_t i = 0; i < n - 1; ++i) {
            Vector3 d{m_Joints[i + 1].X - m_Joints[i].X,
                      m_Joints[i + 1].Y - m_Joints[i].Y,
                      m_Joints[i + 1].Z - m_Joints[i].Z};
            const float len = Vec3Length(d);
            const float t   = (len > 0.f) ? m_SegmentLengths[i] / len : 0.f;
            m_Joints[i + 1] = Vec3Lerp(m_Joints[i], m_Joints[i + 1], t);
        }

        // Check convergence.
        const Vector3& ee = m_Joints[n - 1];
        Vector3 diff{ee.X - target.X, ee.Y - target.Y, ee.Z - target.Z};
        if (Vec3Length(diff) < kTolerance) break;
    }
}

} // namespace NF
