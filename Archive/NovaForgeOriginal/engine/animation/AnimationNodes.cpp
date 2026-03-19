#include "AnimationNodes.h"
#include <algorithm>
#include <cmath>

namespace atlas::animation {

static constexpr float kPi = 3.14159265358979323846f;

// --- ClipNode ---

std::vector<AnimPort> ClipNode::Inputs() const {
    return {{"Time", AnimPinType::Float}};
}

std::vector<AnimPort> ClipNode::Outputs() const {
    return {{"Pose", AnimPinType::Pose}};
}

void ClipNode::Evaluate(const AnimContext& /*ctx*/, const std::vector<AnimValue>& inputs, std::vector<AnimValue>& outputs) const {
    float time = 0.0f;
    if (!inputs.empty() && !inputs[0].data.empty()) {
        time = inputs[0].data[0];
    }

    // Normalize time to [0, 1] range within clip
    float safeLength = (clipLength > 0.0f) ? clipLength : 1.0f;
    float normalizedTime = std::fmod(time, safeLength) / safeLength;

    // Generate a simple procedural pose: 4 bone transforms as (x, y, z, rotation)
    outputs.resize(1);
    outputs[0].type = AnimPinType::Pose;
    outputs[0].data = {
        normalizedTime,                                  // bone 0 x
        std::sin(normalizedTime * kPi),             // bone 0 y
        0.0f,                                            // bone 0 z
        normalizedTime * 360.0f,                         // bone 0 rotation
        -normalizedTime,                                 // bone 1 x
        std::cos(normalizedTime * kPi),             // bone 1 y
        0.0f,                                            // bone 1 z
        normalizedTime * 180.0f                          // bone 1 rotation
    };
}

// --- BlendNode ---

std::vector<AnimPort> BlendNode::Inputs() const {
    return {
        {"PoseA", AnimPinType::Pose},
        {"PoseB", AnimPinType::Pose},
        {"Weight", AnimPinType::Float}
    };
}

std::vector<AnimPort> BlendNode::Outputs() const {
    return {{"Pose", AnimPinType::Pose}};
}

void BlendNode::Evaluate(const AnimContext& /*ctx*/, const std::vector<AnimValue>& inputs, std::vector<AnimValue>& outputs) const {
    float weight = 0.5f;
    if (inputs.size() > 2 && !inputs[2].data.empty()) {
        weight = std::clamp(inputs[2].data[0], 0.0f, 1.0f);
    }

    outputs.resize(1);
    outputs[0].type = AnimPinType::Pose;

    const auto& poseA = (inputs.size() > 0) ? inputs[0].data : std::vector<float>{};
    const auto& poseB = (inputs.size() > 1) ? inputs[1].data : std::vector<float>{};

    size_t maxSize = std::max(poseA.size(), poseB.size());
    outputs[0].data.resize(maxSize, 0.0f);

    for (size_t i = 0; i < maxSize; ++i) {
        float a = (i < poseA.size()) ? poseA[i] : 0.0f;
        float b = (i < poseB.size()) ? poseB[i] : 0.0f;
        outputs[0].data[i] = a * (1.0f - weight) + b * weight;
    }
}

// --- ModifierNode ---

std::vector<AnimPort> ModifierNode::Inputs() const {
    return {
        {"Pose", AnimPinType::Pose},
        {"Modifier", AnimPinType::Modifier}
    };
}

std::vector<AnimPort> ModifierNode::Outputs() const {
    return {{"Pose", AnimPinType::Pose}};
}

void ModifierNode::Evaluate(const AnimContext& ctx, const std::vector<AnimValue>& inputs, std::vector<AnimValue>& outputs) const {
    outputs.resize(1);
    outputs[0].type = AnimPinType::Pose;

    // Copy input pose
    if (!inputs.empty() && !inputs[0].data.empty()) {
        outputs[0].data = inputs[0].data;
    } else {
        outputs[0].data = {0.0f, 0.0f, 0.0f, 0.0f};
    }

    // Get modifier intensity
    float intensity = 1.0f;
    if (inputs.size() > 1 && !inputs[1].data.empty()) {
        intensity = inputs[1].data[0];
    }

    // Apply modifier based on type and context
    switch (modifierType) {
        case ModifierType::Damage:
            // Damage adds tremor proportional to damageLevel
            for (size_t i = 0; i < outputs[0].data.size(); ++i) {
                outputs[0].data[i] += ctx.damageLevel * intensity * 0.1f;
            }
            break;
        case ModifierType::Skill:
            // Skill scales pose precision by speed
            for (size_t i = 0; i < outputs[0].data.size(); ++i) {
                outputs[0].data[i] *= (1.0f + ctx.speed * intensity * 0.05f);
            }
            break;
        case ModifierType::Emotion:
            // Emotion modulates by fatigue
            for (size_t i = 0; i < outputs[0].data.size(); ++i) {
                outputs[0].data[i] *= (1.0f - ctx.fatigue * intensity * 0.1f);
            }
            break;
    }
}

// --- StateMachineNode ---

std::vector<AnimPort> StateMachineNode::Inputs() const {
    return {
        {"Trigger", AnimPinType::Trigger},
        {"BlendTime", AnimPinType::Float}
    };
}

std::vector<AnimPort> StateMachineNode::Outputs() const {
    return {{"Pose", AnimPinType::Pose}};
}

void StateMachineNode::Evaluate(const AnimContext& ctx, const std::vector<AnimValue>& inputs, std::vector<AnimValue>& outputs) const {
    float trigger = 0.0f;
    if (!inputs.empty() && !inputs[0].data.empty()) {
        trigger = inputs[0].data[0];
    }

    float blendTime = 0.2f;
    if (inputs.size() > 1 && !inputs[1].data.empty()) {
        blendTime = inputs[1].data[0];
    }

    // State index derived from trigger value
    int stateIndex = static_cast<int>(trigger) % 4;
    float phase = ctx.deltaTime * (1.0f + static_cast<float>(stateIndex) * 0.5f);

    outputs.resize(1);
    outputs[0].type = AnimPinType::Pose;
    outputs[0].data = {
        phase,
        std::sin(phase * kPi) * (1.0f - blendTime),
        0.0f,
        phase * 90.0f * (stateIndex + 1)
    };
}

}
