#pragma once
/**
 * @file DebugDraw.h
 * @brief Immediate-mode debug geometry overlay (lines, spheres, boxes, text).
 *
 * DebugDraw accumulates geometry requests each frame and submits them as a
 * single draw-call batch at frame end.  All primitives expire automatically
 * after their lifetime, or can be marked persistent.
 *
 * Features:
 *   - Lines, rays, arrows
 *   - Spheres, AABBs, OBBs, capsules, cylinders, cones
 *   - Coordinate axes, grids
 *   - Screen-space text labels attached to world positions
 *   - Groups / channels with enable/disable
 *   - Zero overhead when disabled (no-op all calls)
 *
 * Typical usage:
 * @code
 *   DebugDraw& dd = DebugDraw::Get();
 *   dd.Line({0,0,0}, {1,0,0}, {1,0,0,1});
 *   dd.Sphere(pos, 0.5f, {0,1,0,1});
 *   dd.Text(pos, "enemy HP: 42", {1,1,1,1});
 *   dd.Flush();   // at frame end
 * @endcode
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Engine {

struct DebugColour { float r{1},g{1},b{1},a{1}; };

struct DebugDrawSettings {
    bool enabled{true};
    bool depthTest{false};          ///< true = occluded by geometry
    float defaultLifetime{0.f};     ///< 0 = this frame only
};

class DebugDraw {
public:
    static DebugDraw& Get();

    void Init(const DebugDrawSettings& settings = {});
    void Shutdown();

    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    // Primitives
    void Line(const float a[3], const float b[3],
              DebugColour colour = {}, float lifetime = 0.f);

    void Ray(const float origin[3], const float dir[3], float length = 5.f,
             DebugColour colour = {}, float lifetime = 0.f);

    void Arrow(const float from[3], const float to[3],
               DebugColour colour = {}, float headSize = 0.1f, float lifetime = 0.f);

    void Sphere(const float centre[3], float radius,
                DebugColour colour = {}, float lifetime = 0.f, int segments = 12);

    void Box(const float min[3], const float max[3],
             DebugColour colour = {}, float lifetime = 0.f);

    void OBB(const float centre[3], const float halfExtents[3],
             const float rotMat3[9], DebugColour colour = {}, float lifetime = 0.f);

    void Capsule(const float base[3], const float tip[3], float radius,
                 DebugColour colour = {}, float lifetime = 0.f);

    void Cross(const float pos[3], float size = 0.1f,
               DebugColour colour = {}, float lifetime = 0.f);

    void Axes(const float pos[3], float length = 0.3f, float lifetime = 0.f);

    void Grid(const float centre[3], float cellSize, uint32_t cells,
              DebugColour colour = {0.4f,0.4f,0.4f,0.8f}, float lifetime = 0.f);

    void Text(const float worldPos[3], const std::string& text,
              DebugColour colour = {}, float lifetime = 0.f);

    // Groups
    void EnableGroup(const std::string& group, bool enabled);
    void SetGroup(const std::string& group);   ///< subsequent calls use this group
    void ClearGroup();

    // Frame lifecycle
    void Update(float dt);      ///< expire timed primitives
    void Flush();               ///< submit to GPU (no-op if disabled)
    void Clear();               ///< remove all primitives

    uint32_t PrimitiveCount() const;

private:
    DebugDraw();
    ~DebugDraw();
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
