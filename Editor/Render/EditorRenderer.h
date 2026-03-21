#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace Editor {

// ──────────────────────────────────────────────────────────────────────────
// EditorRenderer
//
// Draws the Atlas Editor UI using OpenGL 2.1 immediate-mode rendering.
// Panels are drawn as coloured quads; text via stb_easy_font (no texture).
//
// Layout (1280×720 default):
//
//   ┌────────────────────────────────────────────────┐  ← title bar  (24px)
//   ├────────────────────────────────────────────────┤  ← menu bar   (22px)
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
    EditorRenderer() = default;
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

    // Feed lines into the console panel
    void AppendConsole(const std::string& line);

    void Shutdown();

private:
    int    m_width  = 1280;
    int    m_height = 720;
    double m_fps    = 0.0;
    double m_fpsAccum  = 0.0;
    int    m_fpsFrames = 0;

    double m_mouseX = 0, m_mouseY = 0;

    // Console state
    std::vector<std::string> m_consoleLines;
    static constexpr int kMaxConsoleLines = 200;

    // Viewport orbit (simple anim)
    float m_gridAngle = 0.f;

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
    void DrawTitleBar (float x, float y, float w, float h);
    void DrawMenuBar  (float x, float y, float w, float h);
    void DrawViewport (float x, float y, float w, float h);
    void DrawOutliner (float x, float y, float w, float h);
    void DrawInspector(float x, float y, float w, float h);
    void DrawConsole  (float x, float y, float w, float h);
    void DrawStatusBar(float x, float y, float w, float h);
    void DrawPanelHeader(const char* title, float x, float y, float w, float h,
                         uint32_t headerCol);

    // ── 3D viewport helpers ────────────────────────────────────────────────
    void DrawViewportGrid(float vpX, float vpY, float vpW, float vpH);
    void DrawViewportAxes(float vpX, float vpY);

    // Dummy scene objects in viewport
    struct DummyObject {
        float x, y, w, h;
        uint32_t color;
        std::string label;
    };
    std::vector<DummyObject> m_sceneObjects;
};

} // namespace Editor
