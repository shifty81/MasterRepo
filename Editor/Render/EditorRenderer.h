#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <memory>
#include <cmath>
#include <future>
#include "Engine/Math/Math.h"
#include "Runtime/Components/Components.h"

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

    // Query PIE state (used by main loop to suppress ESC-close during PIE)
    bool IsPlaying() const { return m_playing; }

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

    // RMB / MMB drag state for camera orbit / pan
    bool   m_rmbDown    = false;
    bool   m_mmbDown    = false;

    // ── 3-D viewport orbit camera ─────────────────────────────────────────
    // RMB drag orbits (yaw / pitch), MMB drag pans target, Scroll zooms.
    float  m_vpYaw    =   35.f;   // horizontal orbit (degrees)
    float  m_vpPitch  =   30.f;   // vertical orbit   (degrees; +up)
    float  m_vpDist   =  200.f;   // orbit distance (world units)
    float  m_vpTargX  =    8.f;   // orbit target world-X
    float  m_vpTargY  =    0.f;   // orbit target world-Y
    float  m_vpTargZ  =    0.f;   // orbit target world-Z

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

    // ── AI Chat dropdown (grows from toolbar button) ─────────────────────
    bool        m_aiDropOpen     = false;  // dropdown visible
    float       m_aiDropBtnX     = 0.f;   // toolbar button left edge (anchor)
    float       m_aiDropBtnY2    = 0.f;   // toolbar button bottom edge (anchor)
    bool        m_aiDropFocused  = false; // input box has focus
    std::string m_aiDropInput;            // input buffer
    float       m_aiDropScrollY  = 0.f;  // chat history scroll offset
    std::future<std::string> m_aiFuture; // async Ollama response
    // Ollama connectivity status (background ping)
    std::future<bool> m_ollamaPingFuture;
    float       m_ollamaPingAccum = 15.f; // start at threshold → immediate first ping
    bool        m_ollamaOk        = false;

    // ── EI-13: PIE (play-in-editor) ─────────────────────────────────────
    bool m_playing = false;

    // ── PIE simulation time ───────────────────────────────────────────────
    float m_pieTime = 0.f;   // accumulates dt when m_playing is true

    // ── PIE FPS camera ────────────────────────────────────────────────────
    float m_pieCamX     = 0.f;
    float m_pieCamY     = 1.7f;
    float m_pieCamZ     = 0.f;
    float m_pieCamYaw   = 0.f;
    float m_pieCamPitch = 0.f;
    // PIE movement key state (held keys)
    bool  m_pieKeyW = false, m_pieKeyA = false;
    bool  m_pieKeyS = false, m_pieKeyD = false;
    bool  m_pieKeyUp = false, m_pieKeyDown = false;

    // ── Gizmo drag undo snapshot ──────────────────────────────────────────
    Runtime::Components::Transform m_gizmoDragStartTransform{};

    // ── Clipboard (Ctrl+C / Ctrl+V / Ctrl+D) ─────────────────────────────
    struct ClipboardEntity {
        Runtime::Components::Tag       tag;
        Runtime::Components::Transform transform;
        bool valid = false;
    };
    ClipboardEntity m_clipboard;

    // ── Keybinds reference panel ──────────────────────────────────────────
    bool m_keybindsVisible = false;

    // ── Panel visibility toggles (View menu) ────────────────────────────
    bool m_outlinerVisible   = true;
    bool m_inspectorVisible  = true;
    bool m_consoleVisible    = true;
    bool m_viewportVisible   = true;
    bool m_contentBrowserVisible = false;

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

    void DrawRect             (float x, float y, float w, float h, uint32_t fill);
    void DrawRectOutline      (float x, float y, float w, float h, uint32_t col, float lw = 1.f);
    void DrawRoundedRect      (float x, float y, float w, float h, float radius, uint32_t fill);
    void DrawRoundedRectOutline(float x, float y, float w, float h, float radius, uint32_t col, float lw = 1.f);
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
    void Draw3DViewportScene (float vx, float vy, float vw, float vh);
    void DrawFloorGrid3D     ();
    void DrawEntities3D      ();
    void DrawGizmo3D         ();
    void DrawViewportAxes    (float vpX, float vpBottom);  // 2-D corner overlay
    void DrawViewportGizmo   (float vpX, float vpY, float vpW, float vpH); // EI-05
    // Removed: void DrawViewportGrid — replaced by DrawFloorGrid3D

    /** Project a 3D world point to viewport pixel coordinates.
     *  Returns false when the point is behind the camera or off-screen. */
    bool Project3D(float wx, float wy, float wz, float& sx, float& sy) const;

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

    void DrawAIDropdown     (float btnX, float btnY2);          // AI chat dropdown overlay
    void DrawKeybindsPanel  (float x, float y, float w, float h);
    void StopPIE();   // stop PIE mode cleanly
};

} // namespace Editor
