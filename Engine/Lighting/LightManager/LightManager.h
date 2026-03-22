#pragma once
/**
 * @file LightManager.h
 * @brief Dynamic scene light management: add, update, cull, and query lights.
 *
 * LightManager maintains a registry of scene lights and provides efficient
 * per-frame culling:
 *   - LightType: Directional, Point, Spot, Area.
 *   - LightDesc: type, position, direction, colour (RGB), intensity, range,
 *     inner/outer spot angles (degrees), shadow casting flag, shadow bias.
 *   - AddLight(desc): register a light; returns stable LightId.
 *   - RemoveLight(id): remove a light from the scene.
 *   - UpdateLight(id, desc): replace descriptor for a live light.
 *   - SetAmbient(colour, intensity): global ambient term.
 *   - CullVisible(frustumNormals, frustumDs): sphere-frustum test for point/spot;
 *     directional lights always pass.
 *   - GetPointLightsInRadius(center, radius): overlap query for local lighting.
 *   - GetDirectionalLights(): all directional lights (no culling needed).
 *   - GetShadowCasters(): lights with castsShadow == true.
 *   - Callbacks: OnLightAdded, OnLightRemoved.
 *   - LightStats: total, visible (after last cull), shadowCasting.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <functional>
#include "Engine/Math/Math.h"

namespace Engine {

// ── Light id ──────────────────────────────────────────────────────────────
using LightId = uint32_t;
static constexpr LightId kInvalidLightId = 0;

// ── Light type ────────────────────────────────────────────────────────────
enum class LightType : uint8_t { Directional, Point, Spot, Area };

// ── Light descriptor ──────────────────────────────────────────────────────
struct LightDesc {
    LightType             type{LightType::Point};
    Math::Vec3            position{};
    Math::Vec3            direction{0.0f, -1.0f, 0.0f};  ///< Normalised
    std::array<float, 3>  colour{1.0f, 1.0f, 1.0f};      ///< Linear RGB
    float                 intensity{1.0f};
    float                 range{10.0f};        ///< World units (Point/Spot/Area)
    float                 spotInnerAngle{15.0f}; ///< Degrees (Spot only)
    float                 spotOuterAngle{30.0f}; ///< Degrees (Spot only)
    bool                  castsShadow{false};
    float                 shadowBias{0.005f};
    std::string           tag;
};

// ── Live light ────────────────────────────────────────────────────────────
struct Light {
    LightId   id{kInvalidLightId};
    LightDesc desc{};
    bool      active{true};
};

// ── Light stats ───────────────────────────────────────────────────────────
struct LightStats {
    uint32_t total{0};
    uint32_t visible{0};        ///< After last CullVisible() call
    uint32_t shadowCasting{0};
};

// ── Callbacks ─────────────────────────────────────────────────────────────
using LightAddedCb   = std::function<void(LightId)>;
using LightRemovedCb = std::function<void(LightId)>;

// ── LightManager ──────────────────────────────────────────────────────────
class LightManager {
public:
    LightManager();
    ~LightManager();

    LightManager(const LightManager&) = delete;
    LightManager& operator=(const LightManager&) = delete;

    // ── registry ──────────────────────────────────────────────
    LightId       AddLight(const LightDesc& desc);
    bool          RemoveLight(LightId id);
    bool          UpdateLight(LightId id, const LightDesc& desc);
    void          SetActive(LightId id, bool active);
    void          Clear();

    // ── ambient ───────────────────────────────────────────────
    void SetAmbient(std::array<float, 3> colour, float intensity);
    std::array<float, 3> AmbientColour() const;
    float                AmbientIntensity() const;

    // ── queries ───────────────────────────────────────────────
    const Light*       GetLight(LightId id) const;
    std::vector<LightId> AllIds() const;
    std::vector<LightId> GetDirectionalLights() const;
    std::vector<LightId> GetShadowCasters() const;

    /// Sphere-frustum test: point/spot lights within the frustum;
    /// directional lights always included.
    std::vector<LightId> CullVisible(
        const std::array<Math::Vec3, 6>& frustumNormals,
        const std::array<float,        6>& frustumDs) const;

    /// Returns point/spot lights whose range spheres overlap the query sphere.
    std::vector<LightId> GetPointLightsInRadius(const Math::Vec3& center,
                                                float radius) const;

    std::vector<LightId> ByTag(const std::string& tag) const;

    // ── callbacks ─────────────────────────────────────────────
    void OnLightAdded(LightAddedCb cb);
    void OnLightRemoved(LightRemovedCb cb);

    // ── stats ─────────────────────────────────────────────────
    LightStats Stats() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
