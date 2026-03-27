#pragma once
/**
 * @file LensFlareSystem.h
 * @brief Screen-space lens flare: source list, occlusion, streaks & ghosts, configurable elements.
 *
 * Features:
 *   - FlareSource: world position, intensity, colour, radius
 *   - Occlusion check callback: LensFlareOcclusionFn (returns 0-1 visibility)
 *   - FlareElement types: Ghost, Streak, Halo, Glow
 *   - Per-element: offset along flare axis, size, colour tint, rotation
 *   - Flare template: ordered list of elements
 *   - Multiple sources; each references a template
 *   - Screen-space axis: source→screen-centre, ghosts placed along it
 *   - Tick: updates occlusion fade per source
 *   - GetActiveElements(viewPos, viewDir) → list of elements to render
 *   - Template presets: Sun, Headlight, Explosion
 *
 * Typical usage:
 * @code
 *   LensFlareSystem lf;
 *   lf.Init();
 *   lf.SetOcclusionFn([](const float wp[3]){ return RayCastSun(wp)?0.f:1.f; });
 *   uint32_t tmpl = lf.CreateTemplate("sun");
 *   lf.AddElement(tmpl, {FlareType::Halo, 0.f, 1.f, {1,0.9f,0.7f,0.5f}});
 *   uint32_t src = lf.AddSource({sunWorldPos, 1.f, {1,1,0.8f}, 1.f, tmpl});
 *   auto elems = lf.GetActiveElements({0,0,0},{0,0,1});
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class FlareType : uint8_t { Ghost=0, Streak, Halo, Glow };

struct FlareElement {
    FlareType type   {FlareType::Ghost};
    float     offset {0.f};       ///< fraction along source→centre axis
    float     size   {0.5f};      ///< relative screen size
    float     colour [4]{1,1,1,0.5f};
    float     rotation{0.f};      ///< degrees
};

struct FlareSourceDesc {
    float    worldPos [3]{};
    float    intensity{1.f};
    float    colour   [3]{1,1,1};
    float    radius   {1.f};      ///< world-space radius for occlusion query
    uint32_t templateId{0};
};

struct ActiveFlareElement {
    FlareType type;
    float     screenPos[2]{};     ///< NDC
    float     size;
    float     colour[4]{};
    float     rotation;
    float     visibility;         ///< 0-1 from occlusion
};

using LensFlareOcclusionFn = std::function<float(const float worldPos[3])>;

class LensFlareSystem {
public:
    LensFlareSystem();
    ~LensFlareSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);      ///< updates occlusion fade

    // Occlusion
    void SetOcclusionFn(LensFlareOcclusionFn fn);

    // Templates
    uint32_t CreateTemplate(const std::string& name);
    void     AddElement    (uint32_t templateId, const FlareElement& elem);
    void     ClearElements (uint32_t templateId);

    // Sources
    uint32_t AddSource   (const FlareSourceDesc& desc);
    void     RemoveSource(uint32_t sourceId);
    void     SetIntensity(uint32_t sourceId, float intensity);
    bool     HasSource   (uint32_t sourceId) const;

    // Query
    std::vector<ActiveFlareElement> GetActiveElements(const float viewPos[3],
                                                       const float viewDir[3],
                                                       float fovY=60.f,
                                                       float aspect=1.78f) const;

    std::vector<uint32_t> GetAllSources() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
