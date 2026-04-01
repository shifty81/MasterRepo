#include "Animation/StateMachine/AnimStateMachine.h"
#include <algorithm>

namespace NF {

void AnimStateMachine::AddState(AnimState state) {
    if (m_CurrentState.empty())
        m_CurrentState = state.Name;
    m_States.push_back(std::move(state));
}

void AnimStateMachine::AddTransition(AnimTransition transition) {
    m_Transitions.push_back(std::move(transition));
}

void AnimStateMachine::Update(float dt, Pose& outPose) {
    // Handle active blend-out to a new state.
    if (!m_NextState.empty()) {
        m_BlendTimer -= dt;
        if (m_BlendTimer <= 0.f) {
            m_CurrentState = m_NextState;
            m_NextState.clear();
            m_BlendTimer = 0.f;
        }
    }

    // Check transitions from the current state.
    if (m_NextState.empty()) {
        for (const auto& t : m_Transitions) {
            if (t.From == m_CurrentState && t.Condition && t.Condition()) {
                m_NextState  = t.To;
                m_BlendTimer = t.BlendTime;
                break;
            }
        }
    }

    // Evaluate the active state's blend tree.
    const std::string& active = m_NextState.empty() ? m_CurrentState : m_NextState;
    auto it = std::find_if(m_States.begin(), m_States.end(),
                           [&active](const AnimState& s) { return s.Name == active; });
    if (it != m_States.end() && it->Root)
        it->Root->Evaluate(dt, outPose);
}

} // namespace NF
