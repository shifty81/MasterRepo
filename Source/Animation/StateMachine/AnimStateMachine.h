#pragma once
#include "Animation/BlendTree/BlendNode.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace NF {

/// @brief A named animation state that wraps a blend tree root.
struct AnimState {
    std::string                Name; ///< Unique state identifier.
    std::shared_ptr<BlendNode> Root; ///< Blend tree root for this state.
};

/// @brief A conditional transition between two named animation states.
struct AnimTransition {
    std::string          From;      ///< Source state name.
    std::string          To;        ///< Destination state name.
    std::function<bool()> Condition; ///< Returns true when the transition should fire.
    float                BlendTime; ///< Duration of the cross-fade in seconds.
};

/// @brief Finite-state machine that drives animation playback.
class AnimStateMachine {
public:
    /// @brief Register an animation state.
    void AddState(AnimState state);

    /// @brief Register a transition between two states.
    void AddTransition(AnimTransition transition);

    /// @brief Advance the state machine and write the active pose.
    /// @param dt       Time delta in seconds.
    /// @param outPose  Destination pose buffer.
    void Update(float dt, Pose& outPose);

    /// @brief Name of the currently active state (empty if none).
    [[nodiscard]] const std::string& GetCurrentState() const noexcept { return m_CurrentState; }

private:
    std::vector<AnimState>      m_States;
    std::vector<AnimTransition> m_Transitions;
    std::string                 m_CurrentState;
    float                       m_BlendTimer{0.f};
    std::string                 m_NextState;
};

} // namespace NF
