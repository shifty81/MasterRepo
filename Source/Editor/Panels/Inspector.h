#pragma once
#include "Engine/ECS/World.h"

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Panel that displays and edits components on the selected entity.
class Inspector {
public:
    /// @brief Set the entity and world to inspect.
    /// @param id    Entity to inspect; pass NullEntity to clear.
    /// @param world World that owns the entity.
    void SetSelectedEntity(EntityId id, World* world) noexcept {
        m_SelectedEntity = id;
        m_World          = world;
    }

    /// @brief Set the UIRenderer used for drawing.
    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Advance panel state.
    void Update(float dt);

    /// @brief Draw all components of the selected entity within the given region.
    void Draw(float x, float y, float w, float h);

private:
    EntityId     m_SelectedEntity{NullEntity};
    World*       m_World{nullptr};
    UIRenderer*  m_Renderer{nullptr};
};

} // namespace NF::Editor
