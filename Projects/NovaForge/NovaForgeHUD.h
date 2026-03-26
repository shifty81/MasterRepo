#pragma once
/**
 * NovaForgeHUD — lightweight immediate-mode OpenGL HUD for the game window.
 *
 * Renders:
 *   1. A real 3-D FPS perspective view of the solar system (bottom layer).
 *   2. Translucent 2-D HUD panels on top (status, log, crosshair, controls).
 *
 * Uses OpenGL 2.1 immediate-mode with manual perspective / lookAt matrices.
 * Text via stb_easy_font (no extra dependencies).
 *
 * Usage:
 *   NovaForge::HUD hud;
 *   // Before calling Draw each frame, set camera and world state:
 *   hud.SetCamera(px, py, pz, yaw, pitch);
 *   hud.SetWorldObjects(objects);
 *   engine.SetRenderCallback([&](int w, int h){ hud.Draw(w, h, state, dt); });
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
#include "External/stb/stb_easy_font.h"

namespace NovaForge {

// ── World object descriptor (passed in per-frame for 3-D rendering) ──────
struct WorldObject {
    float    x = 0.f, y = 0.f, z = 0.f;  // world position
    float    size = 1.f;                   // half-extent of wireframe box
    uint32_t color = 0x4A90D9FF;           // RGBA8888
    std::string name;
};

struct HUDState {
    int         entityCount     = 0;
    int         minerCount      = 0;
    float       totalEarnings   = 0.f;
    int         craftingSessions= 0;
    bool        builderActive   = false;
    bool        playing         = false;
    double      fps             = 0.0;
    std::string sceneName       = "NovaForge_Dev";
    std::vector<std::string>  recentLog;
    std::vector<WorldObject>  worldObjects;  // for 3-D scene render

    // Planet approach state
    struct ApproachTarget {
        std::string name;
        float       distance   = 0.f;
        bool        isGasGiant = false;
        bool        canLand    = false;
    };
    ApproachTarget  nearestBody;
    bool            showApproachPrompt = false;
};

class HUD {
public:
    // ── Camera state (call every frame before Draw) ───────────────────────
    void SetCamera(float x, float y, float z, float yawDeg, float pitchDeg) {
        m_camX = x; m_camY = y; m_camZ = z;
        m_camYaw = yawDeg; m_camPitch = pitchDeg;
    }

    // ── Main draw entry point ─────────────────────────────────────────────
    void Draw(int w, int h, const HUDState& s, double dt = 1.0 / 60.0) {
        // FPS accumulator
        m_fpsFrames++;
        m_fpsAccum += dt;
        if (m_fpsAccum >= 0.5) {
            m_fps = m_fpsFrames / m_fpsAccum;
            m_fpsAccum = 0; m_fpsFrames = 0;
        }

        float W = (float)w, H = (float)h;

        // ── 1.  3-D scene ─────────────────────────────────────────────────
        Draw3DScene(w, h, s);

        // ── 2.  2-D HUD overlay ───────────────────────────────────────────
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

        // FPS crosshair
        DrawCrosshair(W, H);

        // ── Planet approach prompt ────────────────────────────────────────
        if (s.showApproachPrompt) {
            const auto& nb = s.nearestBody;
            char distBuf[64];
            snprintf(distBuf, sizeof(distBuf), "%.0f km", nb.distance);
            std::string line1 = nb.name + "  —  " + distBuf;
            std::string line2 = nb.isGasGiant
                ? "[F] Anchor in Orbit   [G] Deploy Harvest Drone"
                : (nb.canLand ? "[F] Enter Atmosphere  [Esc] Stay in Orbit"
                              : "[Orbit Only — No Landing Zone]");
            float pw = std::max((float)stb_easy_font_width(const_cast<char*>(line1.c_str())),
                                (float)stb_easy_font_width(const_cast<char*>(line2.c_str())));
            pw = pw + 24.f;
            float ph = 44.f;
            float px2 = (W - pw) * 0.5f;
            float py2 = H * 0.72f;
            // Translucent panel
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            Col(0x0A0A1ACC);
            glBegin(GL_QUADS);
            glVertex2f(px2, py2); glVertex2f(px2+pw, py2);
            glVertex2f(px2+pw, py2+ph); glVertex2f(px2, py2+ph);
            glEnd();
            Col(0x4466AAFF);
            glBegin(GL_LINE_LOOP);
            glVertex2f(px2, py2); glVertex2f(px2+pw, py2);
            glVertex2f(px2+pw, py2+ph); glVertex2f(px2, py2+ph);
            glEnd();
            glDisable(GL_BLEND);
            Text(line1, px2 + 12.f, py2 + 6.f,  0xAADDFFFF, 1.1f);
            Text(line2, px2 + 12.f, py2 + 24.f, 0x88AADDFF, 1.0f);
        }

        // Title bar
        Rect(0, 0, W, 28, 0x1A1A2ECC);
        Rect(0, 27, W, 1,  0x3A3A6AFF);
        Text("NovaForge  v0.1", 10, 8, 0x569CD6FF, 1.3f);
        {
            char fpsBuf[32];
            snprintf(fpsBuf, sizeof(fpsBuf), "%.0f FPS", s.fps > 0 ? s.fps : m_fps);
            int ftw = TW(fpsBuf);
            Text(fpsBuf, W - ftw - 10, 8, 0x4EC94EFF, 1.1f);
        }

        // Status bar (bottom)
        float sbY = H - 22.f;
        Rect(0, sbY, W, 22, 0x007ACCCC);
        {
            char statusBuf[128];
            snprintf(statusBuf, sizeof(statusBuf),
                     "  Scene: %s  |  Entities: %d  |  Miners: %d  |  Earnings: %.0f  |  %s",
                     s.sceneName.c_str(), s.entityCount, s.minerCount,
                     s.totalEarnings, s.builderActive ? "BUILDER MODE" : "Ready");
            Text(statusBuf, 4, sbY + 5, 0xFFFFFFFF);
        }

        // Side info panel (right)
        float panX = W - 220.f, panY = 36.f, panW = 216.f;
        Rect(panX, panY, panW, 120, 0x1E1E2ECC);
        RectOutline(panX, panY, panW, 120, 0x3A3A6AFF);
        Text("  GAME STATUS", panX + 6, panY + 4,  0x569CD6FF, 1.1f);
        Line(panX, panY + 18, panX + panW, panY + 18, 0x3A3A6AFF);
        {
            char buf[64];
            float ly = panY + 22.f;
            snprintf(buf, sizeof(buf), "Entities:  %d", s.entityCount);
            Text(buf, panX + 8, ly, 0xD4D4D4FF); ly += 16;
            snprintf(buf, sizeof(buf), "AI Miners: %d", s.minerCount);
            Text(buf, panX + 8, ly, 0xD4D4D4FF); ly += 16;
            snprintf(buf, sizeof(buf), "Earnings:  %.0f cr", s.totalEarnings);
            Text(buf, panX + 8, ly, 0x4EC94EFF); ly += 16;
            snprintf(buf, sizeof(buf), "Builder:   %s", s.builderActive ? "ON" : "off");
            Text(buf, panX + 8, ly, s.builderActive ? 0xFFCC00FF : 0x808080FF);
        }

        // Controls help (bottom-right)
        float helpX = W - 220.f, helpY = sbY - 96.f;
        Rect(helpX, helpY, 216, 92, 0x1E1E2ECC);
        RectOutline(helpX, helpY, 216, 92, 0x3A3A6AFF);
        Text("  CONTROLS", helpX + 6, helpY + 4, 0x808080FF, 1.0f);
        Line(helpX, helpY + 16, helpX + 216, helpY + 16, 0x3A3A6AFF);
        {
            float hy = helpY + 20.f;
            Text("ESC  — Quit",             helpX + 8, hy, 0x808080FF); hy += 13;
            Text("WASD — Move (FPS)",        helpX + 8, hy, 0x808080FF); hy += 13;
            Text("Mouse — Look",             helpX + 8, hy, 0x808080FF); hy += 13;
            Text("F5   — Quick Save",        helpX + 8, hy, 0x808080FF); hy += 13;
            Text("F9   — Quick Load",        helpX + 8, hy, 0x808080FF); hy += 13;
            Text("F2   — Builder Mode",      helpX + 8, hy, 0x808080FF);
        }

        // Recent log panel (bottom-left)
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
    }

    void AppendLog(const std::string& line) {
        m_log.push_back(line);
        if (m_log.size() > 50) m_log.erase(m_log.begin());
    }

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
        int start = std::max(0, (int)m_log.size() - 6);
        s.recentLog = std::vector<std::string>(m_log.begin() + start, m_log.end());
        return s;
    }

private:
    // Camera state for 3-D render
    float m_camX = 0.f, m_camY = 1.7f, m_camZ = 0.f;
    float m_camYaw = 0.f, m_camPitch = 0.f;

    std::vector<std::string> m_log;
    double m_fpsAccum  = 0.0;
    int    m_fpsFrames = 0;
    double m_fps       = 0.0;

    // ── 3-D scene renderer ────────────────────────────────────────────────
    void Draw3DScene(int w, int h, const HUDState& s) {
        glViewport(0, 0, w, h);

        // Clear colour (deep space black)
        glClearColor(0.02f, 0.02f, 0.06f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_CULL_FACE);

        // ── Perspective projection ────────────────────────────────────────
        float aspect = (h > 0) ? (float)w / h : 1.f;
        glMatrixMode(GL_PROJECTION);
        FPSLoadPerspective(70.f, aspect, 0.1f, 50000.f);

        // ── FPS camera view matrix ────────────────────────────────────────
        glMatrixMode(GL_MODELVIEW);
        FPSLoadViewMatrix(m_camX, m_camY, m_camZ, m_camYaw, m_camPitch);

        // ── Star field (dots) ─────────────────────────────────────────────
        DrawStarfield();

        // ── World objects: spheres for celestial bodies, boxes for ships ─
        for (auto& obj : s.worldObjects) {
            float r = (float)((obj.color >> 24) & 0xFF) / 255.f;
            float g = (float)((obj.color >> 16) & 0xFF) / 255.f;
            float b = (float)((obj.color >>  8) & 0xFF) / 255.f;
            // Determine if this is a celestial body by size
            if (obj.size >= 2.0f) {
                // Large = planet / star — draw filled sphere
                int sl = (obj.size >= 4.f) ? 20 : 16;
                int st = (obj.size >= 4.f) ? 12 : 8;
                DrawFilledSphere3D(obj.x, obj.y, obj.z, obj.size, r, g, b, 0.95f, sl, st);
            } else if (obj.size >= 1.5f) {
                // Station — wireframe sphere
                glColor4f(r, g, b, 0.9f);
                const float pi = 3.14159265f;
                for (int pl = 0; pl < 3; pl++) {
                    glBegin(GL_LINE_LOOP);
                    for (int seg = 0; seg < 16; seg++) {
                        float a2 = 2.f * pi * seg / 16;
                        float s2 = std::sin(a2) * obj.size, c2 = std::cos(a2) * obj.size;
                        if (pl==0) glVertex3f(obj.x+c2, obj.y+s2, obj.z);
                        else if (pl==1) glVertex3f(obj.x+c2, obj.y, obj.z+s2);
                        else glVertex3f(obj.x, obj.y+s2, obj.z+c2);
                    }
                    glEnd();
                }
            } else {
                // Small = ship or asteroid — wireframe box
                glColor4f(r, g, b, 0.9f);
                DrawWireBox3D(obj.x, obj.y, obj.z, obj.size);
            }
        }

        // Restore GL state for 2-D HUD
        glDisable(GL_DEPTH_TEST);
    }

    // FPS perspective (column-major for glLoadMatrixf)
    static void FPSLoadPerspective(float fovDeg, float aspect, float nearZ, float farZ) {
        float f  = 1.0f / std::tan(fovDeg * 0.5f * 3.14159265f / 180.f);
        float nf = 1.0f / (nearZ - farZ);
        float m[16] = {
            f / aspect, 0.f, 0.f,                        0.f,
            0.f,        f,   0.f,                        0.f,
            0.f,        0.f, (farZ + nearZ) * nf,       -1.f,
            0.f,        0.f, 2.0f * farZ * nearZ * nf,   0.f
        };
        glLoadMatrixf(m);
    }

    // FPS view matrix from position + yaw/pitch
    static void FPSLoadViewMatrix(float px, float py, float pz,
                                   float yawDeg, float pitchDeg) {
        float yR = yawDeg   * 3.14159265f / 180.f;
        float pR = pitchDeg * 3.14159265f / 180.f;

        // Forward = where we're looking
        float fx = std::cos(pR) * std::sin(yR);
        float fy = std::sin(pR);
        float fz = std::cos(pR) * std::cos(yR);

        // Camera looks at eye + forward
        float cx = px + fx, cy = py + fy, cz = pz + fz;

        // right = cross(forward, up=(0,1,0))
        float rx = -fz, ry = 0.f, rz = fx;
        float rl = std::sqrt(rx*rx + rz*rz);
        if (rl < 1e-6f) rl = 1.f;
        rx /= rl; rz /= rl;

        // true up = cross(right, forward)
        float ux = ry*fz - rz*fy;
        float uy = rz*fx - rx*fz;
        float uz = rx*fy - ry*fx;

        float m[16] = {
            rx,                       ux,                       -fx,                   0.f,
            ry,                       uy,                       -fy,                   0.f,
            rz,                       uz,                       -fz,                   0.f,
            -(rx*px+ry*py+rz*pz),    -(ux*px+uy*py+uz*pz),     fx*px+fy*py+fz*pz,    1.f
        };
        glLoadMatrixf(m);
    }

    // Draw a filled sphere using lat/lon subdivision
    static void DrawFilledSphere3D(float cx, float cy, float cz, float radius,
                                    float r, float g, float b, float a,
                                    int slices = 16, int stacks = 10) {
        const float pi = 3.14159265f;
        for (int i = 0; i < stacks; i++) {
            float lat0 = pi * (-0.5f + (float) i      / stacks);
            float lat1 = pi * (-0.5f + (float)(i + 1) / stacks);
            float sinL0 = std::sin(lat0), cosL0 = std::cos(lat0);
            float sinL1 = std::sin(lat1), cosL1 = std::cos(lat1);
            float shade = 0.65f + 0.35f * (float)(i + 1) / stacks;
            glColor4f(r * shade, g * shade, b * shade, a);
            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= slices; j++) {
                float lng    = 2.f * pi * (float)j / slices;
                float cosLng = std::cos(lng), sinLng = std::sin(lng);
                glVertex3f(cx + radius * cosL0 * cosLng, cy + radius * sinL0, cz + radius * cosL0 * sinLng);
                glVertex3f(cx + radius * cosL1 * cosLng, cy + radius * sinL1, cz + radius * cosL1 * sinLng);
            }
            glEnd();
        }
    }

    // Draw a sparse starfield using GL_POINTS
    static void DrawStarfield() {
        // Fixed pseudo-random star positions
        static float stars[150*3];
        static bool  init = false;
        if (!init) {
            init = true;
            // Simple LCG for deterministic star positions
            unsigned seed = 0xDEADBEEF;
            auto rng = [&]() -> float {
                seed = seed * 1664525u + 1013904223u;
                return ((seed >> 8) & 0xFFFF) / (float)0xFFFF;
            };
            for (int i = 0; i < 150; i++) {
                float theta = rng() * 6.28318f;
                float phi   = (rng() - 0.5f) * 3.14159f;
                float r     = 8000.f;
                stars[i*3+0] = r * std::cos(phi) * std::sin(theta);
                stars[i*3+1] = r * std::sin(phi);
                stars[i*3+2] = r * std::cos(phi) * std::cos(theta);
            }
        }
        glPointSize(2.f);
        glColor4f(0.9f, 0.9f, 1.0f, 0.8f);
        glBegin(GL_POINTS);
        for (int i = 0; i < 150; i++)
            glVertex3f(stars[i*3], stars[i*3+1], stars[i*3+2]);
        glEnd();
        glPointSize(1.f);
    }

    // Wireframe box centred at (x,y,z) with half-extent hs
    static void DrawWireBox3D(float x, float y, float z, float hs) {
        glBegin(GL_LINE_LOOP);
        glVertex3f(x-hs, y-hs, z-hs); glVertex3f(x+hs, y-hs, z-hs);
        glVertex3f(x+hs, y-hs, z+hs); glVertex3f(x-hs, y-hs, z+hs);
        glEnd();
        glBegin(GL_LINE_LOOP);
        glVertex3f(x-hs, y+hs, z-hs); glVertex3f(x+hs, y+hs, z-hs);
        glVertex3f(x+hs, y+hs, z+hs); glVertex3f(x-hs, y+hs, z+hs);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(x-hs, y-hs, z-hs); glVertex3f(x-hs, y+hs, z-hs);
        glVertex3f(x+hs, y-hs, z-hs); glVertex3f(x+hs, y+hs, z-hs);
        glVertex3f(x+hs, y-hs, z+hs); glVertex3f(x+hs, y+hs, z+hs);
        glVertex3f(x-hs, y-hs, z+hs); glVertex3f(x-hs, y+hs, z+hs);
        glEnd();
    }

    // Draw FPS crosshair at screen centre
    static void DrawCrosshair(float W, float H) {
        float cx = W * 0.5f, cy = H * 0.5f;
        float len = 10.f, gap = 4.f;
        glColor4f(1.f, 1.f, 1.f, 0.85f);
        glLineWidth(1.5f);
        glBegin(GL_LINES);
        glVertex2f(cx - len, cy); glVertex2f(cx - gap, cy);
        glVertex2f(cx + gap, cy); glVertex2f(cx + len, cy);
        glVertex2f(cx, cy - len); glVertex2f(cx, cy - gap);
        glVertex2f(cx, cy + gap); glVertex2f(cx, cy + len);
        glEnd();
        // Centre dot
        glPointSize(3.f);
        glBegin(GL_POINTS);
        glVertex2f(cx, cy);
        glEnd();
        glPointSize(1.f);
        glLineWidth(1.f);
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
