#pragma once
#include "Core/Math/Vector.h"
#include "Engine/ECS/World.h"
#include "Renderer/RHI/RenderDevice.h"

namespace NF::Editor {

/// @brief The manipulation mode of the gizmo.
enum class GizmoMode { Translate, Rotate, Scale };

/// @brief 3-D transform gizmo for manipulating entities in the viewport.
class TransformGizmo {
public:
    /// @brief Set the active gizmo mode.
    void SetMode(GizmoMode mode) noexcept { m_Mode = mode; }

    /// @brief Set the entity currently being manipulated.
    void SetSelectedEntity(EntityId id) noexcept { m_SelectedEntity = id; }

    /// @brief Update gizmo interaction state.
    /// @param dt       Delta time in seconds.
    /// @param mousePos Current mouse position in viewport space.
    void Update(float dt, const Vector2& mousePos);

    /// @brief Draw the gizmo into the viewport.
    /// @param device Active render device.
    void Draw(RenderDevice& device);

    /// @brief Returns true while the user is dragging a gizmo handle.
    [[nodiscard]] bool IsActive() const noexcept { return m_Active; }

private:
    GizmoMode m_Mode{GizmoMode::Translate};
    EntityId  m_SelectedEntity{NullEntity};
    Vector2   m_LastMousePos{};
    bool      m_Active{false};
};

} // namespace NF::Editor
