#pragma once
/**
 * @file PostProcessPipeline.h
 * @brief Screen-space post-processing effects pipeline.
 *
 * Provides a configurable pass chain applied after the main scene render:
 *   - Bloom       — luminance threshold extract + Gaussian blur + additive blend
 *   - Tonemapping — ACES filmic / Reinhard / Uncharted2 operators
 *   - Vignette    — radial darkening from screen edges
 *   - SSAO        — screen-space ambient occlusion (hemisphere sampling, blur)
 *   - ChromaticAberration — RGB channel separation
 *   - ColorGrading — lift/gamma/gain, saturation, contrast
 *   - FXAA        — fast approximate anti-aliasing
 *
 * All GPU calls are stubbed (function bodies reference the pass configs only).
 * The pipeline is backend-agnostic (OpenGL / Vulkan backend would implement
 * the virtual RenderPass interface in a derived class).
 */

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Engine {

// ── Pass configs ─────────────────────────────────────────────────────────

struct BloomConfig {
    bool    enabled{true};
    float   threshold{1.0f};   ///< Luminance above which bloom is applied
    float   intensity{0.8f};
    int     blurPasses{5};
    float   radius{4.0f};
};

struct TonemapConfig {
    enum class Operator { ACES, Reinhard, Uncharted2, None };
    bool     enabled{true};
    Operator op{Operator::ACES};
    float    exposure{1.0f};
    float    whitePoint{11.2f}; ///< Uncharted2
};

struct VignetteConfig {
    bool  enabled{true};
    float intensity{0.4f};
    float smoothness{0.45f};
    float roundness{0.5f};
};

struct SSAOConfig {
    bool    enabled{true};
    int     kernelSize{32};
    float   radius{0.5f};
    float   bias{0.025f};
    float   power{2.2f};
    int     blurPasses{4};
};

struct ChromaticAberrationConfig {
    bool  enabled{false};
    float intensity{0.01f};
};

struct ColorGradingConfig {
    bool  enabled{false};
    float lift{0.0f};       ///< Shadow lift
    float gamma{1.0f};
    float gain{1.0f};
    float saturation{1.0f};
    float contrast{1.0f};
};

struct FXAAConfig {
    bool  enabled{true};
    float edgeThreshold{0.063f};
    float edgeThresholdMin{0.0625f};
    float subPixelAA{0.75f};
};

// ── Aggregate config ──────────────────────────────────────────────────────
struct PostProcessConfig {
    BloomConfig              bloom;
    TonemapConfig            tonemap;
    VignetteConfig           vignette;
    SSAOConfig               ssao;
    ChromaticAberrationConfig chromaticAberration;
    ColorGradingConfig        colorGrading;
    FXAAConfig                fxaa;
};

// ── Per-pass render result ─────────────────────────────────────────────────
struct PassResult {
    std::string name;
    bool        executed{false};
    double      elapsedMs{0.0};
};

// ── Pipeline ─────────────────────────────────────────────────────────────
class PostProcessPipeline {
public:
    PostProcessPipeline();
    ~PostProcessPipeline();

    // ── configuration ─────────────────────────────────────────
    void SetConfig(const PostProcessConfig& cfg);
    const PostProcessConfig& Config() const;
    PostProcessConfig&       Config();

    // ── individual pass toggles ───────────────────────────────
    void EnablePass(const std::string& passName, bool enabled);
    bool IsPassEnabled(const std::string& passName) const;

    // ── execution ─────────────────────────────────────────────
    /// Run all enabled passes against the supplied framebuffer handle.
    /// @param framebufferHandle  Opaque GPU handle (e.g. OpenGL FBO id)
    /// @param width / height     Current render resolution
    std::vector<PassResult> Execute(uint32_t framebufferHandle,
                                     uint32_t width,
                                     uint32_t height);

    // ── stats ─────────────────────────────────────────────────
    double LastFrameMs() const;
    const std::vector<PassResult>& LastResults() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};

    PassResult runBloom(uint32_t fbo, uint32_t w, uint32_t h);
    PassResult runTonemap(uint32_t fbo, uint32_t w, uint32_t h);
    PassResult runVignette(uint32_t fbo, uint32_t w, uint32_t h);
    PassResult runSSAO(uint32_t fbo, uint32_t w, uint32_t h);
    PassResult runChromaticAberration(uint32_t fbo, uint32_t w, uint32_t h);
    PassResult runColorGrading(uint32_t fbo, uint32_t w, uint32_t h);
    PassResult runFXAA(uint32_t fbo, uint32_t w, uint32_t h);
};

} // namespace Engine
