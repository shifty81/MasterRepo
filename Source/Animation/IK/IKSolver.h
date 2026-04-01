#pragma once
#include "Core/Math/Vector.h"
#include <vector>

namespace NF {

/// @brief FABRIK (Forward And Backward Reaching Inverse Kinematics) solver.
class IKSolver {
public:
    /// @brief Set the joint chain.  The first joint is the root, the last is the end-effector.
    /// @param joints  Initial world-space joint positions.
    void SetChain(std::vector<Vector3> joints);

    /// @brief Run the FABRIK algorithm to reach target.
    /// @param target        Desired end-effector position.
    /// @param maxIterations Maximum solver iterations (default: 10).
    void Solve(const Vector3& target, int maxIterations = 10);

    /// @brief Return the current joint positions after the last Solve call.
    [[nodiscard]] const std::vector<Vector3>& GetJoints() const noexcept { return m_Joints; }

private:
    std::vector<Vector3> m_Joints;
    std::vector<float>   m_SegmentLengths; ///< Lengths between consecutive joints.
};

} // namespace NF
