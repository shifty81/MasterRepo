#pragma once
/**
 * @file ColorGrading.h
 * @brief LUT upload, exposure/contrast/saturation, lift-gamma-gain, apply to RGBA buffer.
 *
 * Features:
 *   - ColorGradingSettings: exposure(EV), contrast, saturation, hueShift(degrees)
 *   - Lift/Gamma/Gain per channel (shadows/midtones/highlights)
 *   - 3D LUT: 16³ or 32³, RGBA float, upload from array or file
 *   - Apply(pixelBuffer, width, height): in-place CPU processing
 *   - Lerp between two ColorGradingSettings (for cross-fade)
 *   - Presets: Neutral, Warm, Cold, Vintage, Noir, Vivid
 *   - Per-volume blend weight (for post-process volume stack)
 *   - GetLUTTexelSize() for GPU shader use
 *   - Dirty flag: IsSettingsDirty() — for GPU re-upload
 *
 * Typical usage:
 * @code
 *   ColorGrading cg;
 *   cg.Init();
 *   ColorGradingSettings s; s.exposure=0.5f; s.saturation=1.2f;
 *   cg.SetSettings(s);
 *   cg.Apply(rgbaBuffer, width, height);
 *   // Or load LUT:
 *   cg.LoadLUT3D("luts/warm16.cube", 16);
 * @endcode
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Engine {

struct LiftGammaGain {
    float lift [3]{0,0,0};    ///< additive shadow offset [-1,1]
    float gamma[3]{1,1,1};    ///< midtone power [0.1,10]
    float gain [3]{1,1,1};    ///< highlight multiplier [0,4]
};

struct ColorGradingSettings {
    float exposure  {0.f};    ///< EV stops
    float contrast  {1.f};    ///< [0,2]
    float saturation{1.f};    ///< [0,2]
    float hueShift  {0.f};    ///< degrees [-180,180]
    LiftGammaGain lgg;
    float blendWeight{1.f};
};

class ColorGrading {
public:
    ColorGrading();
    ~ColorGrading();

    void Init    ();
    void Shutdown();

    // Settings
    void                        SetSettings(const ColorGradingSettings& s);
    const ColorGradingSettings& GetSettings() const;
    void                        LerpSettings(const ColorGradingSettings& a,
                                              const ColorGradingSettings& b, float t);
    bool                        IsSettingsDirty() const;
    void                        ClearDirty();

    // Presets
    void ApplyPreset(const std::string& presetName); // "neutral","warm","cold","vintage","noir","vivid"

    // LUT
    bool LoadLUT3D  (const std::string& cubePath, uint32_t lutSize=16);
    void UploadLUT3D(const float* rgbData, uint32_t lutSize);
    void ClearLUT   ();
    bool HasLUT     ()  const;
    uint32_t LUTSize() const;
    const float* LUTData() const;      ///< raw lutSize^3 * 3 floats

    // CPU apply
    void Apply(uint8_t* rgbaBuffer, uint32_t width, uint32_t height) const;

    // GPU helper
    float GetLUTTexelSize() const;     ///< 1.0 / lutSize

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
