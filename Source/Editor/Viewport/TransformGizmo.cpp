#include "Editor/Viewport/TransformGizmo.h"

namespace NF::Editor {

void TransformGizmo::Update([[maybe_unused]] float dt, const Vector2& mousePos) {
    m_LastMousePos = mousePos;
    // Drag detection and axis hit-testing would be implemented here.
}

void TransformGizmo::Draw([[maybe_unused]] RenderDevice& device) {
    if (m_SelectedEntity == NullEntity) return;
    // Gizmo geometry draw calls would be issued here.
}

} // namespace NF::Editor
