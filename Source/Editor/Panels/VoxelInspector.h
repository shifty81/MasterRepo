#pragma once
#include "Editor/Application/EditorInputState.h"
#include "Game/Voxel/ChunkMap.h"
#include "Game/Voxel/VoxelDebugOverlay.h"
#include "Game/Voxel/VoxelType.h"

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Editor panel that inspects and toggles voxel visibility in the dev world.
///
/// Shows chunk stats, the selected chunk's voxel composition, and allows the
/// user to toggle between solid/all voxel display modes.  All voxel state is
/// read-only in this panel; modifications go through the VoxelEditApi.
class VoxelInspector {
public:
    // ---- Wiring -------------------------------------------------------------

    void SetUIRenderer(UIRenderer* r)          noexcept { m_Renderer = r; }
    void SetInputState(const EditorInputState* i) noexcept { m_Input = i; }

    /// @brief Provide the chunk map to inspect (non-owning; must outlive panel).
    void SetChunkMap(NF::Game::ChunkMap* map) noexcept { m_ChunkMap = map; }

    // ---- Panel interface ----------------------------------------------------

    void Update(float dt);
    void Draw(float x, float y, float w, float h);

    // ---- State --------------------------------------------------------------

    /// @brief Whether the voxel overlay should be visible in the viewport.
    [[nodiscard]] bool IsVoxelOverlayVisible() const noexcept { return m_OverlayVisible; }

private:
    UIRenderer*             m_Renderer{nullptr};
    const EditorInputState* m_Input{nullptr};
    NF::Game::ChunkMap*     m_ChunkMap{nullptr};

    bool m_OverlayVisible{true}; ///< Toggle for voxel visibility in viewport.

    // Selection state (selected chunk coord index in the loaded list)
    int  m_SelectedChunkIdx{-1};
};

} // namespace NF::Editor
