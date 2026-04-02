#pragma once
#include "Editor/Application/EditorInputState.h"
#include "Engine/ECS/World.h"
#include <functional>

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Panel that lists all entities in the active world.
class SceneOutliner {
public:
    /// @brief Set the world to display.
    /// @param world Non-owning pointer; must outlive this panel.
    void SetWorld(World* world) noexcept { m_World = world; }

    /// @brief Set the UIRenderer used for drawing.
    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Provide the current per-frame OS input state.
    /// @param input Non-owning pointer updated by EditorApp each frame.
    void SetInputState(const EditorInputState* input) noexcept { m_Input = input; }

    /// @brief Advance panel state.
    void Update(float dt);

    /// @brief Draw the panel content within the given region.
    void Draw(float x, float y, float w, float h);

    /// @brief Return the currently selected entity.
    [[nodiscard]] EntityId GetSelectedEntity() const noexcept { return m_SelectedEntity; }

    /// @brief Register a callback invoked whenever the selection changes.
    void SetOnSelectionChanged(std::function<void(EntityId)> cb) { m_OnSelectionChanged = std::move(cb); }

private:
    World*                        m_World{nullptr};
    UIRenderer*                   m_Renderer{nullptr};
    const EditorInputState*       m_Input{nullptr};
    EntityId                      m_SelectedEntity{NullEntity};
    std::function<void(EntityId)> m_OnSelectionChanged;
};

} // namespace NF::Editor
