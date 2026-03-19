#pragma once
#include "Engine/Animation/AnimationGraph.h"

namespace Engine::Animation {

// Outputs a base pose from a time parameter
class ClipNode : public AnimNode {
public:
    float clipLength = 1.0f;

    const char* GetName() const override { return "Clip"; }
    const char* GetCategory() const override { return "Source"; }
    std::vector<AnimPort> Inputs() const override;
    std::vector<AnimPort> Outputs() const override;
    void Evaluate(const AnimContext& ctx, const std::vector<AnimValue>& inputs, std::vector<AnimValue>& outputs) const override;
} // namespace Engine::Animation;

// Blends two poses by weight
class BlendNode : public AnimNode {
public:
    const char* GetName() const override { return "Blend"; }
    const char* GetCategory() const override { return "Blend"; }
    std::vector<AnimPort> Inputs() const override;
    std::vector<AnimPort> Outputs() const override;
    void Evaluate(const AnimContext& ctx, const std::vector<AnimValue>& inputs, std::vector<AnimValue>& outputs) const override;
} // namespace Engine::Animation;

enum class ModifierType : uint8_t {
    Damage,
    Skill,
    Emotion
} // namespace Engine::Animation;

// Applies a modifier (e.g. limp, recoil, tremor) to a pose
class ModifierNode : public AnimNode {
public:
    ModifierType modifierType = ModifierType::Damage;

    const char* GetName() const override { return "Modifier"; }
    const char* GetCategory() const override { return "Modifier"; }
    std::vector<AnimPort> Inputs() const override;
    std::vector<AnimPort> Outputs() const override;
    void Evaluate(const AnimContext& ctx, const std::vector<AnimValue>& inputs, std::vector<AnimValue>& outputs) const override;
} // namespace Engine::Animation;

// Simple state transition node
class StateMachineNode : public AnimNode {
public:
    const char* GetName() const override { return "StateMachine"; }
    const char* GetCategory() const override { return "State"; }
    std::vector<AnimPort> Inputs() const override;
    std::vector<AnimPort> Outputs() const override;
    void Evaluate(const AnimContext& ctx, const std::vector<AnimValue>& inputs, std::vector<AnimValue>& outputs) const override;
} // namespace Engine::Animation;

} // namespace Engine::Animation
