#pragma once
/**
 * @file ToneMappingSystem.h
 * @brief HDR→LDR tone mapping operators: Reinhard, ACES, Uncharted2, exposure, gamma.
 *
 * Features:
 *   - Operator: Linear, Reinhard, ACES, Uncharted2, Hejl, Custom
 *   - SetOperator(op) / GetOperator() → Operator
 *   - SetExposure(ev) / GetExposure() → float: EV stops
 *   - SetGamma(g) / GetGamma() → float: display gamma (default 2.2)
 *   - SetWhitePoint(w): Reinhard/Uncharted2 white point
 *   - SetSaturation(s): post-tone-map colour saturation [0,2]
 *   - SetContrast(c): contrast S-curve strength [0,2]
 *   - Apply(inHDR[], outLDR[], pixelCount): apply selected operator
 *   - ApplyPixel(r, g, b, outR, outG, outB): single-pixel version
 *   - SetCustomCurve(cb): callback(r,g,b,outR,outG,outB) for Custom operator
 *   - ComputeHistogram(inHDR[], pixelCount, outBins[], binCount)
 *   - AutoExposure(inHDR[], pixelCount): adjust m_exposure via histogram
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

enum class TonemapOperator : uint8_t { Linear, Reinhard, ACES, Uncharted2, Hejl, Custom };

class ToneMappingSystem {
public:
    ToneMappingSystem();
    ~ToneMappingSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Config
    void             SetOperator  (TonemapOperator op);
    TonemapOperator  GetOperator  () const;
    void             SetExposure  (float ev);
    float            GetExposure  () const;
    void             SetGamma     (float g);
    float            GetGamma     () const;
    void             SetWhitePoint(float w);
    void             SetSaturation(float s);
    void             SetContrast  (float c);

    // Processing
    void Apply     (const std::vector<float>& inHDR,
                    std::vector<float>&       outLDR,
                    uint32_t pixelCount) const;
    void ApplyPixel(float r, float g, float b,
                    float& outR, float& outG, float& outB) const;

    // Custom curve
    void SetCustomCurve(
        std::function<void(float r, float g, float b,
                           float& outR, float& outG, float& outB)> cb);

    // Auto-exposure
    void ComputeHistogram(const std::vector<float>& inHDR, uint32_t pixelCount,
                          std::vector<uint32_t>& outBins, uint32_t binCount) const;
    void AutoExposure    (const std::vector<float>& inHDR, uint32_t pixelCount);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
