#include "WeaponNodes.h"
#include <algorithm>
#include <cmath>

namespace atlas::weapon {

// Hash mixing for seed-based variation
static float HashMix(uint64_t seed, uint32_t index) {
    uint32_t h = static_cast<uint32_t>(seed ^ (seed >> 32));
    h ^= index * 374761393u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return static_cast<float>(h & 0xFFFFu) / 65535.0f;
}

// --- ReceiverNode ---
// Component data layout: [damage, fireRate, accuracy, range]

std::vector<WeaponPort> ReceiverNode::Inputs() const {
    return {
        {"Seed", WeaponPinType::Seed}
    };
}

std::vector<WeaponPort> ReceiverNode::Outputs() const {
    return {{"Component", WeaponPinType::Component}};
}

void ReceiverNode::Evaluate(const WeaponContext& ctx, const std::vector<WeaponValue>& inputs, std::vector<WeaponValue>& outputs) const {
    uint64_t seed = ctx.seed;
    if (!inputs.empty() && !inputs[0].data.empty()) {
        seed = static_cast<uint64_t>(inputs[0].data[0]);
    }

    float variation = HashMix(seed, 0) * 0.2f - 0.1f; // ±10% variation

    outputs.resize(1);
    outputs[0].type = WeaponPinType::Component;
    outputs[0].data = {
        baseDamage * (1.0f + variation),    // damage
        baseFireRate * (1.0f - variation),  // fireRate
        0.7f + HashMix(seed, 1) * 0.3f,    // accuracy [0.7 - 1.0]
        50.0f + HashMix(seed, 2) * 50.0f   // range [50 - 100]
    };
}

// --- BarrelNode ---

std::vector<WeaponPort> BarrelNode::Inputs() const {
    return {
        {"Component", WeaponPinType::Component},
        {"Length", WeaponPinType::Float}
    };
}

std::vector<WeaponPort> BarrelNode::Outputs() const {
    return {{"Component", WeaponPinType::Component}};
}

void BarrelNode::Evaluate(const WeaponContext& /*ctx*/, const std::vector<WeaponValue>& inputs, std::vector<WeaponValue>& outputs) const {
    float damage = 10.0f, fireRate = 1.0f, accuracy = 0.7f, range = 50.0f;

    if (!inputs.empty() && inputs[0].data.size() >= 4) {
        damage = inputs[0].data[0];
        fireRate = inputs[0].data[1];
        accuracy = inputs[0].data[2];
        range = inputs[0].data[3];
    }

    float length = 1.0f;
    if (inputs.size() > 1 && !inputs[1].data.empty()) {
        length = inputs[1].data[0];
    }

    // Longer barrel: +accuracy, +range, slight -fireRate
    float lengthFactor = std::clamp(length, 0.5f, 2.0f);
    accuracy = std::clamp(accuracy * (0.9f + 0.1f * lengthFactor), 0.0f, 1.0f);
    range *= lengthFactor;
    fireRate *= (1.1f - 0.1f * lengthFactor);

    outputs.resize(1);
    outputs[0].type = WeaponPinType::Component;
    outputs[0].data = {damage, fireRate, accuracy, range};
}

// --- RecoilProfileNode ---

std::vector<WeaponPort> RecoilProfileNode::Inputs() const {
    return {
        {"Component", WeaponPinType::Component}
    };
}

std::vector<WeaponPort> RecoilProfileNode::Outputs() const {
    return {{"Profile", WeaponPinType::Profile}};
}

void RecoilProfileNode::Evaluate(const WeaponContext& /*ctx*/, const std::vector<WeaponValue>& inputs, std::vector<WeaponValue>& outputs) const {
    float damage = 10.0f, fireRate = 1.0f;

    if (!inputs.empty() && inputs[0].data.size() >= 2) {
        damage = inputs[0].data[0];
        fireRate = inputs[0].data[1];
    }

    // Profile data: [verticalRecoil, horizontalRecoil, spread]
    float verticalRecoil = damage * 0.1f * fireRate;
    float horizontalRecoil = verticalRecoil * 0.3f;
    float spread = fireRate * 0.05f;

    outputs.resize(1);
    outputs[0].type = WeaponPinType::Profile;
    outputs[0].data = {verticalRecoil, horizontalRecoil, spread};
}

// --- WeaponStatsNode ---

std::vector<WeaponPort> WeaponStatsNode::Inputs() const {
    return {
        {"Component", WeaponPinType::Component},
        {"Wear", WeaponPinType::Float}
    };
}

std::vector<WeaponPort> WeaponStatsNode::Outputs() const {
    return {{"Stats", WeaponPinType::Stats}};
}

void WeaponStatsNode::Evaluate(const WeaponContext& ctx, const std::vector<WeaponValue>& inputs, std::vector<WeaponValue>& outputs) const {
    float damage = 10.0f, fireRate = 1.0f, accuracy = 0.7f, range = 50.0f;

    if (!inputs.empty() && inputs[0].data.size() >= 4) {
        damage = inputs[0].data[0];
        fireRate = inputs[0].data[1];
        accuracy = inputs[0].data[2];
        range = inputs[0].data[3];
    }

    float wear = ctx.wearLevel;
    if (inputs.size() > 1 && !inputs[1].data.empty()) {
        wear = inputs[1].data[0];
    }
    wear = std::clamp(wear, 0.0f, 1.0f);

    // Wear degrades stats
    float wearFactor = 1.0f - wear * 0.3f; // up to 30% degradation
    damage *= wearFactor;
    accuracy *= wearFactor;
    range *= wearFactor;

    // Stats: [damage, fireRate, accuracy, range, dps]
    float dps = damage * fireRate;

    outputs.resize(1);
    outputs[0].type = WeaponPinType::Stats;
    outputs[0].data = {damage, fireRate, accuracy, range, dps};
}

}
