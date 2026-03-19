#include "CharacterAnimationController.h"
#include <cmath>

namespace atlas::animation {

CharacterAnimationController::CharacterAnimationController() {
    RebuildGraph();
    // Initialize pose to identity (idle)
    m_currentPose.resize(8, 0.0f);
}

float CharacterAnimationController::ClipLengthForState(CharacterAnimState state) {
    switch (state) {
        case CharacterAnimState::Idle:   return 2.0f;
        case CharacterAnimState::Walk:   return 1.0f;
        case CharacterAnimState::Run:    return 0.6f;
        case CharacterAnimState::Attack: return 0.4f;
        case CharacterAnimState::Hit:    return 0.3f;
        case CharacterAnimState::Death:  return 1.5f;
        default: return 1.0f;
    }
}

void CharacterAnimationController::RebuildGraph() {
    m_graph = AnimationGraph();

    auto clip = std::make_unique<ClipNode>();
    clip->clipLength = ClipLengthForState(m_state);
    m_clipNode = m_graph.AddNode(std::move(clip));

    m_blendNode = m_graph.AddNode(std::make_unique<BlendNode>());

    auto mod = std::make_unique<ModifierNode>();
    mod->modifierType = ModifierType::Damage;
    m_modNode = m_graph.AddNode(std::move(mod));

    // Clip -> Blend (port 0) -> Modifier
    m_graph.AddEdge({m_clipNode, 0, m_blendNode, 0});
    m_graph.AddEdge({m_blendNode, 0, m_modNode, 0});

    m_graph.Compile();
}

void CharacterAnimationController::SetState(CharacterAnimState state, float blendTime) {
    if (state == m_state) return;
    m_state = state;
    m_blendTime = blendTime;
    RebuildGraph();
}

void CharacterAnimationController::Update(float dt) {
    m_elapsed += dt;
    m_tick++;

    AnimContext ctx{};
    ctx.deltaTime   = dt;
    ctx.speed       = m_speed;
    ctx.fatigue     = m_fatigue;
    ctx.damageLevel = m_damage;
    ctx.tick        = m_tick;

    if (m_graph.Execute(ctx)) {
        if (auto* out = m_graph.GetOutput(m_modNode, 0)) {
            m_currentPose = out->data;
        }
    }
}

} // namespace atlas::animation
