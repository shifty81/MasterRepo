#pragma once
#include "Core/Math/Vector.h"
#include <vector>

namespace NF {

class RenderDevice;

/// @brief Immediate-mode debug visualisation helpers.
///
/// All methods accumulate geometry into an internal buffer; call Flush() once
/// per frame to submit them to the GPU.
class DebugDraw {
public:
    /// @brief Enqueue a line segment.
    /// @param start World-space start position.
    /// @param end   World-space end position.
    /// @param color RGB colour in [0, 1].
    static void Line(const Vector3& start, const Vector3& end, const Vector3& color);

    /// @brief Enqueue a wireframe axis-aligned box.
    /// @param center  World-space centre.
    /// @param extents Half-extents along each axis.
    /// @param color   RGB colour in [0, 1].
    static void Box(const Vector3& center, const Vector3& extents, const Vector3& color);

    /// @brief Enqueue a wireframe sphere approximated by latitude/longitude arcs.
    /// @param center World-space centre.
    /// @param radius Sphere radius.
    /// @param color  RGB colour in [0, 1].
    static void Sphere(const Vector3& center, float radius, const Vector3& color);

    /// @brief Submit all accumulated geometry to the GPU and clear the buffer.
    /// @param device Active render device.
    static void Flush(RenderDevice& device);

private:
    struct LineSegment {
        Vector3 Start{};
        Vector3 End{};
        Vector3 Color{};
    };

    static std::vector<LineSegment>& GetBuffer();
};

} // namespace NF
