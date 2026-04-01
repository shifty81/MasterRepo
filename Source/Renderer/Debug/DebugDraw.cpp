#include "Renderer/Debug/DebugDraw.h"
#include "Renderer/RHI/RenderDevice.h"
#include <cmath>
#include <numbers>

namespace NF {

std::vector<DebugDraw::LineSegment>& DebugDraw::GetBuffer() {
    static std::vector<LineSegment> s_Lines;
    return s_Lines;
}

void DebugDraw::Line(const Vector3& start, const Vector3& end, const Vector3& color) {
    GetBuffer().push_back({start, end, color});
}

void DebugDraw::Box(const Vector3& center, const Vector3& extents, const Vector3& color) {
    // Compute the 8 corners of the AABB.
    const float ex = extents.X, ey = extents.Y, ez = extents.Z;
    Vector3 c[8] = {
        {center.X - ex, center.Y - ey, center.Z - ez},
        {center.X + ex, center.Y - ey, center.Z - ez},
        {center.X + ex, center.Y + ey, center.Z - ez},
        {center.X - ex, center.Y + ey, center.Z - ez},
        {center.X - ex, center.Y - ey, center.Z + ez},
        {center.X + ex, center.Y - ey, center.Z + ez},
        {center.X + ex, center.Y + ey, center.Z + ez},
        {center.X - ex, center.Y + ey, center.Z + ez},
    };
    // Bottom face
    Line(c[0], c[1], color); Line(c[1], c[2], color);
    Line(c[2], c[3], color); Line(c[3], c[0], color);
    // Top face
    Line(c[4], c[5], color); Line(c[5], c[6], color);
    Line(c[6], c[7], color); Line(c[7], c[4], color);
    // Vertical edges
    Line(c[0], c[4], color); Line(c[1], c[5], color);
    Line(c[2], c[6], color); Line(c[3], c[7], color);
}

void DebugDraw::Sphere(const Vector3& center, float radius, const Vector3& color) {
    constexpr int   kSegments = 16;
    constexpr float kStep     = 2.f * std::numbers::pi_v<float> / kSegments;

    // Draw three orthogonal circles.
    for (int i = 0; i < kSegments; ++i) {
        float a0 = i * kStep, a1 = (i + 1) * kStep;
        // XY plane
        Line({center.X + radius * std::cos(a0), center.Y + radius * std::sin(a0), center.Z},
             {center.X + radius * std::cos(a1), center.Y + radius * std::sin(a1), center.Z},
             color);
        // XZ plane
        Line({center.X + radius * std::cos(a0), center.Y, center.Z + radius * std::sin(a0)},
             {center.X + radius * std::cos(a1), center.Y, center.Z + radius * std::sin(a1)},
             color);
        // YZ plane
        Line({center.X, center.Y + radius * std::cos(a0), center.Z + radius * std::sin(a0)},
             {center.X, center.Y + radius * std::cos(a1), center.Z + radius * std::sin(a1)},
             color);
    }
}

void DebugDraw::Flush(RenderDevice& device) {
    // In a full implementation this would batch-upload line vertices and issue
    // a draw call; here we simply discard the accumulated geometry since there
    // is no immediate-mode line renderer wired to the device yet.
    (void)device;
    GetBuffer().clear();
}

} // namespace NF
