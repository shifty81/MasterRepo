#pragma once
#include "Engine/ECS/World.h"

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

    /// @brief Advance panel state.
    void Update(float dt);

    /// @brief Draw all components of the selected entity.
    void Draw();

private:
    EntityId  m_SelectedEntity{NullEntity};
    World*    m_World{nullptr};
};

} // namespace NF::Editor
