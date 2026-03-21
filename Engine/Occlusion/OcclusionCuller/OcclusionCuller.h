#pragma once
/**
 * @file OcclusionCuller.h
 * @brief View-frustum culling with hierarchical AABB occlusion testing.
 *
 * OcclusionCuller accelerates scene rendering by rejecting objects that
 * lie outside the camera frustum or are fully occluded by opaque geometry:
 *
 *   - Frustum: 6 planes (left/right/top/bottom/near/far) extracted from a
 *     view-projection matrix via the Gribb-Hartmann method.
 *   - CullAABB(min, max): tests an axis-aligned bounding box against all 6
 *     planes; returns CullResult::Inside, Intersecting, or Outside.
 *   - Batch cull: SubmitAABB / CullAll returns per-object visibility flags
 *     in one pass — friendly to cache and SIMD.
 *   - OccluderList: a set of axis-aligned occluder planes (screen-space
 *     occlusion disabled in this CPU-only implementation; reserved for
 *     future GPU query support).
 *   - Stats: tracks objects submitted, culled, and visible per frame.
 */

#include <cstdint>
#include <vector>
#include <array>
#include "Engine/Math/Math.h"

namespace Engine {

// ── Culling result ─────────────────────────────────────────────────────────
enum class CullResult : uint8_t { Inside, Intersecting, Outside };

// ── Frustum plane ─────────────────────────────────────────────────────────
struct FrustumPlane {
    Math::Vec3 normal{};
    float      d{0.0f};          ///< Plane equation: normal·x + d = 0

    float DistanceTo(const Math::Vec3& p) const {
        return normal.x * p.x + normal.y * p.y + normal.z * p.z + d;
    }
};

// ── View frustum ──────────────────────────────────────────────────────────
struct Frustum {
    std::array<FrustumPlane, 6> planes{};

    /// Extract frustum planes from a combined view-projection matrix.
    static Frustum FromMatrix(const Math::Mat4& viewProj);

    /// Test point; returns Outside if behind any plane.
    CullResult TestPoint(const Math::Vec3& p) const;

    /// Test AABB; returns Outside, Intersecting, or Inside.
    CullResult TestAABB(const Math::Vec3& minPt, const Math::Vec3& maxPt) const;
};

// ── Submitted object ──────────────────────────────────────────────────────
struct CullObject {
    uint32_t   id{0};            ///< User-assigned object id
    Math::Vec3 aabbMin{};
    Math::Vec3 aabbMax{};
};

// ── Per-frame stats ───────────────────────────────────────────────────────
struct CullStats {
    uint32_t submitted{0};
    uint32_t visible{0};
    uint32_t culled{0};
};

// ── Culler ────────────────────────────────────────────────────────────────
class OcclusionCuller {
public:
    OcclusionCuller();
    ~OcclusionCuller();

    OcclusionCuller(const OcclusionCuller&) = delete;
    OcclusionCuller& operator=(const OcclusionCuller&) = delete;

    // ── frustum ───────────────────────────────────────────────
    void SetFrustum(const Frustum& f);
    void SetFrustumFromMatrix(const Math::Mat4& viewProj);
    const Frustum& GetFrustum() const;

    // ── single object test ────────────────────────────────────
    CullResult CullAABB(const Math::Vec3& minPt, const Math::Vec3& maxPt) const;
    bool       IsVisible(const Math::Vec3& minPt, const Math::Vec3& maxPt) const;

    // ── batch cull ────────────────────────────────────────────
    /// Add an object for batch processing.  Call CullAll() to test all at once.
    void SubmitAABB(uint32_t id, const Math::Vec3& minPt, const Math::Vec3& maxPt);

    /// Cull all submitted objects; returns ids of visible objects.
    std::vector<uint32_t> CullAll();

    /// Clear submitted list (called automatically by CullAll).
    void ClearSubmitted();

    // ── stats ─────────────────────────────────────────────────
    CullStats FrameStats() const;
    void      ResetStats();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
