#include "PostProcess.h"

namespace atlas::render {

BloomSettings BloomSettings::Default() {
    BloomSettings s;
    s.threshold = 1.0f;
    s.intensity = 1.0f;
    s.radius = 0.005f;
    s.mipCount = 5;
    s.enabled = true;
    return s;
}

ToneMappingSettings ToneMappingSettings::Default() {
    ToneMappingSettings s;
    s.op = ToneMapOperator::ACES;
    s.exposure = 1.0f;
    s.gamma = 2.2f;
    s.whitePoint = 4.0f;
    s.enabled = true;
    return s;
}

PostProcessSettings PostProcessSettings::Default() {
    PostProcessSettings s;
    s.bloom = BloomSettings::Default();
    s.toneMapping = ToneMappingSettings::Default();
    s.fxaaEnabled = true;
    s.vignetteIntensity = 0.0f;
    s.chromaticAberrationIntensity = 0.0f;
    s.filmGrainIntensity = 0.0f;
    return s;
}

void PostProcessPipeline::Init(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    m_settings = PostProcessSettings::Default();
    m_outputTextureId = m_nextTextureId++;
    m_initialized = true;
}

void PostProcessPipeline::Shutdown() {
    m_initialized = false;
    m_outputTextureId = 0;
    m_width = 0;
    m_height = 0;
}

void PostProcessPipeline::Resize(uint32_t width, uint32_t height) {
    if (!m_initialized) return;
    m_width = width;
    m_height = height;
    // Re-allocate output texture with new dimensions
    m_outputTextureId = m_nextTextureId++;
}

void PostProcessPipeline::SetSettings(const PostProcessSettings& settings) {
    m_settings = settings;
}

bool PostProcessPipeline::IsEffectEnabled(PostProcessEffect effect) const {
    switch (effect) {
        case PostProcessEffect::Bloom:                return m_settings.bloom.enabled;
        case PostProcessEffect::ToneMapping:          return m_settings.toneMapping.enabled;
        case PostProcessEffect::FXAA:                 return m_settings.fxaaEnabled;
        case PostProcessEffect::Vignette:             return m_settings.vignetteIntensity > 0.0f;
        case PostProcessEffect::ChromaticAberration:  return m_settings.chromaticAberrationIntensity > 0.0f;
        case PostProcessEffect::FilmGrain:            return m_settings.filmGrainIntensity > 0.0f;
    }
    return false;
}

void PostProcessPipeline::SetEffectEnabled(PostProcessEffect effect, bool enabled) {
    switch (effect) {
        case PostProcessEffect::Bloom:
            m_settings.bloom.enabled = enabled;
            break;
        case PostProcessEffect::ToneMapping:
            m_settings.toneMapping.enabled = enabled;
            break;
        case PostProcessEffect::FXAA:
            m_settings.fxaaEnabled = enabled;
            break;
        case PostProcessEffect::Vignette:
            m_settings.vignetteIntensity = enabled ? 0.5f : 0.0f;
            break;
        case PostProcessEffect::ChromaticAberration:
            m_settings.chromaticAberrationIntensity = enabled ? 0.5f : 0.0f;
            break;
        case PostProcessEffect::FilmGrain:
            m_settings.filmGrainIntensity = enabled ? 0.5f : 0.0f;
            break;
    }
}

void PostProcessPipeline::Execute() {
    if (!m_initialized) return;

    // Each active effect conceptually processes the frame buffer.
    // The output texture id represents the final composited result.
    m_outputTextureId = m_nextTextureId++;
}

uint32_t PostProcessPipeline::EffectCount() const {
    uint32_t count = 0;
    if (m_settings.bloom.enabled) ++count;
    if (m_settings.toneMapping.enabled) ++count;
    if (m_settings.fxaaEnabled) ++count;
    if (m_settings.vignetteIntensity > 0.0f) ++count;
    if (m_settings.chromaticAberrationIntensity > 0.0f) ++count;
    if (m_settings.filmGrainIntensity > 0.0f) ++count;
    return count;
}

} // namespace atlas::render
