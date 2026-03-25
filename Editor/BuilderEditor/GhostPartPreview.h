#pragma once
/**
 * @file GhostPartPreview.h
 * @brief Live ghost-part snap preview for the EditorRenderer viewport.
 *
 * Augments BuilderEditor with a real-time visual preview of where a part
 * will be placed before the player commits.  The ghost is rendered as a
 * translucent / colour-coded overlay:
 *
 *   GREEN  — placement is valid and a snap candidate was found
 *   YELLOW — placement is valid but no snap point was located
 *   RED    — placement is invalid (collision, missing library entry, etc.)
 *
 * Usage:
 * @code
 *   GhostPartPreview ghost;
 *   ghost.Init(&builderEditor, &partLibrary, &snapRules);
 *   // Each frame before rendering:
 *   ghost.UpdateCursorRay(rayOriX, rayOriZ, rayDirX, rayDirZ);
 *   ghost.Tick(dt);
 *   // Renderer queries ghost state:
 *   if (ghost.IsActive()) {
 *       auto& info = ghost.GetGhostInfo();
 *       RenderGhostMesh(info.meshPath, info.transform, info.tint);
 *   }
 * @endcode
 */

#include "Builder/Assembly/Assembly.h"
#include "Builder/Parts/PartDef.h"
#include "Builder/SnapRules/SnapRules.h"
#include "Editor/BuilderEditor/BuilderEditor.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Editor {

// ── Ghost colour states ───────────────────────────────────────────────────────

enum class GhostState : uint8_t {
    Hidden,     ///< no part selected for placement
    Valid,      ///< green — will snap cleanly
    Floating,   ///< yellow — no snap found but position is clear
    Invalid,    ///< red — cannot place here
};

// ── Ghost part information (consumed by renderer) ─────────────────────────────

struct GhostPartInfo {
    uint32_t     partDefId{0};
    std::string  meshPath;
    float        transform[16]{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    GhostState   state{GhostState::Hidden};

    // RGBA tint for ghost mesh
    float        tintR{0.3f}, tintG{1.0f}, tintB{0.3f}, tintA{0.45f};

    // Snap information (valid when state == Valid)
    bool         hasSnap{false};
    uint32_t     snapPartId{0};
    uint32_t     snapPointId{0};
    float        snapPos[3]{0,0,0};

    bool IsVisible() const { return state != GhostState::Hidden; }
};

// ── GhostPartPreview ──────────────────────────────────────────────────────────

class GhostPartPreview {
public:
    void Init(BuilderEditor*        editor,
              Builder::PartLibrary* partLib,
              Builder::SnapRules*   snapRules);

    // ── Part selection ────────────────────────────────────────────────────
    /// Begin previewing a part (called when the player picks from the palette).
    void BeginPreview(uint32_t partDefId);

    /// Stop showing the ghost (called on cancel or after commit).
    void EndPreview();

    bool IsActive() const;

    // ── Per-frame update ──────────────────────────────────────────────────
    /// Supply the cursor ray in world space (from camera unproject).
    void UpdateCursorRay(float originX, float originY, float originZ,
                          float dirX,    float dirY,    float dirZ);

    /// Supply a custom placement transform directly (e.g. snapped to grid).
    void UpdateTransform(const float transform[16]);

    /// Advance snap-search and state evaluation.
    void Tick(float dt);

    // ── Renderer query ────────────────────────────────────────────────────
    const GhostPartInfo& GetGhostInfo() const;

    // ── Snap settings ─────────────────────────────────────────────────────
    void SetSnapSearchRadius(float radius);
    void SetGridSnap(bool enabled, float gridSize = 1.0f);

    // ── Commit ────────────────────────────────────────────────────────────
    /// Commit the current ghost position — calls BuilderEditor::CommitPlacement().
    bool Commit();

private:
    void UpdateState();
    void FindBestSnap();
    void ApplyGridSnap(float& x, float& y, float& z) const;
    void ApplySnapTint();

    BuilderEditor*        m_editor{nullptr};
    Builder::PartLibrary* m_partLib{nullptr};
    Builder::SnapRules*   m_snapRules{nullptr};

    uint32_t    m_partDefId{0};
    bool        m_active{false};
    bool        m_gridSnap{false};
    float       m_gridSize{1.0f};
    float       m_snapRadius{30.0f};

    // Current cursor ray
    float m_rayOriX{0}, m_rayOriY{0}, m_rayOriZ{0};
    float m_rayDirX{0}, m_rayDirY{1}, m_rayDirZ{0};

    GhostPartInfo m_ghost;
};

} // namespace Editor
