#include "Engine/PostProcess/PostProcessPipeline.h"
#include <chrono>
#include <algorithm>

namespace Engine {

static double ppNowMs() {
    using namespace std::chrono;
    return static_cast<double>(
        duration_cast<microseconds>(
            high_resolution_clock::now().time_since_epoch()).count()) / 1000.0;
}

struct PostProcessPipeline::Impl {
    PostProcessConfig        cfg;
    std::vector<PassResult>  lastResults;
    double                   lastFrameMs{0.0};
    // Per-pass enabled overrides (keyed by name); absent = follow config
    std::unordered_map<std::string, bool> passOverrides;
};

PostProcessPipeline::PostProcessPipeline() : m_impl(new Impl()) {}
PostProcessPipeline::~PostProcessPipeline() { delete m_impl; }

void PostProcessPipeline::SetConfig(const PostProcessConfig& cfg) {
    m_impl->cfg = cfg;
}
const PostProcessConfig& PostProcessPipeline::Config() const { return m_impl->cfg; }
PostProcessConfig&       PostProcessPipeline::Config()       { return m_impl->cfg; }

void PostProcessPipeline::EnablePass(const std::string& passName, bool enabled) {
    m_impl->passOverrides[passName] = enabled;
}
bool PostProcessPipeline::IsPassEnabled(const std::string& passName) const {
    auto it = m_impl->passOverrides.find(passName);
    if (it != m_impl->passOverrides.end()) return it->second;
    // Fall back to config flags
    const auto& c = m_impl->cfg;
    if (passName == "bloom")   return c.bloom.enabled;
    if (passName == "tonemap") return c.tonemap.enabled;
    if (passName == "vignette")return c.vignette.enabled;
    if (passName == "ssao")    return c.ssao.enabled;
    if (passName == "chroma")  return c.chromaticAberration.enabled;
    if (passName == "grade")   return c.colorGrading.enabled;
    if (passName == "fxaa")    return c.fxaa.enabled;
    return false;
}

// ── Private pass stubs ────────────────────────────────────────────────────
#define PP_PASS(name) \
    PassResult PostProcessPipeline::name(uint32_t /*fbo*/, uint32_t /*w*/, uint32_t /*h*/)

PP_PASS(runBloom) {
    double t0 = ppNowMs();
    (void)m_impl->cfg.bloom; // config consumed by GPU shader (stubbed)
    return {"bloom", m_impl->cfg.bloom.enabled, ppNowMs() - t0};
}
PP_PASS(runTonemap) {
    double t0 = ppNowMs();
    return {"tonemap", m_impl->cfg.tonemap.enabled, ppNowMs() - t0};
}
PP_PASS(runVignette) {
    double t0 = ppNowMs();
    return {"vignette", m_impl->cfg.vignette.enabled, ppNowMs() - t0};
}
PP_PASS(runSSAO) {
    double t0 = ppNowMs();
    return {"ssao", m_impl->cfg.ssao.enabled, ppNowMs() - t0};
}
PP_PASS(runChromaticAberration) {
    double t0 = ppNowMs();
    return {"chroma", m_impl->cfg.chromaticAberration.enabled, ppNowMs() - t0};
}
PP_PASS(runColorGrading) {
    double t0 = ppNowMs();
    return {"grade", m_impl->cfg.colorGrading.enabled, ppNowMs() - t0};
}
PP_PASS(runFXAA) {
    double t0 = ppNowMs();
    return {"fxaa", m_impl->cfg.fxaa.enabled, ppNowMs() - t0};
}

std::vector<PassResult> PostProcessPipeline::Execute(uint32_t fbo,
                                                       uint32_t w,
                                                       uint32_t h) {
    double t0 = ppNowMs();
    m_impl->lastResults.clear();

    auto run = [&](auto fn, const std::string& name) {
        if (IsPassEnabled(name)) {
            auto r = (this->*fn)(fbo, w, h);
            m_impl->lastResults.push_back(r);
        }
    };

    run(&PostProcessPipeline::runSSAO,               "ssao");
    run(&PostProcessPipeline::runBloom,              "bloom");
    run(&PostProcessPipeline::runChromaticAberration,"chroma");
    run(&PostProcessPipeline::runColorGrading,       "grade");
    run(&PostProcessPipeline::runTonemap,            "tonemap");
    run(&PostProcessPipeline::runVignette,           "vignette");
    run(&PostProcessPipeline::runFXAA,               "fxaa");

    m_impl->lastFrameMs = ppNowMs() - t0;
    return m_impl->lastResults;
}

double PostProcessPipeline::LastFrameMs() const { return m_impl->lastFrameMs; }
const std::vector<PassResult>& PostProcessPipeline::LastResults() const {
    return m_impl->lastResults;
}

} // namespace Engine
