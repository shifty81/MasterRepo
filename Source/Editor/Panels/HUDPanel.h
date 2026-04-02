#pragma once
#include "Editor/Application/EditorInputState.h"
#include "Game/Interaction/InteractionLoop.h"

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Minimal HUD / status panel for Phase 3 interaction loop.
///
/// Displays the player rig's health and energy bars alongside a compact
/// inventory readout.  Wired to an InteractionLoop so it always reflects
/// live gameplay state.
class HUDPanel {
public:
    // -------------------------------------------------------------------------
    // Wiring
    // -------------------------------------------------------------------------

    void SetUIRenderer(UIRenderer* r)                 noexcept { m_Renderer = r; }
    void SetInputState(const EditorInputState* i)     noexcept { m_Input = i; }

    /// @brief Provide the interaction loop to display (non-owning).
    void SetInteractionLoop(NF::Game::InteractionLoop* loop) noexcept { m_Loop = loop; }

    // -------------------------------------------------------------------------
    // Panel interface
    // -------------------------------------------------------------------------

    void Update(float dt);
    void Draw(float x, float y, float w, float h);

private:
    UIRenderer*                   m_Renderer{nullptr};
    const EditorInputState*       m_Input{nullptr};
    NF::Game::InteractionLoop*    m_Loop{nullptr};

    /// @brief Draw a labelled progress bar (value in [0,1]).
    void DrawBar(float x, float y, float w, float barH,
                 float fraction, uint32_t fillColor,
                 const char* label) const;
};

} // namespace NF::Editor
