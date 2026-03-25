#include "Editor/BuilderEditor/GhostPartPreview.h"
#include <algorithm>
#include <cmath>

namespace Editor {

void GhostPartPreview::Init(BuilderEditor*        editor,
                             Builder::PartLibrary* partLib,
                             Builder::SnapRules*   snapRules) {
    m_editor    = editor;
    m_partLib   = partLib;
    m_snapRules = snapRules;
    m_active    = false;
    m_ghost     = GhostPartInfo{};
}

// ── Part selection ────────────────────────────────────────────────────────────

void GhostPartPreview::BeginPreview(uint32_t partDefId) {
    m_partDefId         = partDefId;
    m_active            = true;
    m_ghost             = GhostPartInfo{};
    m_ghost.partDefId   = partDefId;
    m_ghost.state       = GhostState::Floating;
    if (m_partLib) {
        const Builder::PartDef* def = m_partLib->Get(partDefId);
        if (def) m_ghost.meshPath = def->meshPath;
    }
    if (m_editor) m_editor->BeginPlacement(partDefId);
}

void GhostPartPreview::EndPreview() {
    m_active      = false;
    m_ghost.state = GhostState::Hidden;
    if (m_editor) m_editor->CancelPlacement();
}

bool GhostPartPreview::IsActive() const { return m_active; }

// ── Per-frame update ──────────────────────────────────────────────────────────

void GhostPartPreview::UpdateCursorRay(float ox, float oy, float oz,
                                        float dx, float dy, float dz) {
    m_rayOriX = ox; m_rayOriY = oy; m_rayOriZ = oz;
    m_rayDirX = dx; m_rayDirY = dy; m_rayDirZ = dz;

    // Intersect ray with Y=0 plane (simple ground/build plane)
    // t = -originY / dirY
    if (std::abs(dy) < 1e-6f) return;
    float t = -oy / dy;
    if (t < 0.0f) return;
    float hitX = ox + dx * t;
    float hitY = 0.0f;
    float hitZ = oz + dz * t;

    if (m_gridSnap) ApplyGridSnap(hitX, hitY, hitZ);

    // Build identity transform at hit position
    float transform[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    transform[12] = hitX;
    transform[13] = hitY;
    transform[14] = hitZ;
    UpdateTransform(transform);
}

void GhostPartPreview::UpdateTransform(const float transform[16]) {
    for (int i = 0; i < 16; ++i) m_ghost.transform[i] = transform[i];
    if (m_editor) m_editor->UpdatePlacementTransform(transform);
}

void GhostPartPreview::Tick(float /*dt*/) {
    if (!m_active) return;
    FindBestSnap();
    UpdateState();
    ApplySnapTint();
}

// ── Renderer query ────────────────────────────────────────────────────────────

const GhostPartInfo& GhostPartPreview::GetGhostInfo() const { return m_ghost; }

// ── Snap settings ─────────────────────────────────────────────────────────────

void GhostPartPreview::SetSnapSearchRadius(float r)  { m_snapRadius = r; }
void GhostPartPreview::SetGridSnap(bool e, float gs) { m_gridSnap = e; m_gridSize = gs; }

// ── Commit ────────────────────────────────────────────────────────────────────

bool GhostPartPreview::Commit() {
    if (!m_active || m_ghost.state == GhostState::Invalid) return false;
    if (m_editor) m_editor->CommitPlacement();
    EndPreview();
    return true;
}

// ── Private helpers ───────────────────────────────────────────────────────────

void GhostPartPreview::FindBestSnap() {
    m_ghost.hasSnap = false;
    if (!m_editor || !m_partLib || !m_snapRules) return;
    Builder::Assembly* asm_ = m_editor->GetAssembly();
    if (!asm_ || asm_->PartCount() == 0) return;

    const Builder::PartDef* newDef = m_partLib->Get(m_partDefId);
    if (!newDef || newDef->snapPoints.empty()) return;

    const Builder::SnapPoint& newSnap = newDef->snapPoints[0];

    // Ghost world position
    float gx = m_ghost.transform[12];
    float gz = m_ghost.transform[14];

    float bestDist = m_snapRadius * m_snapRadius;
    for (const auto& pp : asm_->GetAllParts()) {
        const Builder::PartDef* def = m_partLib->Get(pp.defId);
        if (!def) continue;
        // Check snap compatibility
        auto candidates = m_snapRules->FindCompatibleSnaps(*def, newSnap);
        for (const auto& cand : candidates) {
            // Find the actual snap point on the placed part by ID
            const Builder::SnapPoint* sp = nullptr;
            for (const auto& point : def->snapPoints)
                if (point.id == cand.snapId) { sp = &point; break; }
            if (!sp) continue;
            // Snap point world position = part transform * snap localPos
            float spx = pp.transform[12] + sp->localPos[0];
            float spz = pp.transform[14] + sp->localPos[2];
            float dx = gx - spx;
            float dz = gz - spz;
            float dist2 = dx*dx + dz*dz;
            if (dist2 < bestDist) {
                bestDist              = dist2;
                m_ghost.hasSnap       = true;
                m_ghost.snapPartId    = pp.instanceId;
                m_ghost.snapPointId   = cand.snapId;
                m_ghost.snapPos[0]    = spx;
                m_ghost.snapPos[1]    = pp.transform[13];
                m_ghost.snapPos[2]    = spz;

                // Snap the ghost transform to the snap position
                m_ghost.transform[12] = spx;
                m_ghost.transform[13] = m_ghost.snapPos[1];
                m_ghost.transform[14] = spz;
            }
        }
    }
}

void GhostPartPreview::UpdateState() {
    if (!m_editor) { m_ghost.state = GhostState::Invalid; return; }
    // Use BuilderEditor::Validate() to check the current preview
    bool valid = m_editor->Validate();
    if (!valid)            m_ghost.state = GhostState::Invalid;
    else if (m_ghost.hasSnap) m_ghost.state = GhostState::Valid;
    else                   m_ghost.state = GhostState::Floating;
}

void GhostPartPreview::ApplySnapTint() {
    switch (m_ghost.state) {
    case GhostState::Valid:
        m_ghost.tintR = 0.2f; m_ghost.tintG = 1.0f; m_ghost.tintB = 0.2f; m_ghost.tintA = 0.5f;
        break;
    case GhostState::Floating:
        m_ghost.tintR = 1.0f; m_ghost.tintG = 0.9f; m_ghost.tintB = 0.1f; m_ghost.tintA = 0.45f;
        break;
    case GhostState::Invalid:
        m_ghost.tintR = 1.0f; m_ghost.tintG = 0.15f; m_ghost.tintB = 0.15f; m_ghost.tintA = 0.5f;
        break;
    default:
        break;
    }
}

void GhostPartPreview::ApplyGridSnap(float& x, float& /*y*/, float& z) const {
    x = std::round(x / m_gridSize) * m_gridSize;
    z = std::round(z / m_gridSize) * m_gridSize;
}

} // namespace Editor
