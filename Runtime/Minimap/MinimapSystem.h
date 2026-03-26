#pragma once
/**
 * @file MinimapSystem.h
 * @brief Minimap and world-map rendering with icons, fog-of-war, and zoom.
 *
 * The minimap renders to an off-screen texture (or callback-provided surface)
 * at configurable resolution.  Supports:
 *   - Configurable world-space bounds → texture-space transform
 *   - Per-layer icons: players, enemies, items, waypoints (custom sprites)
 *   - Fog-of-war mask updated by entity vision range
 *   - Zoom in/out with smooth lerp
 *   - Compass-style rotation to follow player heading
 *   - Region label overlay
 *
 * Typical usage:
 * @code
 *   MinimapSystem map;
 *   MinimapConfig cfg;
 *   cfg.worldExtent = 2000.f;
 *   cfg.textureSize = 256;
 *   map.Init(cfg);
 *   map.RegisterIcon("player", "UI/Icons/player.png", IconLayer::Player);
 *   map.SetPlayerPosition(playerPos, playerHeading);
 *   map.Update(dt);
 *   map.Render();
 *   auto* tex = map.GetRenderTexture();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

// ── Icon layer ────────────────────────────────────────────────────────────────

enum class IconLayer : uint8_t {
    Terrain   = 0,
    Waypoint  = 1,
    Item      = 2,
    Enemy     = 3,
    Player    = 4,
    Objective = 5,
    Custom    = 6,
    COUNT
};

// ── A map icon ────────────────────────────────────────────────────────────────

struct MapIcon {
    uint32_t    id{0};
    std::string name;
    std::string spritePath;
    IconLayer   layer{IconLayer::Custom};
    float       worldPos[3]{};
    float       size{8.f};      ///< pixels at zoom=1
    uint32_t    colour{0xFFFFFFFFu}; ///< RGBA tint
    bool        visible{true};
    float       heading{0.f};   ///< rotation degrees
};

// ── Minimap configuration ─────────────────────────────────────────────────────

struct MinimapConfig {
    float    worldExtent{1000.f};   ///< half-side of the square world (meters)
    uint32_t textureSize{256};      ///< output render texture size (px)
    float    defaultZoom{1.f};      ///< 1 = full map visible
    bool     rotateWithPlayer{false};
    bool     fogOfWarEnabled{true};
    float    fogVisionRadius{100.f}; ///< in world units
    uint32_t backgroundColour{0xFF222222u}; ///< unexplored colour
};

// ── MinimapSystem ─────────────────────────────────────────────────────────────

class MinimapSystem {
public:
    MinimapSystem();
    ~MinimapSystem();

    void Init(const MinimapConfig& config = {});
    void Shutdown();

    // ── Player ────────────────────────────────────────────────────────────────

    void SetPlayerPosition(const float worldPos[3], float headingDeg = 0.f);
    void SetZoom(float zoom);    ///< 1 = full-map, >1 = zoomed in
    void ZoomTowards(float targetZoom, float speed = 2.f);

    // ── Icons ─────────────────────────────────────────────────────────────────

    uint32_t AddIcon(const std::string& name, const std::string& spritePath,
                     IconLayer layer, const float worldPos[3]);
    void     MoveIcon(uint32_t iconId, const float worldPos[3]);
    void     SetIconVisible(uint32_t iconId, bool visible);
    void     SetIconHeading(uint32_t iconId, float headingDeg);
    void     RemoveIcon(uint32_t iconId);
    MapIcon  GetIcon(uint32_t iconId) const;

    // ── Fog of war ────────────────────────────────────────────────────────────

    void RevealArea(const float worldPos[3], float radius);
    void SetFogEnabled(bool enabled);
    void ResetFog();

    // ── Regions ───────────────────────────────────────────────────────────────

    void AddRegionLabel(const std::string& label, const float worldPos[3],
                        float radius);
    void ClearRegionLabels();

    // ── Update / render ───────────────────────────────────────────────────────

    void Update(float dt);
    void Render();                              ///< draws into internal texture
    const uint8_t* GetRenderTexture() const;    ///< RGBA, textureSize×textureSize

    // ── World ↔ screen helpers ────────────────────────────────────────────────

    void WorldToMap(const float worldPos[3], float& mapX, float& mapY) const;
    void MapToWorld(float mapX, float mapY, float worldPos[3]) const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnIconClicked(std::function<void(uint32_t iconId)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
