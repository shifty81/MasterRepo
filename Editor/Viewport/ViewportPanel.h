#pragma once
#include <cstdint>
#include <string>

namespace Editor {

enum class CameraMode { FPS, Orbit, Freecam };
enum class GizmoMode  { None, Translate, Rotate, Scale };

class IPanel;

class ViewportPanel {
public:
    ViewportPanel();

    // IPanel-compatible interface
    std::string Name() const { return "Viewport"; }
    bool IsVisible() const   { return m_visible; }
    void SetVisible(bool v)  { m_visible = v; }

    // Gizmo
    void      SetGizmoMode(GizmoMode mode);
    GizmoMode GetGizmoMode() const;

    // Grid
    void SetGridVisible(bool visible);
    bool IsGridVisible() const;

    // Selection
    uint32_t SelectedObjectId() const;
    void     DeselectAll();

    // Camera
    void       SetCameraDistance(float dist);
    void       SetCameraMode(CameraMode mode);
    CameraMode GetCameraMode() const;

private:
    uint32_t   m_selectedId  = 0;
    bool       m_gridVisible = true;
    float      m_camDistance = 10.0f;
    bool       m_visible     = true;
    GizmoMode  m_gizmoMode   = GizmoMode::None;
    CameraMode m_camMode     = CameraMode::Orbit;
};

} // namespace Editor
