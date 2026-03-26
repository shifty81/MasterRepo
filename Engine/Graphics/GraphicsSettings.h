#pragma once
/**
 * @file GraphicsSettings.h
 * @brief Runtime graphics quality manager — resolution scaling, LOD bias,
 *        shadow quality, MSAA, vsync, and dynamic quality adjustment.
 *
 * Settings are persisted to `Config/graphics.json` between sessions.
 * The DynamicQuality subsystem automatically steps down or up quality
 * presets to maintain a target frame rate.
 *
 * Typical usage:
 * @code
 *   GraphicsSettings gfx;
 *   gfx.Init();
 *   gfx.SetPreset(GraphicsPreset::High);
 *   gfx.SetResolutionScale(0.75f);    // 75% render resolution
 *   gfx.SetVSync(true);
 *   gfx.EnableDynamicQuality(60.f);   // target 60 fps
 *   gfx.Tick(deltaSeconds, measuredFps);
 *   gfx.Apply();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>

namespace Engine {

// ── Named quality presets ──────────────────────────────────────────────────────

enum class GraphicsPreset : uint8_t {
    Ultra  = 0,
    High   = 1,
    Medium = 2,
    Low    = 3,
    Custom = 4,
};

// ── MSAA sample count ─────────────────────────────────────────────────────────

enum class MSAALevel : uint8_t { Off = 0, X2 = 1, X4 = 2, X8 = 3 };

// ── Shadow quality tier ───────────────────────────────────────────────────────

enum class ShadowQuality : uint8_t { Off = 0, Low = 1, Medium = 2, High = 3, Ultra = 4 };

// ── Full settings snapshot ────────────────────────────────────────────────────

struct GraphicsSettingsData {
    GraphicsPreset preset{GraphicsPreset::High};
    float          resolutionScale{1.0f};   ///< 0.5 – 2.0
    MSAALevel      msaa{MSAALevel::X4};
    ShadowQuality  shadows{ShadowQuality::High};
    float          lodBias{0.0f};           ///< negative = higher detail
    bool           vsync{true};
    bool           hdr{false};
    bool           bloom{true};
    bool           ambientOcclusion{true};
    bool           motionBlur{false};
    uint32_t       maxFPS{0};              ///< 0 = unlimited
    bool           fullscreen{false};
    uint32_t       windowWidth{1280};
    uint32_t       windowHeight{720};
};

// ── GraphicsSettings ──────────────────────────────────────────────────────────

class GraphicsSettings {
public:
    GraphicsSettings();
    ~GraphicsSettings();

    void Init(const std::string& configPath = "Config/graphics.json");
    void Shutdown();

    // ── Preset & individual settings ──────────────────────────────────────────

    void SetPreset(GraphicsPreset preset);
    GraphicsPreset GetPreset() const;

    void SetResolutionScale(float scale);
    void SetMSAA(MSAALevel level);
    void SetShadowQuality(ShadowQuality quality);
    void SetLodBias(float bias);
    void SetVSync(bool enabled);
    void SetHDR(bool enabled);
    void SetBloom(bool enabled);
    void SetAmbientOcclusion(bool enabled);
    void SetMotionBlur(bool enabled);
    void SetMaxFPS(uint32_t fps);
    void SetFullscreen(bool fullscreen);
    void SetResolution(uint32_t w, uint32_t h);

    GraphicsSettingsData GetSettings() const;
    void                 SetSettings(const GraphicsSettingsData& data);

    // ── Dynamic quality ───────────────────────────────────────────────────────

    /// Enable automatic quality scaling to hit targetFps.
    void EnableDynamicQuality(float targetFps, float hysteresis = 5.f);
    void DisableDynamicQuality();

    /// Call once per frame to allow dynamic quality to adjust.
    void Tick(float deltaSeconds, float measuredFps);

    // ── Persistence ───────────────────────────────────────────────────────────

    void Save() const;
    void Load();

    /// Push current settings to the renderer / engine (call after changes).
    void Apply();

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnSettingsChanged(std::function<void(const GraphicsSettingsData&)> cb);
    void OnPresetChanged(std::function<void(GraphicsPreset)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
