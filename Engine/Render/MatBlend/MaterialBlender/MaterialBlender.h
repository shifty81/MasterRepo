#pragma once
/**
 * @file MaterialBlender.h
 * @brief Multi-layer material blending with mask textures, weight arrays, and lerp/overlay modes.
 *
 * Features:
 *   - CreateBlendSet(id): create a named blend set
 *   - AddLayer(setId, materialKey, weight, blendMode): push a material layer
 *   - RemoveLayer(setId, layerIndex)
 *   - SetLayerWeight(setId, layerIndex, weight): update blend weight [0,1]
 *   - SetMaskSampler(setId, layerIndex, sampleFn): per-pixel mask function
 *   - Sample(setId, u, v) → RGBA: evaluate blended colour at UV coordinates
 *   - Blend modes: Normal, Multiply, Overlay, Screen, Add, Lerp
 *   - NormalizeWeights(setId): scale weights so they sum to 1
 *   - GetLayerCount(setId) → uint32_t
 *   - SetLayerEnabled(setId, layerIndex, enabled)
 *   - GetDominantLayer(setId, u, v) → layerIndex
 *   - ExportWeightMap(setId, width, height, outPixels[]): bake weight map
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class MatBlendMode : uint8_t { Normal, Multiply, Overlay, Screen, Add, Lerp };

struct MatRGBA { float r, g, b, a; };

class MaterialBlender {
public:
    MaterialBlender();
    ~MaterialBlender();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Blend set management
    uint32_t CreateBlendSet  (const std::string& name);
    void     DestroyBlendSet (uint32_t setId);

    // Layer authoring
    uint32_t AddLayer   (uint32_t setId, const std::string& materialKey,
                         float weight=1.f, MatBlendMode mode=MatBlendMode::Lerp);
    void     RemoveLayer(uint32_t setId, uint32_t layerIndex);

    // Layer control
    void SetLayerWeight (uint32_t setId, uint32_t layerIndex, float weight);
    void SetLayerEnabled(uint32_t setId, uint32_t layerIndex, bool enabled);
    void SetBlendMode   (uint32_t setId, uint32_t layerIndex, MatBlendMode mode);
    void SetMaterialColour(uint32_t setId, uint32_t layerIndex, MatRGBA colour);

    // Mask sampler: returns [0,1] mask value at (u,v)
    void SetMaskSampler(uint32_t setId, uint32_t layerIndex,
                        std::function<float(float u, float v)> sampler);

    // Sampling
    MatRGBA  Sample          (uint32_t setId, float u, float v) const;
    uint32_t GetDominantLayer(uint32_t setId, float u, float v) const;

    // Utilities
    void NormalizeWeights(uint32_t setId);
    void ExportWeightMap (uint32_t setId, uint32_t width, uint32_t height,
                          float* outPixels) const; ///< outPixels[w*h] = dominant weight

    uint32_t GetLayerCount(uint32_t setId) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
