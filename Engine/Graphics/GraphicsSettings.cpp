#include "Engine/Graphics/GraphicsSettings.h"
#include <chrono>
#include <fstream>
#include <sstream>

namespace Engine {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

static GraphicsSettingsData PresetDefaults(GraphicsPreset p) {
    GraphicsSettingsData d;
    d.preset = p;
    switch (p) {
        case GraphicsPreset::Ultra:
            d.resolutionScale = 1.5f; d.msaa = MSAALevel::X8;
            d.shadows = ShadowQuality::Ultra; d.lodBias = -0.5f;
            d.bloom = true; d.ambientOcclusion = true; d.motionBlur = true;
            break;
        case GraphicsPreset::High:
            d.resolutionScale = 1.0f; d.msaa = MSAALevel::X4;
            d.shadows = ShadowQuality::High; d.lodBias = 0.f;
            d.bloom = true; d.ambientOcclusion = true;
            break;
        case GraphicsPreset::Medium:
            d.resolutionScale = 1.0f; d.msaa = MSAALevel::X2;
            d.shadows = ShadowQuality::Medium; d.lodBias = 0.5f;
            d.bloom = true;
            break;
        case GraphicsPreset::Low:
            d.resolutionScale = 0.75f; d.msaa = MSAALevel::Off;
            d.shadows = ShadowQuality::Low; d.lodBias = 1.5f;
            break;
        default: break;
    }
    return d;
}

struct GraphicsSettings::Impl {
    GraphicsSettingsData  data{};
    std::string           configPath;
    bool                  dynamicQuality{false};
    float                 targetFps{60.f};
    float                 hysteresis{5.f};
    uint64_t              lastAdjustMs{0};
    std::function<void(const GraphicsSettingsData&)> onChange;
    std::function<void(GraphicsPreset)>              onPreset;
};

GraphicsSettings::GraphicsSettings() : m_impl(new Impl()) {}
GraphicsSettings::~GraphicsSettings() { delete m_impl; }

void GraphicsSettings::Init(const std::string& configPath) {
    m_impl->configPath = configPath;
    Load();
}

void GraphicsSettings::Shutdown() { Save(); }

void GraphicsSettings::SetPreset(GraphicsPreset p) {
    m_impl->data = PresetDefaults(p);
    if (m_impl->onPreset) m_impl->onPreset(p);
    if (m_impl->onChange) m_impl->onChange(m_impl->data);
}

GraphicsPreset GraphicsSettings::GetPreset() const { return m_impl->data.preset; }

void GraphicsSettings::SetResolutionScale(float v)          { m_impl->data.resolutionScale = v;  m_impl->data.preset = GraphicsPreset::Custom; if (m_impl->onChange) m_impl->onChange(m_impl->data); }
void GraphicsSettings::SetMSAA(MSAALevel v)                 { m_impl->data.msaa = v;             m_impl->data.preset = GraphicsPreset::Custom; if (m_impl->onChange) m_impl->onChange(m_impl->data); }
void GraphicsSettings::SetShadowQuality(ShadowQuality v)    { m_impl->data.shadows = v;          m_impl->data.preset = GraphicsPreset::Custom; if (m_impl->onChange) m_impl->onChange(m_impl->data); }
void GraphicsSettings::SetLodBias(float v)                  { m_impl->data.lodBias = v;          m_impl->data.preset = GraphicsPreset::Custom; if (m_impl->onChange) m_impl->onChange(m_impl->data); }
void GraphicsSettings::SetVSync(bool v)                     { m_impl->data.vsync = v;            m_impl->data.preset = GraphicsPreset::Custom; if (m_impl->onChange) m_impl->onChange(m_impl->data); }
void GraphicsSettings::SetHDR(bool v)                       { m_impl->data.hdr = v;              m_impl->data.preset = GraphicsPreset::Custom; if (m_impl->onChange) m_impl->onChange(m_impl->data); }
void GraphicsSettings::SetBloom(bool v)                     { m_impl->data.bloom = v;            m_impl->data.preset = GraphicsPreset::Custom; if (m_impl->onChange) m_impl->onChange(m_impl->data); }
void GraphicsSettings::SetAmbientOcclusion(bool v)          { m_impl->data.ambientOcclusion = v; m_impl->data.preset = GraphicsPreset::Custom; if (m_impl->onChange) m_impl->onChange(m_impl->data); }
void GraphicsSettings::SetMotionBlur(bool v)                { m_impl->data.motionBlur = v;       m_impl->data.preset = GraphicsPreset::Custom; if (m_impl->onChange) m_impl->onChange(m_impl->data); }
void GraphicsSettings::SetMaxFPS(uint32_t v)                { m_impl->data.maxFPS = v;           m_impl->data.preset = GraphicsPreset::Custom; if (m_impl->onChange) m_impl->onChange(m_impl->data); }
void GraphicsSettings::SetFullscreen(bool v)                { m_impl->data.fullscreen = v;       m_impl->data.preset = GraphicsPreset::Custom; if (m_impl->onChange) m_impl->onChange(m_impl->data); }

void GraphicsSettings::SetResolution(uint32_t w, uint32_t h) {
    m_impl->data.windowWidth  = w;
    m_impl->data.windowHeight = h;
    m_impl->data.preset = GraphicsPreset::Custom;
    if (m_impl->onChange) m_impl->onChange(m_impl->data);
}

GraphicsSettingsData GraphicsSettings::GetSettings() const { return m_impl->data; }

void GraphicsSettings::SetSettings(const GraphicsSettingsData& d) {
    m_impl->data = d;
    if (m_impl->onChange) m_impl->onChange(d);
}

void GraphicsSettings::EnableDynamicQuality(float targetFps, float hysteresis) {
    m_impl->dynamicQuality = true;
    m_impl->targetFps      = targetFps;
    m_impl->hysteresis     = hysteresis;
}

void GraphicsSettings::DisableDynamicQuality() { m_impl->dynamicQuality = false; }

void GraphicsSettings::Tick(float /*dt*/, float measuredFps) {
    if (!m_impl->dynamicQuality) return;
    uint64_t now = NowMs();
    if (now - m_impl->lastAdjustMs < 2000) return; // adjust every 2s max
    m_impl->lastAdjustMs = now;

    GraphicsPreset cur = m_impl->data.preset;
    if (measuredFps < m_impl->targetFps - m_impl->hysteresis) {
        // Step down
        if (cur == GraphicsPreset::Ultra)  { SetPreset(GraphicsPreset::High);   Apply(); }
        else if (cur == GraphicsPreset::High)   { SetPreset(GraphicsPreset::Medium); Apply(); }
        else if (cur == GraphicsPreset::Medium) { SetPreset(GraphicsPreset::Low);    Apply(); }
    } else if (measuredFps > m_impl->targetFps + m_impl->hysteresis * 2.f) {
        // Step up
        if (cur == GraphicsPreset::Low)    { SetPreset(GraphicsPreset::Medium); Apply(); }
        else if (cur == GraphicsPreset::Medium) { SetPreset(GraphicsPreset::High);   Apply(); }
        else if (cur == GraphicsPreset::High)   { SetPreset(GraphicsPreset::Ultra);  Apply(); }
    }
}

void GraphicsSettings::Save() const {
    if (m_impl->configPath.empty()) return;
    std::ofstream f(m_impl->configPath);
    if (!f.is_open()) return;
    f << "{\"resolutionScale\":" << m_impl->data.resolutionScale
      << ",\"vsync\":"           << (m_impl->data.vsync ? "true" : "false")
      << ",\"preset\":"          << static_cast<int>(m_impl->data.preset)
      << ",\"msaa\":"            << static_cast<int>(m_impl->data.msaa)
      << ",\"shadows\":"         << static_cast<int>(m_impl->data.shadows)
      << "}\n";
}

void GraphicsSettings::Load() {
    // Minimal stub; full impl delegates to Core/Serialization
}

void GraphicsSettings::Apply() {
    // Platform hook: push settings to the renderer
}

void GraphicsSettings::OnSettingsChanged(
    std::function<void(const GraphicsSettingsData&)> cb) {
    m_impl->onChange = std::move(cb);
}

void GraphicsSettings::OnPresetChanged(std::function<void(GraphicsPreset)> cb) {
    m_impl->onPreset = std::move(cb);
}

} // namespace Engine
