#include "Editor/Viewport/ViewportPanel.h"

namespace Editor {

ViewportPanel::ViewportPanel() = default;

void ViewportPanel::SetGizmoMode(GizmoMode mode) { m_gizmoMode = mode; }
GizmoMode ViewportPanel::GetGizmoMode() const    { return m_gizmoMode; }

void ViewportPanel::SetGridVisible(bool visible) { m_gridVisible = visible; }
bool ViewportPanel::IsGridVisible() const        { return m_gridVisible; }

uint32_t ViewportPanel::SelectedObjectId() const { return m_selectedId; }
void     ViewportPanel::DeselectAll()            { m_selectedId = 0; }

void ViewportPanel::SetCameraDistance(float dist) { m_camDistance = dist; }
void ViewportPanel::SetCameraMode(CameraMode mode) { m_camMode = mode; }
CameraMode ViewportPanel::GetCameraMode() const    { return m_camMode; }

} // namespace Editor
