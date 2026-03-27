#pragma once
/**
 * @file TransformGizmo.h
 * @brief 3-axis translate/rotate/scale handles, snap, undo-friendly delta output.
 *
 * Features:
 *   - GizmoMode: Translate, Rotate, Scale
 *   - GizmoSpace: World, Local
 *   - Mouse interaction: BeginDrag(axis, screenPos), Drag(screenPos), EndDrag()
 *   - Returns GizmoDelta: translation[3], rotation (axis+angle), scale[3]
 *   - Snap: SetSnapTranslate(grid), SetSnapRotate(degreesStep), SetSnapScale(step)
 *   - Multi-axis: Shift to constrain to plane (XY/XZ/YZ)
 *   - Axis hit-test: GetHoveredAxis(screenRay) → Axis enum
 *   - DrawData: array of colored line segments for renderer
 *   - Per-entity transform input: SetTransform(pos, rot, scale)
 *   - Immediate-mode: ManipulateTransform(...) → returns bool changed
 *
 * Typical usage:
 * @code
 *   TransformGizmo gizmo;
 *   gizmo.Init();
 *   gizmo.SetMode(GizmoMode::Translate);
 *   gizmo.SetTransform(entity.pos, entity.rot, entity.scale);
 *   if (gizmo.ManipulateTransform(viewMat, projMat, mousePos, mouseDown))
 *       entity.pos = gizmo.GetPosition();
 *   for (auto& seg : gizmo.GetDrawData()) renderer.DrawLine(seg);
 * @endcode
 */

#include <cstdint>
#include <vector>

namespace Editor {

enum class GizmoMode  : uint8_t { Translate=0, Rotate, Scale };
enum class GizmoSpace : uint8_t { World=0, Local };
enum class GizmoAxis  : uint8_t { None=0, X, Y, Z, XY, XZ, YZ };

struct GizmoDelta {
    float translate[3]{};
    float rotAxis  [3]{0,1,0};
    float rotAngle {0.f};   ///< degrees
    float scale    [3]{1,1,1};
    bool  changed  {false};
};

struct GizmoSegment {
    float start[3]{};
    float end  [3]{};
    float colour[4]{1,1,1,1};
    float thickness{1.f};
};

class TransformGizmo {
public:
    TransformGizmo();
    ~TransformGizmo();

    void Init    ();
    void Shutdown();

    // Mode / space
    void       SetMode (GizmoMode  m);
    GizmoMode  GetMode ()  const;
    void       SetSpace(GizmoSpace s);
    GizmoSpace GetSpace()  const;

    // Current entity transform
    void SetTransform(const float pos[3], const float rotQuat[4], const float scale[3]);
    void GetPosition (float out[3]) const;
    void GetRotation (float out[4]) const;
    void GetScale    (float out[3]) const;

    // Snap settings
    void SetSnapTranslate(float grid);
    void SetSnapRotate   (float degreesStep);
    void SetSnapScale    (float step);
    void SetSnapEnabled  (bool enabled);

    // Interaction
    GizmoAxis GetHoveredAxis(const float rayOrigin[3], const float rayDir[3]) const;
    void      BeginDrag(GizmoAxis axis, const float screenPos[2]);
    void      Drag     (const float screenPos[2], const float rayDir[3]);
    GizmoDelta EndDrag ();
    bool      IsDragging() const;

    // Immediate-mode helper (returns true when transform changed)
    bool ManipulateTransform(const float view[16], const float proj[16],
                              const float mousePos[2], bool mouseDown);

    // Render data
    const std::vector<GizmoSegment>& GetDrawData() const;

    void SetScreenSize(uint32_t w, uint32_t h);
    void SetGizmoScale(float worldScale);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Editor
