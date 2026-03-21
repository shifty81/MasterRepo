#pragma once
/**
 * @file DecalSystem.h
 * @brief Runtime decal projection: place, cull, sort, and expire textured decals.
 *
 * DecalSystem manages a pool of oriented-box decals projected onto world geometry:
 *   - Decal: position, half-extents, orientation (forward/up), texture id,
 *     colour tint, opacity, lifetime, and layer.
 *   - Spawn(desc): allocate a decal from the pool; returns stable DecalId.
 *   - Remove(id): immediately free a decal slot.
 *   - Update(dt): decrement lifetimes; fade-out dying decals; recycle expired.
 *   - CullVisible(frustum): returns ids of decals intersecting the view frustum.
 *   - SortByLayer / SortByDistance: ordering helpers for the render pass.
 *   - SetMaxDecals(n): resize the pool at runtime (caps at kHardLimit).
 *   - Stats: active count, expired this frame, pool capacity.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include "Engine/Math/Math.h"

namespace Engine {

// ── Decal id ──────────────────────────────────────────────────────────────
using DecalId = uint32_t;
static constexpr DecalId kInvalidDecalId = 0;

// ── Decal descriptor ──────────────────────────────────────────────────────
struct DecalDesc {
    Math::Vec3   position{};
    Math::Vec3   forward{0, 0, -1};      ///< Projection direction
    Math::Vec3   up{0, 1, 0};
    Math::Vec3   halfExtents{0.5f, 0.5f, 0.5f};
    uint32_t     textureId{0};           ///< GPU texture handle
    std::array<float, 4> colour{1,1,1,1}; ///< RGBA tint
    float        opacity{1.0f};
    float        lifetime{-1.0f};        ///< Seconds; negative = infinite
    float        fadeOutTime{0.5f};      ///< Seconds before expiry to fade
    int32_t      layer{0};              ///< Render layer (higher = on top)
    std::string  tag;
};

// ── Live decal ────────────────────────────────────────────────────────────
struct Decal {
    DecalId      id{kInvalidDecalId};
    DecalDesc    desc{};
    float        remainingLife{-1.0f};
    float        currentOpacity{1.0f};
    bool         active{false};
};

// ── Decal stats ───────────────────────────────────────────────────────────
struct DecalStats {
    uint32_t active{0};
    uint32_t expiredThisFrame{0};
    uint32_t poolCapacity{0};
};

// ── System ────────────────────────────────────────────────────────────────
class DecalSystem {
public:
    static constexpr uint32_t kHardLimit = 4096;

    DecalSystem();
    explicit DecalSystem(uint32_t maxDecals);
    ~DecalSystem();

    DecalSystem(const DecalSystem&) = delete;
    DecalSystem& operator=(const DecalSystem&) = delete;

    // ── spawn / remove ────────────────────────────────────────
    DecalId Spawn(const DecalDesc& desc);
    bool    Remove(DecalId id);
    void    Clear();

    // ── per-frame ─────────────────────────────────────────────
    void Update(float dt);

    // ── visibility ────────────────────────────────────────────
    /// Returns ids of active decals whose oriented boxes overlap the frustum.
    std::vector<DecalId> CullVisible(const std::array<Math::Vec3, 6>& frustumNormals,
                                     const std::array<float,        6>& frustumDs) const;

    // ── sorting ───────────────────────────────────────────────
    void SortByLayer(std::vector<DecalId>& ids) const;
    void SortByDistance(std::vector<DecalId>& ids, const Math::Vec3& eye) const;

    // ── queries ───────────────────────────────────────────────
    const Decal*            GetDecal(DecalId id) const;
    std::vector<DecalId>    ActiveIds() const;
    std::vector<DecalId>    ByTag(const std::string& tag) const;
    DecalStats              Stats() const;

    // ── pool resize ───────────────────────────────────────────
    void SetMaxDecals(uint32_t n);
    uint32_t MaxDecals() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
