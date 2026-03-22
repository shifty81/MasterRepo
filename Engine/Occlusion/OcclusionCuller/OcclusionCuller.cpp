#include "Engine/Occlusion/OcclusionCuller/OcclusionCuller.h"
#include <cmath>
#include <algorithm>
#include <vector>

namespace Engine {

// ── Frustum helpers ───────────────────────────────────────────────────────
static void normalisePlane(FrustumPlane& p) {
    float len = std::sqrt(p.normal.x * p.normal.x +
                          p.normal.y * p.normal.y +
                          p.normal.z * p.normal.z);
    if (len > 1e-7f) {
        float inv = 1.0f / len;
        p.normal.x *= inv; p.normal.y *= inv; p.normal.z *= inv;
        p.d *= inv;
    }
}

Frustum Frustum::FromMatrix(const Math::Mat4& m) {
    // Gribb-Hartmann extraction (row-major)
    Frustum f;

    // Left:   col3 + col0
    f.planes[0].normal = { m.m[3][0] + m.m[0][0],
                            m.m[3][1] + m.m[0][1],
                            m.m[3][2] + m.m[0][2] };
    f.planes[0].d      =   m.m[3][3] + m.m[0][3];

    // Right:  col3 - col0
    f.planes[1].normal = { m.m[3][0] - m.m[0][0],
                            m.m[3][1] - m.m[0][1],
                            m.m[3][2] - m.m[0][2] };
    f.planes[1].d      =   m.m[3][3] - m.m[0][3];

    // Bottom: col3 + col1
    f.planes[2].normal = { m.m[3][0] + m.m[1][0],
                            m.m[3][1] + m.m[1][1],
                            m.m[3][2] + m.m[1][2] };
    f.planes[2].d      =   m.m[3][3] + m.m[1][3];

    // Top:    col3 - col1
    f.planes[3].normal = { m.m[3][0] - m.m[1][0],
                            m.m[3][1] - m.m[1][1],
                            m.m[3][2] - m.m[1][2] };
    f.planes[3].d      =   m.m[3][3] - m.m[1][3];

    // Near:   col3 + col2
    f.planes[4].normal = { m.m[3][0] + m.m[2][0],
                            m.m[3][1] + m.m[2][1],
                            m.m[3][2] + m.m[2][2] };
    f.planes[4].d      =   m.m[3][3] + m.m[2][3];

    // Far:    col3 - col2
    f.planes[5].normal = { m.m[3][0] - m.m[2][0],
                            m.m[3][1] - m.m[2][1],
                            m.m[3][2] - m.m[2][2] };
    f.planes[5].d      =   m.m[3][3] - m.m[2][3];

    for (auto& p : f.planes) normalisePlane(p);
    return f;
}

CullResult Frustum::TestPoint(const Math::Vec3& p) const {
    for (const auto& plane : planes)
        if (plane.DistanceTo(p) < 0.0f) return CullResult::Outside;
    return CullResult::Inside;
}

CullResult Frustum::TestAABB(const Math::Vec3& minPt, const Math::Vec3& maxPt) const {
    bool intersecting = false;
    for (const auto& plane : planes) {
        // Positive vertex (most positive in plane direction)
        Math::Vec3 pos = {
            plane.normal.x >= 0.0f ? maxPt.x : minPt.x,
            plane.normal.y >= 0.0f ? maxPt.y : minPt.y,
            plane.normal.z >= 0.0f ? maxPt.z : minPt.z
        };
        if (plane.DistanceTo(pos) < 0.0f) return CullResult::Outside;

        Math::Vec3 neg = {
            plane.normal.x >= 0.0f ? minPt.x : maxPt.x,
            plane.normal.y >= 0.0f ? minPt.y : maxPt.y,
            plane.normal.z >= 0.0f ? minPt.z : maxPt.z
        };
        if (plane.DistanceTo(neg) < 0.0f) intersecting = true;
    }
    return intersecting ? CullResult::Intersecting : CullResult::Inside;
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct OcclusionCuller::Impl {
    Frustum              frustum{};
    std::vector<CullObject> submitted;
    CullStats            stats{};
};

OcclusionCuller::OcclusionCuller() : m_impl(new Impl()) {}
OcclusionCuller::~OcclusionCuller() { delete m_impl; }

void OcclusionCuller::SetFrustum(const Frustum& f) { m_impl->frustum = f; }
void OcclusionCuller::SetFrustumFromMatrix(const Math::Mat4& vp) {
    m_impl->frustum = Frustum::FromMatrix(vp);
}
const Frustum& OcclusionCuller::GetFrustum() const { return m_impl->frustum; }

CullResult OcclusionCuller::CullAABB(const Math::Vec3& minPt,
                                      const Math::Vec3& maxPt) const {
    return m_impl->frustum.TestAABB(minPt, maxPt);
}

bool OcclusionCuller::IsVisible(const Math::Vec3& minPt,
                                 const Math::Vec3& maxPt) const {
    return CullAABB(minPt, maxPt) != CullResult::Outside;
}

void OcclusionCuller::SubmitAABB(uint32_t id,
                                  const Math::Vec3& minPt,
                                  const Math::Vec3& maxPt) {
    m_impl->submitted.push_back({id, minPt, maxPt});
}

std::vector<uint32_t> OcclusionCuller::CullAll() {
    std::vector<uint32_t> visible;
    visible.reserve(m_impl->submitted.size());

    m_impl->stats.submitted += static_cast<uint32_t>(m_impl->submitted.size());

    for (const auto& obj : m_impl->submitted) {
        if (IsVisible(obj.aabbMin, obj.aabbMax)) {
            visible.push_back(obj.id);
            ++m_impl->stats.visible;
        } else {
            ++m_impl->stats.culled;
        }
    }
    m_impl->submitted.clear();
    return visible;
}

void OcclusionCuller::ClearSubmitted() { m_impl->submitted.clear(); }

CullStats OcclusionCuller::FrameStats() const { return m_impl->stats; }
void      OcclusionCuller::ResetStats() { m_impl->stats = {}; }

} // namespace Engine
