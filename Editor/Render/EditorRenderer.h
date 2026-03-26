#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <memory>
#include <cmath>
#include "Engine/Math/Math.h"

// Forward declarations to avoid heavy headers in the interface
namespace Runtime::ECS  { class World; }
namespace Editor        { class ErrorPanel; }
namespace IDE           { class CodeEditor; class AIChat; }
namespace Editor        { class ContentBrowser; }

namespace Editor {

// ──────────────────────────────────────────────────────────────────────────
// EditorRenderer
//
// Draws the Atlas Editor UI using OpenGL 2.1 immediate-mode rendering.
// Panels are drawn as coloured quads; text via stb_easy_font (no texture).
//
// Layout (1280×720 default):
//
//   ┌────────────────────────────────────────────────┐
//   ├───────────────────────────────── toolbar ──────┤  ← menu+toolbar (50px)
//   ├──────────┬─────────────────────┬───────────────┤
//   │  Scene   │                     │               │
//   │ Outliner │     VIEWPORT        │  Inspector    │
//   │  (220px) │     (centre)        │   (240px)     │
//   ├──────────┴─────────────────────┴───────────────┤
//   │               Console  (160px)                 │
//   ├────────────────────────────────────────────────┤  ← status bar (22px)
//   └────────────────────────────────────────────────┘
// ──────────────────────────────────────────────────────────────────────────
class EditorRenderer {
public:
    EditorRenderer();
    ~EditorRenderer();

    // Call once after the GL context is current
    bool Init(int width, int height);

    // Call every frame
    void Render(double dt);

    // Call when the framebuffer is resized
    void Resize(int width, int height);

    // Input
    void OnMouseMove  (double x, double y);
    void OnMouseButton(int btn, bool pressed);
    void OnKey        (int key, bool pressed);
    void OnChar       (unsigned int codepoint);
    void OnScroll     (double dx, double dy);

    // Feed lines into the console panel  (EI-01)
    void AppendConsole(const std::string& line);

    // EI-02: Wire the live ECS world into the renderer
    void SetWorld(Runtime::ECS::World* world);

    void Shutdown();

private:
    int    m_width  = 1280;
    int    m_height = 720;
    double m_fps    = 0.0;
    double m_fpsAccum  = 0.0;
    int    m_fpsFrames = 0;

    double m_mouseX = 0, m_mouseY = 0;
    bool   m_leftMousePressed = false;
    bool   m_ctrlHeld         = false;

    // RMB / MMB drag state for camera pan
    bool   m_rmbDown    = false;
    bool   m_mmbDown    = false;

    // Camera pan offset and zoom (applied in viewport rendering)
    float  m_camOffX    = 0.f;
    float  m_camOffY    = 0.f;
    float  m_camZoom    = 1.f;

    // Text-input focus (console input bar or AI chat input)
    bool   m_consoleFocused  = false;
    bool   m_aiInputFocused  = false;
    std::string m_consoleInput;         // console command being typed

    // Console state
    std::vector<std::string> m_consoleLines;
    static constexpr int kMaxConsoleLines = 200;

    // Viewport orbit (simple anim)
    float m_gridAngle = 0.f;

    // ── EI-02: ECS world ──────────────────────────────────────────────────
    Runtime::ECS::World* m_world = nullptr;

    // ── EI-03: entity selection ──────────────────────────────────────────
    uint32_t m_selectedEntity = 0; // 0 = none
    // Last computed viewport bounds (for click-to-pick)
    float m_vpX = 0.f, m_vpY = 0.f, m_vpW = 0.f, m_vpH = 0.f;

    // ── EI-06: content browser ───────────────────────────────────────────
    std::unique_ptr<ContentBrowser> m_contentBrowser;

    // ── EI-09: error panel ───────────────────────────────────────────────
    std::unique_ptr<ErrorPanel> m_errorPanel;

    // ── EI-12: embedded code editor ─────────────────────────────────────
    std::unique_ptr<IDE::CodeEditor> m_codeEditor;
    bool m_codeEditorVisible = false;

    // ── EI-14: AI chat ───────────────────────────────────────────────────
    std::unique_ptr<IDE::AIChat> m_aiChat;
    bool m_aiChatVisible     = false;
    std::string m_aiInput;          // current input line being typed

    // ── EI-13: PIE (play-in-editor) ─────────────────────────────────────
    bool m_playing = false;

    // ── Gizmo drag (from ideas123gui.md) ─────────────────────────────────
    // Gizmo mode: 0=Move, 1=Rotate, 2=Scale
    int  m_gizmoMode     = 0;
    // Drag state: which axis is being dragged (0=none, 1=X, 2=Y, 3=Z)
    int  m_gizmoDragAxis = 0;
    bool m_gizmoDragging = false;
    double m_dragLastX   = 0.0, m_dragLastY = 0.0;

    // ── Grid snapping (from ideas123gui.md) ──────────────────────────────
    bool  m_gridSnap     = false;
    float m_gridSize     = 1.0f;

    // ── Add-component popup state ────────────────────────────────────────
    bool m_addCompMenuOpen       = false;
    bool m_addCompMenuJustOpened = false;   // suppress same-frame close

    // ── Menu bar click tracking ──────────────────────────────────────────
    // which top-level menu is open (-1 = none)
    int  m_activeMenu    = -1;
    // sub-item hover
    int  m_menuHoverItem = -1;

    // ── drawing helpers ────────────────────────────────────────────────────
    void SetupOrtho();

    // Packed RGBA (0xRRGGBBAA)
    static void SetColor(uint32_t rgba);

    void DrawRect        (float x, float y, float w, float h, uint32_t fill);
    void DrawRectOutline (float x, float y, float w, float h, uint32_t col, float lw = 1.f);
    void DrawLine        (float x0, float y0, float x1, float y1, uint32_t col, float lw = 1.f);
    void DrawText        (const std::string& text, float x, float y,
                          uint32_t col = 0xFFFFFFFF, float scale = 1.f);
    int  TextWidth       (const std::string& text, float scale = 1.f);

    // ── panel draw methods ─────────────────────────────────────────────────
    void DrawTitleBar   (float x, float y, float w, float h);
    void DrawMenuBar    (float x, float y, float w, float h);
    void DrawToolbar    (float x, float y, float w, float h);   // EI-13 play/stop
    void DrawViewport   (float x, float y, float w, float h);
    void DrawOutliner   (float x, float y, float w, float h);
    void DrawInspector  (float x, float y, float w, float h);
    void DrawConsole    (float x, float y, float w, float h);
    void DrawStatusBar  (float x, float y, float w, float h);
    void DrawContentBrowser(float x, float y, float w, float h);  // EI-06
    void DrawErrorPanel (float x, float y, float w, float h);     // EI-09
    void DrawCodeEditor (float x, float y, float w, float h);     // EI-12
    void DrawAIChat     (float x, float y, float w, float h);     // EI-14
    void DrawPanelHeader(const char* title, float x, float y, float w, float h,
                         uint32_t headerCol);
    void DrawAddCompMenu(float x, float y, float w, float h);    // add-component popup

    // ── 3D viewport helpers ────────────────────────────────────────────────
    void DrawViewportGrid (float vpX, float vpY, float vpW, float vpH);
    void DrawViewportAxes (float vpX, float vpY);
    void DrawViewportGizmo(float vpX, float vpY, float vpW, float vpH); // EI-05

    // EI-03: pick entity at screen coord inside viewport
    uint32_t PickEntityAt(float screenX, float screenY);

    // Gizmo helpers — returns true if mouse is over the given axis arrow
    bool GizmoAxisHit(float ex, float ey, int axis, float gLen) const;

    // Apply grid snapping to a value
    float SnapToGrid(float v) const;

    // EI-08: launch build and forward output to logger/error panel
    void TriggerBuild();

    // Scene save / load
    void SaveScene(const std::string& path);
    void LoadScene(const std::string& path);

    // Helper: get entity display name (Tag.name or "Entity #N")
    std::string EntityName(uint32_t id) const;
};

} // namespace Editor
