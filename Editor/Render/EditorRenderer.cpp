#include "Editor/Render/EditorRenderer.h"
#include <GL/gl.h>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdio>

// ── stb_easy_font (single header, public domain) ──────────────────────────
#define STB_EASY_FONT_IMPLEMENTATION
#include "External/stb/stb_easy_font.h"

namespace Editor {

// ── colour helpers ─────────────────────────────────────────────────────────
static inline uint8_t R(uint32_t c) { return (c >> 24) & 0xFF; }
static inline uint8_t G(uint32_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t B(uint32_t c) { return (c >>  8) & 0xFF; }
static inline uint8_t A(uint32_t c) { return (c      ) & 0xFF; }

void EditorRenderer::SetColor(uint32_t rgba) {
    glColor4ub(R(rgba), G(rgba), B(rgba), A(rgba));
}

// ── Dark editor palette ────────────────────────────────────────────────────
static constexpr uint32_t kBgBase        = 0x1E1E1EFF;
static constexpr uint32_t kBgPanel       = 0x252526FF;
static constexpr uint32_t kBgTitleBar    = 0x323233FF;
static constexpr uint32_t kBgMenuBar     = 0x2D2D30FF;
static constexpr uint32_t kBgHeader      = 0x3C3C3CFF;
static constexpr uint32_t kBgViewport    = 0x1A1A2EFF;
static constexpr uint32_t kBgConsole     = 0x1C1C1CFF;
static constexpr uint32_t kBgStatusBar   = 0x007ACCFF;
static constexpr uint32_t kBorderColor   = 0x3F3F46FF;
static constexpr uint32_t kTextNormal    = 0xD4D4D4FF;
static constexpr uint32_t kTextMuted     = 0x808080FF;
static constexpr uint32_t kTextAccent    = 0x569CD6FF;
static constexpr uint32_t kTextWarn      = 0xFFCC00FF;
static constexpr uint32_t kTextSuccess   = 0x4EC94EFF;
static constexpr uint32_t kTextError     = 0xF44747FF;
static constexpr uint32_t kGridLine      = 0x2A2A4AFF;
static constexpr uint32_t kGridLineMain  = 0x3A3A6AFF;
static constexpr uint32_t kAxisX        = 0xE05555FF;
static constexpr uint32_t kAxisY        = 0x55BB55FF;
static constexpr uint32_t kAxisZ        = 0x5588E0FF;
static constexpr uint32_t kHighlight    = 0x007ACCFF;

// ── Layout constants ───────────────────────────────────────────────────────
static constexpr float kTitleH    = 26.f;
static constexpr float kMenuH     = 22.f;
static constexpr float kStatusH   = 22.f;
static constexpr float kConsoleH  = 160.f;
static constexpr float kOutlinerW = 220.f;
static constexpr float kInspectorW= 240.f;
static constexpr float kPanelHdrH = 20.f;

// ──────────────────────────────────────────────────────────────────────────
bool EditorRenderer::Init(int width, int height) {
    m_width  = width;
    m_height = height;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Populate dummy scene objects for the outliner / viewport
    m_sceneObjects = {
        {0.f, 0.f, 80.f, 40.f, 0x4A90D9FF, "PlayerShip"},
        {0.f, 0.f, 60.f, 30.f, 0x7ED321FF, "Station_Hull"},
        {0.f, 0.f, 50.f, 25.f, 0xF5A623FF, "Engine_L"},
        {0.f, 0.f, 50.f, 25.f, 0xF5A623FF, "Engine_R"},
        {0.f, 0.f, 40.f, 20.f, 0xBD10E0FF, "Turret_01"},
        {0.f, 0.f, 40.f, 20.f, 0xBD10E0FF, "Turret_02"},
        {0.f, 0.f, 70.f, 35.f, 0xE05555FF, "AsteroidCluster"},
    };

    m_consoleLines.push_back("[Info]  AtlasEditor v0.1 initialized");
    m_consoleLines.push_back("[Info]  OpenGL renderer active");
    m_consoleLines.push_back("[Info]  ECS world ready — 0 entities");
    m_consoleLines.push_back("[Info]  PCG pipeline online");
    m_consoleLines.push_back("[Info]  AI agent system standby");
    m_consoleLines.push_back("[Info]  Ready.");

    return true;
}

void EditorRenderer::Resize(int width, int height) {
    m_width  = width;
    m_height = height;
    glViewport(0, 0, width, height);
}

EditorRenderer::~EditorRenderer() { Shutdown(); }
void EditorRenderer::Shutdown() {}

// ── Input forwarding ───────────────────────────────────────────────────────
void EditorRenderer::OnMouseMove  (double x, double y) { m_mouseX = x; m_mouseY = y; }
void EditorRenderer::OnMouseButton(int /*btn*/, bool /*pressed*/) {}
void EditorRenderer::OnKey        (int /*key*/, bool /*pressed*/) {}

void EditorRenderer::AppendConsole(const std::string& line) {
    m_consoleLines.push_back(line);
    if ((int)m_consoleLines.size() > kMaxConsoleLines)
        m_consoleLines.erase(m_consoleLines.begin());
}

// ──────────────────────────────────────────────────────────────────────────
// SetupOrtho — maps pixel coords (0,0)=top-left, (w,h)=bottom-right
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::SetupOrtho() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // glOrtho(left, right, bottom, top, near, far)
    // We want y=0 at the top, y=m_height at the bottom
    glOrtho(0.0, m_width, m_height, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// ──────────────────────────────────────────────────────────────────────────
// Drawing primitives
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawRect(float x, float y, float w, float h, uint32_t fill) {
    SetColor(fill);
    glBegin(GL_QUADS);
    glVertex2f(x,     y    );
    glVertex2f(x + w, y    );
    glVertex2f(x + w, y + h);
    glVertex2f(x,     y + h);
    glEnd();
}

void EditorRenderer::DrawRectOutline(float x, float y, float w, float h,
                                      uint32_t col, float lw) {
    glLineWidth(lw);
    SetColor(col);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x,     y    );
    glVertex2f(x + w, y    );
    glVertex2f(x + w, y + h);
    glVertex2f(x,     y + h);
    glEnd();
    glLineWidth(1.f);
}

void EditorRenderer::DrawLine(float x0, float y0, float x1, float y1,
                               uint32_t col, float lw) {
    glLineWidth(lw);
    SetColor(col);
    glBegin(GL_LINES);
    glVertex2f(x0, y0);
    glVertex2f(x1, y1);
    glEnd();
    glLineWidth(1.f);
}

// Text via stb_easy_font — builds quad vertex buffer, renders as GL_QUADS
void EditorRenderer::DrawText(const std::string& text, float x, float y,
                               uint32_t col, float scale) {
    if (text.empty()) return;

    static char vbuf[65536];
    unsigned char c[4] = { R(col), G(col), B(col), A(col) };
    int quads = stb_easy_font_print(0.f, 0.f,
                                    const_cast<char*>(text.c_str()),
                                    c, vbuf, sizeof(vbuf));

    glPushMatrix();
    glTranslatef(x, y, 0.f);
    glScalef(scale, scale, 1.f);

    SetColor(col);
    glEnableClientState(GL_VERTEX_ARRAY);
    // stb_easy_font vertex layout: x(4), y(4), z(4), color(4) = 16 bytes/vertex
    glVertexPointer(2, GL_FLOAT, 16, vbuf);
    glDrawArrays(GL_QUADS, 0, quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();
}

int EditorRenderer::TextWidth(const std::string& text, float scale) {
    return static_cast<int>(stb_easy_font_width(const_cast<char*>(text.c_str())) * scale);
}

// ──────────────────────────────────────────────────────────────────────────
// Panel header helper
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawPanelHeader(const char* title,
                                      float x, float y, float w, float h,
                                      uint32_t headerCol) {
    DrawRect(x, y, w, h, headerCol);
    DrawText(title, x + 6.f, y + 4.f, kTextNormal, 1.1f);
}

// ──────────────────────────────────────────────────────────────────────────
// Title bar
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawTitleBar(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgTitleBar);

    // App name left
    DrawText("AtlasEditor  v0.1", x + 8.f, y + 6.f, kTextNormal, 1.2f);

    // Window controls right (decorative)
    float btnW = 14.f, btnH = 14.f;
    float btnY = y + (h - btnH) * 0.5f;
    // Minimise
    DrawRect(w - 50.f, btnY, btnW, btnH, 0x444444FF);
    DrawText("-", w - 47.f, btnY + 2.f, kTextNormal);
    // Maximise
    DrawRect(w - 32.f, btnY, btnW, btnH, 0x444444FF);
    DrawText("O", w - 30.f, btnY + 2.f, kTextNormal);
    // Close
    DrawRect(w - 14.f, btnY, btnW, btnH, 0xE04040FF);
    DrawText("X", w - 12.f, btnY + 2.f, 0xFFFFFFFF);
}

// ──────────────────────────────────────────────────────────────────────────
// Menu bar
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawMenuBar(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgMenuBar);
    DrawLine(x, y + h - 1.f, x + w, y + h - 1.f, kBorderColor);

    static const char* menus[] = {
        "File", "Edit", "View", "Tools", "Scene", "AI", "Build", "Help"
    };
    float cx = x + 8.f;
    for (auto* m : menus) {
        int tw = TextWidth(m, 1.f);
        // hover highlight
        bool hovered = (m_mouseX >= cx - 4 && m_mouseX < cx + tw + 4 &&
                        m_mouseY >= y       && m_mouseY < y + h);
        if (hovered)
            DrawRect(cx - 4.f, y + 1.f, tw + 8.f, h - 2.f, 0x3E3E52FF);
        DrawText(m, cx, y + 5.f, hovered ? 0xFFFFFFFF : kTextNormal);
        cx += tw + 20.f;
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Viewport — dark 3D area with grid + dummy objects
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawViewport(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgViewport);
    DrawPanelHeader("  Viewport  [Camera: Perspective]", x, y, w, kPanelHdrH, kBgHeader);

    float vx = x, vy = y + kPanelHdrH, vw = w, vh = h - kPanelHdrH;

    // Clip content to viewport rect
    glScissor((int)vx, (int)(m_height - vy - vh), (int)vw, (int)vh);
    glEnable(GL_SCISSOR_TEST);

    DrawViewportGrid(vx, vy, vw, vh);

    // ── Dummy 3D objects (projected flat) ─────────────────────────────────
    float cx = vx + vw * 0.5f;
    float cy = vy + vh * 0.5f;

    // Large station hull
    float hullW = vw * 0.30f, hullH = vh * 0.20f;
    DrawRect(cx - hullW * 0.5f, cy - hullH * 0.5f, hullW, hullH, 0x2C4A6EFF);
    DrawRectOutline(cx - hullW * 0.5f, cy - hullH * 0.5f, hullW, hullH, kTextAccent, 1.5f);
    DrawText("Station_Hull", cx - hullW * 0.5f + 4.f, cy - hullH * 0.5f + 4.f, kTextAccent);

    // Port engine
    float engW = vw * 0.08f, engH = vh * 0.07f;
    DrawRect(cx - hullW * 0.5f - engW - 4.f, cy - engH * 0.5f, engW, engH, 0x3A3A1EFF);
    DrawRectOutline(cx - hullW * 0.5f - engW - 4.f, cy - engH * 0.5f, engW, engH, kTextWarn, 1.f);
    DrawText("Eng_L", cx - hullW * 0.5f - engW - 2.f, cy - engH * 0.5f + 2.f, kTextWarn);

    // Starboard engine
    DrawRect(cx + hullW * 0.5f + 4.f, cy - engH * 0.5f, engW, engH, 0x3A3A1EFF);
    DrawRectOutline(cx + hullW * 0.5f + 4.f, cy - engH * 0.5f, engW, engH, kTextWarn, 1.f);
    DrawText("Eng_R", cx + hullW * 0.5f + 6.f, cy - engH * 0.5f + 2.f, kTextWarn);

    // Turrets
    float tW = vw * 0.05f, tH = vh * 0.05f;
    DrawRect(cx - tW * 0.5f - tW * 1.5f, cy - hullH * 0.5f - tH - 2.f, tW, tH, 0x4A1E4AFF);
    DrawRectOutline(cx - tW * 0.5f - tW * 1.5f, cy - hullH * 0.5f - tH - 2.f, tW, tH, 0xBD10E0FF, 1.f);
    DrawRect(cx - tW * 0.5f + tW * 0.5f, cy - hullH * 0.5f - tH - 2.f, tW, tH, 0x4A1E4AFF);
    DrawRectOutline(cx - tW * 0.5f + tW * 0.5f, cy - hullH * 0.5f - tH - 2.f, tW, tH, 0xBD10E0FF, 1.f);

    // Selected entity highlight
    DrawRectOutline(cx - hullW * 0.5f - 3.f, cy - hullH * 0.5f - 3.f,
                    hullW + 6.f, hullH + 6.f, 0xFFCC00FF, 1.5f);

    // Coordinate axes (bottom-left of viewport)
    DrawViewportAxes(vx, vy + vh);

    glDisable(GL_SCISSOR_TEST);

    // Viewport overlay text
    DrawText("RMB: orbit  MMB: pan  Scroll: zoom", vx + 6.f, vy + vh - 18.f, kTextMuted);

    char fpsBuf[32];
    snprintf(fpsBuf, sizeof(fpsBuf), "%.0f FPS", m_fps);
    int tw = TextWidth(fpsBuf);
    DrawText(fpsBuf, vx + vw - tw - 8.f, vy + kPanelHdrH + 4.f, kTextSuccess);
}

void EditorRenderer::DrawViewportGrid(float vpX, float vpY, float vpW, float vpH) {
    float cx = vpX + vpW * 0.5f;
    float cy = vpY + vpH * 0.5f;

    // Minor grid — every 30px
    for (float gx = cx; gx < vpX + vpW; gx += 30.f)
        DrawLine(gx, vpY, gx, vpY + vpH, kGridLine);
    for (float gx = cx - 30.f; gx > vpX; gx -= 30.f)
        DrawLine(gx, vpY, gx, vpY + vpH, kGridLine);
    for (float gy = cy; gy < vpY + vpH; gy += 30.f)
        DrawLine(vpX, gy, vpX + vpW, gy, kGridLine);
    for (float gy = cy - 30.f; gy > vpY; gy -= 30.f)
        DrawLine(vpX, gy, vpX + vpW, gy, kGridLine);

    // Major grid — every 150px
    for (float gx = cx; gx < vpX + vpW; gx += 150.f)
        DrawLine(gx, vpY, gx, vpY + vpH, kGridLineMain, 1.5f);
    for (float gx = cx - 150.f; gx > vpX; gx -= 150.f)
        DrawLine(gx, vpY, gx, vpY + vpH, kGridLineMain, 1.5f);
    for (float gy = cy; gy < vpY + vpH; gy += 150.f)
        DrawLine(vpX, gy, vpX + vpW, gy, kGridLineMain, 1.5f);
    for (float gy = cy - 150.f; gy > vpY; gy -= 150.f)
        DrawLine(vpX, gy, vpX + vpW, gy, kGridLineMain, 1.5f);
}

void EditorRenderer::DrawViewportAxes(float vpX, float vpBottom) {
    float ox = vpX + 40.f;
    float oy = vpBottom - 40.f;
    float len = 22.f;

    DrawLine(ox, oy, ox + len, oy,       kAxisX, 2.f);
    DrawText("X", ox + len + 2.f, oy - 5.f, kAxisX);

    DrawLine(ox, oy, ox, oy - len,       kAxisY, 2.f);
    DrawText("Y", ox - 3.f, oy - len - 12.f, kAxisY);

    DrawLine(ox, oy, ox + len * 0.6f, oy - len * 0.6f, kAxisZ, 2.f);
    DrawText("Z", ox + len * 0.6f + 2.f, oy - len * 0.6f - 10.f, kAxisZ);
}

// ──────────────────────────────────────────────────────────────────────────
// Scene Outliner
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawOutliner(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgPanel);
    DrawPanelHeader("  Scene Outliner", x, y, w, kPanelHdrH, kBgHeader);

    static const char* icons[] = {"[G]", "[M]", "[M]", "[M]", "[T]", "[T]", "[A]"};
    static const char* names[] = {
        "PlayerShip", "Station_Hull", "Engine_L", "Engine_R",
        "Turret_01", "Turret_02", "AsteroidCluster"
    };
    static const uint32_t colors[] = {
        0x4A90D9FF, 0x7ED321FF, 0xF5A623FF, 0xF5A623FF,
        0xBD10E0FF, 0xBD10E0FF, 0xE05555FF
    };

    float iy = y + kPanelHdrH + 2.f;
    float rowH = 18.f;
    constexpr int kCount = 7;
    for (int i = 0; i < kCount; i++) {
        bool selected = (i == 1); // Station_Hull selected
        if (selected)
            DrawRect(x, iy, w, rowH, 0x094771FF);
        else if (m_mouseX >= x && m_mouseX < x + w &&
                 m_mouseY >= iy && m_mouseY < iy + rowH)
            DrawRect(x, iy, w, rowH, 0x2A2D2EFF);

        // indent for children
        float indent = (i >= 2 && i <= 5) ? 12.f : 0.f;
        if (i >= 2 && i <= 5)
            DrawLine(x + 8.f, iy, x + 8.f, iy + rowH, kBorderColor);

        DrawText(icons[i], x + 4.f + indent, iy + 3.f, colors[i]);
        DrawText(names[i], x + 28.f + indent, iy + 3.f,
                 selected ? 0xFFFFFFFF : kTextNormal);
        iy += rowH;
    }

    // Bottom: entity count
    DrawLine(x, y + h - 20.f, x + w, y + h - 20.f, kBorderColor);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d entities", kCount);
    DrawText(buf, x + 4.f, y + h - 16.f, kTextMuted);
}

// ──────────────────────────────────────────────────────────────────────────
// Inspector
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawInspector(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgPanel);
    DrawPanelHeader("  Inspector", x, y, w, kPanelHdrH, kBgHeader);

    float cy = y + kPanelHdrH + 6.f;
    float lh = 18.f;

    auto Row = [&](const char* label, const char* value, uint32_t valCol = kTextAccent) {
        DrawText(label, x + 6.f, cy, kTextMuted);
        DrawText(value, x + w * 0.45f, cy, valCol);
        cy += lh;
    };

    // Entity header
    DrawRect(x + 4.f, cy, w - 8.f, 20.f, 0x094771FF);
    DrawText("Station_Hull", x + 8.f, cy + 3.f, 0xFFFFFFFF, 1.1f);
    DrawText("[enabled]", x + w - 68.f, cy + 3.f, kTextSuccess);
    cy += 24.f;

    // Transform component
    DrawRect(x + 4.f, cy, w - 8.f, 16.f, kBgHeader);
    DrawText("Transform", x + 8.f, cy + 2.f, kTextWarn);
    cy += 18.f;

    Row("Position",  "0.0, 0.0, 0.0");
    Row("Rotation",  "0.0, 0.0, 0.0");
    Row("Scale",     "1.0, 1.0, 1.0");
    cy += 4.f;

    // Mesh component
    DrawRect(x + 4.f, cy, w - 8.f, 16.f, kBgHeader);
    DrawText("Mesh", x + 8.f, cy + 2.f, kTextWarn);
    cy += 18.f;

    Row("Asset",     "hull_v2.obj");
    Row("LOD",       "Auto");
    Row("Visible",   "true",        kTextSuccess);
    cy += 4.f;

    // Physics
    DrawRect(x + 4.f, cy, w - 8.f, 16.f, kBgHeader);
    DrawText("RigidBody", x + 8.f, cy + 2.f, kTextWarn);
    cy += 18.f;
    Row("Mass",      "1200.0 kg");
    Row("Friction",  "0.35");
    Row("Kinematic", "false",       0xF5A623FF);
    cy += 4.f;

    // Tags
    DrawRect(x + 4.f, cy, w - 8.f, 16.f, kBgHeader);
    DrawText("Tags", x + 8.f, cy + 2.f, kTextWarn);
    cy += 18.f;
    DrawText("[Structure] [Snappable]", x + 6.f, cy, 0xBD10E0FF);
    cy += lh;

    // Add Component button
    cy = y + h - 28.f;
    bool hovBtn = (m_mouseX >= x + 6 && m_mouseX < x + w - 6 &&
                   m_mouseY >= cy    && m_mouseY < cy + 20);
    DrawRect(x + 6.f, cy, w - 12.f, 20.f, hovBtn ? 0x1177BBFF : 0x007ACCFF);
    DrawText("+ Add Component", x + w * 0.25f, cy + 4.f, 0xFFFFFFFF);
}

// ──────────────────────────────────────────────────────────────────────────
// Console
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawConsole(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgConsole);
    DrawPanelHeader("  Console  Output", x, y, w, kPanelHdrH, kBgHeader);

    glScissor((int)x, (int)(m_height - y - h), (int)w, (int)(h - kPanelHdrH - 20.f));
    glEnable(GL_SCISSOR_TEST);

    float lineH = 14.f;
    float fy = y + h - 22.f - lineH; // start from bottom

    // Show last N lines that fit
    int visLines = (int)((h - kPanelHdrH - 22.f) / lineH);
    int start = std::max(0, (int)m_consoleLines.size() - visLines);
    for (int i = (int)m_consoleLines.size() - 1; i >= start && fy > y + kPanelHdrH; i--) {
        const std::string& line = m_consoleLines[i];
        uint32_t col = kTextNormal;
        if (line.find("[Error]")   != std::string::npos) col = kTextError;
        else if (line.find("[Warn]")  != std::string::npos) col = kTextWarn;
        else if (line.find("[Info]")  != std::string::npos) col = kTextAccent;
        else if (line.find("[Debug]") != std::string::npos) col = kTextMuted;
        DrawText(line, x + 4.f, fy, col);
        fy -= lineH;
    }
    glDisable(GL_SCISSOR_TEST);

    // Input line
    DrawLine(x, y + h - 20.f, x + w, y + h - 20.f, kBorderColor);
    DrawRect(x + 2.f, y + h - 19.f, w - 4.f, 18.f, 0x141414FF);
    DrawText("> _", x + 6.f, y + h - 16.f, kTextAccent);
}

// ──────────────────────────────────────────────────────────────────────────
// Status bar
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawStatusBar(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgStatusBar);

    DrawText("  Ready  |  Scene: NovaForge_Dev  |  Entities: 7  |  PCG: idle",
             x + 4.f, y + 5.f, 0xFFFFFFFF);

    char buf[64];
    snprintf(buf, sizeof(buf), "%.0f FPS  |  GL 2.1  ", m_fps);
    int tw = TextWidth(buf);
    DrawText(buf, x + w - tw - 4.f, y + 5.f, 0xFFFFFFFF);
}

// ──────────────────────────────────────────────────────────────────────────
// Main Render
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::Render(double dt) {
    // FPS counter
    m_fpsAccum += dt;
    m_fpsFrames++;
    if (m_fpsAccum >= 0.5) {
        m_fps = m_fpsFrames / m_fpsAccum;
        m_fpsAccum  = 0.0;
        m_fpsFrames = 0;
    }

    float W = (float)m_width;
    float H = (float)m_height;

    glViewport(0, 0, m_width, m_height);
    glClearColor(R(kBgBase)/255.f, G(kBgBase)/255.f, B(kBgBase)/255.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    SetupOrtho();

    // ── Layout geometry ────────────────────────────────────────────────────
    float titleY   = 0.f;
    float menuY    = titleY   + kTitleH;
    float statusY  = H        - kStatusH;
    float consoleY = statusY  - kConsoleH;
    float mainY    = menuY    + kMenuH;
    float mainH    = consoleY - mainY;

    float outlinerX  = 0.f;
    float inspectorX = W - kInspectorW;
    float viewportX  = kOutlinerW;
    float viewportW  = W - kOutlinerW - kInspectorW;

    // ── Background ─────────────────────────────────────────────────────────
    DrawRect(0.f, 0.f, W, H, kBgBase);

    // ── Panels ─────────────────────────────────────────────────────────────
    DrawOutliner (outlinerX,  mainY,  kOutlinerW,  mainH);
    DrawViewport (viewportX,  mainY,  viewportW,   mainH);
    DrawInspector(inspectorX, mainY,  kInspectorW, mainH);
    DrawConsole  (0.f,        consoleY, W, kConsoleH);

    // ── Borders between panels ─────────────────────────────────────────────
    DrawLine(kOutlinerW,  mainY, kOutlinerW,  consoleY, kBorderColor, 1.5f);
    DrawLine(inspectorX,  mainY, inspectorX,  consoleY, kBorderColor, 1.5f);
    DrawLine(0.f, consoleY, W, consoleY,                kBorderColor, 1.5f);

    // ── Bars (drawn on top) ────────────────────────────────────────────────
    DrawMenuBar (0.f, menuY,  W, kMenuH);
    DrawTitleBar(0.f, titleY, W, kTitleH);
    DrawStatusBar(0.f, statusY, W, kStatusH);

    // ── Top / bottom hairlines ─────────────────────────────────────────────
    DrawLine(0.f, menuY + kMenuH, W, menuY + kMenuH, kBorderColor);
}

} // namespace Editor
