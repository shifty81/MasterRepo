#pragma once
/**
 * NovaForgeHUD — lightweight immediate-mode OpenGL HUD for the game window.
 *
 * Uses the same OpenGL 2.1 immediate-mode approach as EditorRenderer so no
 * additional dependencies are needed.  Text is rendered via stb_easy_font.
 *
 * Usage:
 *   NovaForgeHUD hud;
 *   engine.SetRenderCallback([&](int w, int h){ hud.Draw(w, h, state); });
 */

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  undef DrawText
#endif
#include <GL/gl.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cmath>
#include <algorithm>

// ── stb_easy_font ─────────────────────────────────────────────────────────
// Only define the implementation once per translation unit.
#define STB_EASY_FONT_IMPLEMENTATION
#include "External/stb/stb_easy_font.h"

namespace NovaForge {

struct HUDState {
    int         entityCount     = 0;
    int         minerCount      = 0;
    float       totalEarnings   = 0.f;
    int         craftingSessions= 0;
    bool        builderActive   = false;
    bool        playing         = false;   // PIE / play mode
    double      fps             = 0.0;
    std::string sceneName       = "NovaForge_Dev";
    std::vector<std::string> recentLog;    // last few log lines
};

class HUD {
public:
    // Call every render frame.  w/h are the framebuffer dimensions.
    void Draw(int w, int h, const HUDState& s) {
        // Accumulate FPS
        m_fpsFrames++;
        m_fpsAccum += 1.0 / 60.0; // approximation; real dt not available here
        if (m_fpsAccum >= 0.5) {
            m_fps = m_fpsFrames / m_fpsAccum;
            m_fpsAccum = 0; m_fpsFrames = 0;
        }

        // ── Setup 2-D ortho ──────────────────────────────────────────────
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, w, h, 0.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        float W = (float)w, H = (float)h;

        // ── Title bar ────────────────────────────────────────────────────
        Rect(0, 0, W, 28, 0x1A1A2EFF);
        Rect(0, 27, W, 1,  0x3A3A6AFF);
        Text("NovaForge  v0.1", 10, 8, 0x569CD6FF, 1.3f);
        char fpsBuf[32];
        snprintf(fpsBuf, sizeof(fpsBuf), "%.0f FPS", s.fps > 0 ? s.fps : m_fps);
        int ftw = TW(fpsBuf);
        Text(fpsBuf, W - ftw - 10, 8, 0x4EC94EFF, 1.1f);

        // ── Status bar (bottom) ──────────────────────────────────────────
        float sbY = H - 22.f;
        Rect(0, sbY, W, 22, 0x007ACCFF);
        char statusBuf[128];
        snprintf(statusBuf, sizeof(statusBuf),
                 "  Scene: %s  |  Entities: %d  |  Miners: %d  |  Earnings: %.0f  |  %s",
                 s.sceneName.c_str(), s.entityCount, s.minerCount,
                 s.totalEarnings, s.builderActive ? "BUILDER MODE" : "Ready");
        Text(statusBuf, 4, sbY + 5, 0xFFFFFFFF);

        // ── Side info panel (right) ───────────────────────────────────────
        float panX = W - 220.f, panY = 36.f, panW = 216.f;
        Rect(panX, panY, panW, 120, 0x1E1E2EDD);
        RectOutline(panX, panY, panW, 120, 0x3A3A6AFF);
        Text("  GAME STATUS", panX + 6, panY + 4,  0x569CD6FF, 1.1f);
        Line(panX, panY + 18, panX + panW, panY + 18, 0x3A3A6AFF);

        char buf[64];
        float ly = panY + 22.f;
        snprintf(buf, sizeof(buf), "Entities:    %d", s.entityCount);
        Text(buf, panX + 8, ly, 0xD4D4D4FF); ly += 16;
        snprintf(buf, sizeof(buf), "AI Miners:   %d", s.minerCount);
        Text(buf, panX + 8, ly, 0xD4D4D4FF); ly += 16;
        snprintf(buf, sizeof(buf), "Earnings:    %.0f cr", s.totalEarnings);
        Text(buf, panX + 8, ly, 0x4EC94EFF); ly += 16;
        snprintf(buf, sizeof(buf), "Crafting:    %d sessions", s.craftingSessions);
        Text(buf, panX + 8, ly, 0xD4D4D4FF); ly += 16;
        snprintf(buf, sizeof(buf), "Builder:     %s", s.builderActive ? "ON" : "off");
        Text(buf, panX + 8, ly, s.builderActive ? 0xFFCC00FF : 0x808080FF);

        // ── Controls help (bottom-right) ─────────────────────────────────
        float helpX = W - 220.f, helpY = sbY - 82.f;
        Rect(helpX, helpY, 216, 78, 0x1E1E2ECC);
        RectOutline(helpX, helpY, 216, 78, 0x3A3A6AFF);
        Text("  CONTROLS", helpX + 6, helpY + 4, 0x808080FF, 1.0f);
        Line(helpX, helpY + 16, helpX + 216, helpY + 16, 0x3A3A6AFF);
        float hy = helpY + 20.f;
        Text("ESC  — Quit",          helpX + 8, hy, 0x808080FF); hy += 13;
        Text("F5   — Quick Save",    helpX + 8, hy, 0x808080FF); hy += 13;
        Text("F9   — Quick Load",    helpX + 8, hy, 0x808080FF); hy += 13;
        Text("F2   — Builder Mode",  helpX + 8, hy, 0x808080FF); hy += 13;
        Text("Tab  — Inventory",     helpX + 8, hy, 0x808080FF);

        // ── Recent log panel (bottom-left) ───────────────────────────────
        if (!s.recentLog.empty()) {
            float logH = (float)std::min((int)s.recentLog.size(), 6) * 14.f + 8.f;
            float logY = sbY - logH - 4.f;
            Rect(4, logY, W * 0.55f, logH, 0x1C1C1CCC);
            RectOutline(4, logY, W * 0.55f, logH, 0x3F3F46FF);
            float ry = logY + 4.f;
            int start = std::max(0, (int)s.recentLog.size() - 6);
            for (int i = start; i < (int)s.recentLog.size(); i++) {
                Text(s.recentLog[i], 8, ry, 0xA0A0A0FF); ry += 14;
            }
        }

        // ── Centre viewport decoration ───────────────────────────────────
        float cx = W * 0.5f - 100.f, cy = 38.f;
        Rect(cx, cy, 200.f, H - 60.f - logHeightApprox(s) - 22.f, 0x0D0D1AFF);
        RectOutline(cx, cy, 200.f, H - 60.f - logHeightApprox(s) - 22.f, 0x2A2A4AFF);
        Text("  3D VIEWPORT", cx + 4, cy + 4, 0x2A2A4AFF, 1.0f);
        Text("[Game world renders here]", cx + 8, cy + 24, 0x3A3A6AFF);
        Text("[Waiting for 3D renderer]", cx + 8, cy + 40, 0x2A3A5AFF);
    }

    // Feed new log lines in so they appear in the HUD
    void AppendLog(const std::string& line) {
        m_log.push_back(line);
        if (m_log.size() > 50) m_log.erase(m_log.begin());
    }

    // Build a HUDState snapshot — call before Draw()
    HUDState BuildState(int entityCount, int minerCount,
                        float earnings, int crafting,
                        bool builder, double fps,
                        const std::string& scene = "NovaForge_Dev") const {
        HUDState s;
        s.entityCount      = entityCount;
        s.minerCount       = minerCount;
        s.totalEarnings    = earnings;
        s.craftingSessions = crafting;
        s.builderActive    = builder;
        s.fps              = fps;
        s.sceneName        = scene;
        // Last 6 lines
        int start = std::max(0, (int)m_log.size() - 6);
        s.recentLog = std::vector<std::string>(m_log.begin() + start, m_log.end());
        return s;
    }

private:
    std::vector<std::string> m_log;
    double m_fpsAccum  = 0.0;
    int    m_fpsFrames = 0;
    double m_fps       = 0.0;

    static float logHeightApprox(const HUDState& s) {
        int cnt = std::min((int)s.recentLog.size(), 6);
        return cnt > 0 ? cnt * 14.f + 12.f : 0.f;
    }

    // ── Colour helpers ────────────────────────────────────────────────────
    static void Col(uint32_t rgba) {
        glColor4ub((rgba>>24)&0xFF,(rgba>>16)&0xFF,(rgba>>8)&0xFF,rgba&0xFF);
    }
    void Rect(float x, float y, float w, float h, uint32_t fill) {
        Col(fill);
        glBegin(GL_QUADS);
        glVertex2f(x,     y    );
        glVertex2f(x + w, y    );
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
        glEnd();
    }
    void RectOutline(float x, float y, float w, float h, uint32_t col) {
        Col(col);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x,     y    );
        glVertex2f(x + w, y    );
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
        glEnd();
    }
    void Line(float x0, float y0, float x1, float y1, uint32_t col) {
        Col(col);
        glBegin(GL_LINES);
        glVertex2f(x0, y0); glVertex2f(x1, y1);
        glEnd();
    }
    void Text(const std::string& txt, float x, float y,
              uint32_t col, float scale = 1.f) {
        if (txt.empty()) return;
        static char vb[65536];
        unsigned char c[4] = { (unsigned char)((col>>24)&0xFF),
                                (unsigned char)((col>>16)&0xFF),
                                (unsigned char)((col>> 8)&0xFF),
                                (unsigned char)( col     &0xFF) };
        int q = stb_easy_font_print(0, 0, const_cast<char*>(txt.c_str()),
                                    c, vb, sizeof(vb));
        glPushMatrix();
        glTranslatef(x, y, 0);
        glScalef(scale, scale, 1);
        Col(col);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 16, vb);
        glDrawArrays(GL_QUADS, 0, q * 4);
        glDisableClientState(GL_VERTEX_ARRAY);
        glPopMatrix();
    }
    int TW(const std::string& t, float scale = 1.f) {
        return (int)(stb_easy_font_width(const_cast<char*>(t.c_str())) * scale);
    }
};

} // namespace NovaForge
