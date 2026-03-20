#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

namespace Editor {

// ──────────────────────────────────────────────────────────────
// Gizmo types
// ──────────────────────────────────────────────────────────────

enum class GizmoType { None, Translate, Rotate, Scale, Universal };
enum class GizmoSpace { World, Local };

struct GizmoState {
    GizmoType  type       = GizmoType::None;
    GizmoSpace space      = GizmoSpace::World;
    uint32_t   entityId   = 0;
    float      position[3]= {0,0,0};
    float      rotation[4]= {0,0,0,1}; // quaternion
    float      scale[3]   = {1,1,1};
    bool       dragging   = false;
    int        activeAxis = -1; // 0=X, 1=Y, 2=Z, -1=none
};

// ──────────────────────────────────────────────────────────────
// GizmoSystem
// ──────────────────────────────────────────────────────────────

class GizmoSystem {
public:
    void SetGizmoType(GizmoType type);
    void SetGizmoSpace(GizmoSpace space);
    void SelectEntity(uint32_t entityId, const float pos[3],
                      const float rot[4], const float scl[3]);
    void Deselect();

    const GizmoState& State() const { return m_state; }
    GizmoType  Type()  const { return m_state.type; }
    GizmoSpace Space() const { return m_state.space; }
    bool HasSelection() const { return m_state.entityId != 0; }

    // Called from input handling
    bool BeginDrag(int axis);
    void UpdateDrag(float deltaX, float deltaY, float deltaZ);
    void EndDrag();
    bool IsDragging() const { return m_state.dragging; }

    // Callbacks
    using TransformChangedFn = std::function<void(uint32_t entityId, const GizmoState& s)>;
    void SetTransformChangedCallback(TransformChangedFn fn);

    // Keyboard shortcut dispatch (W=Translate, E=Rotate, R=Scale)
    bool HandleKey(char key);

private:
    GizmoState          m_state;
    TransformChangedFn  m_changedFn;
    float               m_dragStart[3] = {};
};

// ──────────────────────────────────────────────────────────────
// OverlaySystem — diagnostic overlays on the viewport
// ──────────────────────────────────────────────────────────────

struct OverlayEntry {
    uint32_t    id      = 0;
    std::string label;
    float       x       = 0.f;
    float       y       = 0.f;
    uint32_t    color   = 0xFFFFFFFF; // RGBA
    bool        visible = true;
    std::string value;
};

class OverlaySystem {
public:
    uint32_t AddEntry(const std::string& label, float x, float y,
                      uint32_t color = 0xFFFFFFFF);
    void     RemoveEntry(uint32_t id);
    void     UpdateValue(uint32_t id, const std::string& value);
    void     SetVisible(uint32_t id, bool visible);
    void     Clear();

    const std::vector<OverlayEntry>& Entries() const { return m_entries; }

    // Stats overlays
    void SetFPS(float fps);
    void SetEntityCount(int32_t count);
    void SetMemoryMB(float mb);

private:
    uint32_t m_nextID      = 1;
    uint32_t m_fpsID       = 0;
    uint32_t m_entityID    = 0;
    uint32_t m_memID       = 0;
    std::vector<OverlayEntry> m_entries;

    OverlayEntry* find(uint32_t id);
};

} // namespace Editor
