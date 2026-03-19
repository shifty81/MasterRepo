#include "WorldNodes.h"
#include "NoiseGenerator.h"
#include <algorithm>

namespace atlas::world {

static constexpr int kChunkRes = 64;
static constexpr int kFieldSize = kChunkRes * kChunkRes;

// --- SeedNode ---

std::vector<NodePort> SeedNode::Outputs() const {
    return {{"Seed", ValueType::Seed}};
}

void SeedNode::Evaluate(const WorldGenContext& ctx, const std::vector<Value>& /*inputs*/, std::vector<Value>& outputs) const {
    outputs.resize(1);
    outputs[0].type = ValueType::Seed;
    outputs[0].data = {static_cast<float>(ctx.worldSeed)};
}

// --- NoiseNode ---

std::vector<NodePort> NoiseNode::Inputs() const {
    return {
        {"Seed", ValueType::Seed},
        {"Frequency", ValueType::Float}
    };
}

std::vector<NodePort> NoiseNode::Outputs() const {
    return {{"Height", ValueType::HeightField}};
}

void NoiseNode::Evaluate(const WorldGenContext& ctx, const std::vector<Value>& inputs, std::vector<Value>& outputs) const {
    uint32_t seed = 0;
    if (!inputs.empty() && !inputs[0].data.empty()) {
        seed = static_cast<uint32_t>(inputs[0].data[0]);
    }

    float frequency = 0.01f;
    if (inputs.size() > 1 && !inputs[1].data.empty()) {
        frequency = inputs[1].data[0];
    }

    outputs.resize(1);
    outputs[0].type = ValueType::HeightField;
    outputs[0].data.resize(kFieldSize);

    float offsetX = static_cast<float>(ctx.chunkX * kChunkRes);
    float offsetZ = static_cast<float>(ctx.chunkZ * kChunkRes);

    for (int z = 0; z < kChunkRes; ++z) {
        for (int x = 0; x < kChunkRes; ++x) {
            float wx = (offsetX + static_cast<float>(x)) * frequency;
            float wz = (offsetZ + static_cast<float>(z)) * frequency;
            outputs[0].data[static_cast<size_t>(z) * kChunkRes + x] =
                NoiseGenerator::FBM2D(wx, wz, 6, 2.0f, 0.5f, seed);
        }
    }
}

// --- BlendNode ---

std::vector<NodePort> BlendNode::Inputs() const {
    return {
        {"A", ValueType::HeightField},
        {"B", ValueType::HeightField},
        {"Factor", ValueType::Float}
    };
}

std::vector<NodePort> BlendNode::Outputs() const {
    return {{"Out", ValueType::HeightField}};
}

void BlendNode::Evaluate(const WorldGenContext& /*ctx*/, const std::vector<Value>& inputs, std::vector<Value>& outputs) const {
    outputs.resize(1);
    outputs[0].type = ValueType::HeightField;
    outputs[0].data.resize(kFieldSize, 0.0f);

    float factor = 0.5f;
    if (inputs.size() > 2 && !inputs[2].data.empty()) {
        factor = inputs[2].data[0];
    }

    const auto& a = (inputs.size() > 0 && inputs[0].data.size() == kFieldSize)
        ? inputs[0].data : outputs[0].data;
    const auto& b = (inputs.size() > 1 && inputs[1].data.size() == kFieldSize)
        ? inputs[1].data : outputs[0].data;

    for (int i = 0; i < kFieldSize; ++i) {
        outputs[0].data[i] = a[i] + (b[i] - a[i]) * factor;
    }
}

// --- ClampNode ---

std::vector<NodePort> ClampNode::Inputs() const {
    return {
        {"In", ValueType::HeightField},
        {"Min", ValueType::Float},
        {"Max", ValueType::Float}
    };
}

std::vector<NodePort> ClampNode::Outputs() const {
    return {{"Out", ValueType::HeightField}};
}

void ClampNode::Evaluate(const WorldGenContext& /*ctx*/, const std::vector<Value>& inputs, std::vector<Value>& outputs) const {
    outputs.resize(1);
    outputs[0].type = ValueType::HeightField;

    float minVal = 0.0f;
    float maxVal = 1.0f;
    if (inputs.size() > 1 && !inputs[1].data.empty()) minVal = inputs[1].data[0];
    if (inputs.size() > 2 && !inputs[2].data.empty()) maxVal = inputs[2].data[0];

    if (!inputs.empty() && inputs[0].data.size() == kFieldSize) {
        outputs[0].data.resize(kFieldSize);
        for (int i = 0; i < kFieldSize; ++i) {
            outputs[0].data[i] = std::clamp(inputs[0].data[i], minVal, maxVal);
        }
    } else {
        outputs[0].data.resize(kFieldSize, std::clamp(0.0f, minVal, maxVal));
    }
}

// --- ConstantNode ---

std::vector<NodePort> ConstantNode::Outputs() const {
    return {{"Value", ValueType::Float}};
}

void ConstantNode::Evaluate(const WorldGenContext& /*ctx*/, const std::vector<Value>& /*inputs*/, std::vector<Value>& outputs) const {
    outputs.resize(1);
    outputs[0].type = ValueType::Float;
    outputs[0].data = {value};
}

// --- BiomeNode ---

std::vector<NodePort> BiomeNode::Inputs() const {
    return {
        {"Elevation", ValueType::HeightField},
        {"Moisture", ValueType::HeightField}
    };
}

std::vector<NodePort> BiomeNode::Outputs() const {
    return {{"BiomeMap", ValueType::Mask}};
}

void BiomeNode::Evaluate(const WorldGenContext& /*ctx*/, const std::vector<Value>& inputs, std::vector<Value>& outputs) const {
    outputs.resize(1);
    outputs[0].type = ValueType::Mask;
    outputs[0].data.resize(kFieldSize, 2.0f); // default Plains

    bool hasElevation = !inputs.empty() && inputs[0].data.size() == static_cast<size_t>(kFieldSize);
    bool hasMoisture = inputs.size() > 1 && inputs[1].data.size() == static_cast<size_t>(kFieldSize);

    for (int i = 0; i < kFieldSize; ++i) {
        float elev = hasElevation ? inputs[0].data[i] : 0.5f;
        float moist = hasMoisture ? inputs[1].data[i] : 0.5f;

        // Biome classification based on elevation and moisture
        // 0=Ocean, 1=Beach, 2=Plains, 3=Forest, 4=Mountain, 5=Snow
        float biome;
        if (elev < 0.15f) {
            biome = 0.0f; // Ocean
        } else if (elev < 0.2f) {
            biome = 1.0f; // Beach
        } else if (elev > 0.85f) {
            biome = 5.0f; // Snow
        } else if (elev > 0.65f) {
            biome = 4.0f; // Mountain
        } else if (moist > 0.5f) {
            biome = 3.0f; // Forest
        } else {
            biome = 2.0f; // Plains
        }
        outputs[0].data[i] = biome;
    }
}

// --- ErosionNode ---

std::vector<NodePort> ErosionNode::Inputs() const {
    return {
        {"In", ValueType::HeightField},
        {"Seed", ValueType::Seed}
    };
}

std::vector<NodePort> ErosionNode::Outputs() const {
    return {{"Out", ValueType::HeightField}};
}

void ErosionNode::Evaluate(const WorldGenContext& /*ctx*/, const std::vector<Value>& inputs, std::vector<Value>& outputs) const {
    outputs.resize(1);
    outputs[0].type = ValueType::HeightField;

    if (inputs.empty() || inputs[0].data.size() != static_cast<size_t>(kFieldSize)) {
        outputs[0].data.resize(kFieldSize, 0.0f);
        return;
    }

    outputs[0].data = inputs[0].data;

    uint64_t seed = 0;
    if (inputs.size() > 1 && !inputs[1].data.empty()) {
        seed = static_cast<uint64_t>(inputs[1].data[0]);
    }

    // Deterministic xorshift64 RNG for droplet placement
    uint64_t state = seed ^ 0x5DEECE66DULL;
    if (state == 0) state = 1;
    auto xorshift = [&]() -> float {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return static_cast<float>(state & 0xFFFFu) / 65535.0f;
    };

    // Simplified hydraulic erosion: drop particles that flow downhill
    int iters = iterations > 0 ? iterations : 50;
    for (int iter = 0; iter < iters; ++iter) {
        int px = static_cast<int>(xorshift() * (kChunkRes - 2)) + 1;
        int py = static_cast<int>(xorshift() * (kChunkRes - 2)) + 1;
        float sediment = 0.0f;
        float water = 1.0f;
        constexpr float erosionRate = 0.01f;
        constexpr float depositionRate = 0.01f;
        constexpr float evaporationRate = 0.05f;

        for (int step = 0; step < 30 && water > 0.01f; ++step) {
            int idx = py * kChunkRes + px;
            float h = outputs[0].data[idx];

            // Find steepest neighbor
            int bestX = px, bestY = py;
            float bestH = h;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = px + dx, ny = py + dy;
                    if (nx < 0 || nx >= kChunkRes || ny < 0 || ny >= kChunkRes) continue;
                    float nh = outputs[0].data[ny * kChunkRes + nx];
                    if (nh < bestH) {
                        bestH = nh;
                        bestX = nx;
                        bestY = ny;
                    }
                }
            }

            float diff = h - bestH;
            if (diff <= 0.0f) {
                // Deposit sediment in local minimum
                outputs[0].data[idx] += sediment * depositionRate;
                break;
            }

            // Erode and carry sediment
            constexpr float kErosionDiffFactor = 0.5f;
            float erode = std::min(diff * kErosionDiffFactor, erosionRate * water);
            outputs[0].data[idx] -= erode;
            sediment += erode;

            // Deposit some sediment
            float deposit = sediment * depositionRate * (1.0f - diff);
            if (deposit > 0.0f && deposit < sediment) {
                outputs[0].data[idx] += deposit;
                sediment -= deposit;
            }

            water *= (1.0f - evaporationRate);
            px = bestX;
            py = bestY;
        }
    }
}

}
