#include "Editor/Gizmos/GizmoSystem.h"
#include <algorithm>
#include <cstring>

namespace Editor {

// ──────────────────────────────────────────────────────────────
// GizmoSystem
// ──────────────────────────────────────────────────────────────

void GizmoSystem::SetGizmoType(GizmoType type)   { m_state.type  = type; }
void GizmoSystem::SetGizmoSpace(GizmoSpace space) { m_state.space = space; }

void GizmoSystem::SelectEntity(uint32_t entityId, const float pos[3],
                                const float rot[4], const float scl[3]) {
    m_state.entityId = entityId;
    std::copy_n(pos, 3, m_state.position);
    std::copy_n(rot, 4, m_state.rotation);
    std::copy_n(scl, 3, m_state.scale);
}

void GizmoSystem::Deselect() { m_state = {}; }

bool GizmoSystem::BeginDrag(int axis) {
    if (!HasSelection()) return false;
    m_state.dragging   = true;
    m_state.activeAxis = axis;
    std::copy_n(m_state.position, 3, m_dragStart);
    return true;
}

void GizmoSystem::UpdateDrag(float dx, float dy, float dz) {
    if (!m_state.dragging) return;
    switch (m_state.type) {
    case GizmoType::Translate:
    case GizmoType::Universal:
        if (m_state.activeAxis == 0 || m_state.activeAxis < 0) m_state.position[0] += dx;
        if (m_state.activeAxis == 1 || m_state.activeAxis < 0) m_state.position[1] += dy;
        if (m_state.activeAxis == 2 || m_state.activeAxis < 0) m_state.position[2] += dz;
        break;
    case GizmoType::Scale:
        if (m_state.activeAxis == 0 || m_state.activeAxis < 0) m_state.scale[0] += dx;
        if (m_state.activeAxis == 1 || m_state.activeAxis < 0) m_state.scale[1] += dy;
        if (m_state.activeAxis == 2 || m_state.activeAxis < 0) m_state.scale[2] += dz;
        break;
    default: break;
    }
    if (m_changedFn) m_changedFn(m_state.entityId, m_state);
}

void GizmoSystem::EndDrag() {
    m_state.dragging   = false;
    m_state.activeAxis = -1;
}

bool GizmoSystem::HandleKey(char key) {
    switch (key) {
    case 'w': case 'W': SetGizmoType(GizmoType::Translate); return true;
    case 'e': case 'E': SetGizmoType(GizmoType::Rotate);    return true;
    case 'r': case 'R': SetGizmoType(GizmoType::Scale);     return true;
    }
    return false;
}

void GizmoSystem::SetTransformChangedCallback(TransformChangedFn fn) {
    m_changedFn = std::move(fn);
}

// ──────────────────────────────────────────────────────────────
// OverlaySystem
// ──────────────────────────────────────────────────────────────

OverlayEntry* OverlaySystem::find(uint32_t id) {
    for (auto& e : m_entries) if (e.id == id) return &e;
    return nullptr;
}

uint32_t OverlaySystem::AddEntry(const std::string& label, float x, float y,
                                  uint32_t color) {
    OverlayEntry e; e.id = m_nextID++; e.label = label;
    e.x = x; e.y = y; e.color = color;
    m_entries.push_back(std::move(e));
    return m_entries.back().id;
}

void OverlaySystem::RemoveEntry(uint32_t id) {
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
        [id](const OverlayEntry& e){ return e.id == id; }), m_entries.end());
}

void OverlaySystem::UpdateValue(uint32_t id, const std::string& value) {
    if (auto* e = find(id)) e->value = value;
}

void OverlaySystem::SetVisible(uint32_t id, bool visible) {
    if (auto* e = find(id)) e->visible = visible;
}

void OverlaySystem::Clear() { m_entries.clear(); }

void OverlaySystem::SetFPS(float fps) {
    if (!m_fpsID) m_fpsID = AddEntry("FPS", 8.f, 8.f, 0xFFFF00FF);
    UpdateValue(m_fpsID, std::to_string((int)fps));
}

void OverlaySystem::SetEntityCount(int32_t count) {
    if (!m_entityID) m_entityID = AddEntry("Entities", 8.f, 28.f, 0x00FF00FF);
    UpdateValue(m_entityID, std::to_string(count));
}

void OverlaySystem::SetMemoryMB(float mb) {
    if (!m_memID) m_memID = AddEntry("Mem MB", 8.f, 48.f, 0x00FFFFFF);
    UpdateValue(m_memID, std::to_string((int)mb));
}

} // namespace Editor
