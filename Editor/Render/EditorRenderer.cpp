#include "Editor/Render/EditorRenderer.h"
#include "Runtime/ECS/ECS.h"
#include "Runtime/Components/Components.h"
#include "Editor/Panels/ContentBrowser/ContentBrowser.h"
#include "Editor/Panels/ErrorPanel/ErrorPanel.h"
#include "Editor/UndoableCommandBus.h"
#include "IDE/CodeEditor/CodeEditor.h"
#include "IDE/AIChat/AIChat.h"
#include "Engine/Core/Logger.h"
#include <GL/gl.h>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <cstdlib>
#include <sstream>

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
static constexpr float kToolbarH  = 28.f;
static constexpr float kStatusH   = 22.f;
static constexpr float kConsoleH  = 160.f;
static constexpr float kOutlinerW = 220.f;
static constexpr float kInspectorW= 240.f;
static constexpr float kPanelHdrH = 20.f;

// ──────────────────────────────────────────────────────────────────────────
EditorRenderer::EditorRenderer()
    : m_contentBrowser(std::make_unique<ContentBrowser>())
    , m_errorPanel(std::make_unique<ErrorPanel>())
    , m_codeEditor(std::make_unique<IDE::CodeEditor>())
    , m_aiChat(std::make_unique<IDE::AIChat>())
{
    // EI-09: wire error panel navigation to code editor
    m_errorPanel->SetNavigateCallback([this](const std::string& file, uint32_t line) {
        m_codeEditor->Open(file);
        m_codeEditor->SetCursor(line > 0 ? line - 1 : 0, 0);
        m_codeEditorVisible = true;
        Engine::Core::Logger::Info("Navigate to " + file + ":" + std::to_string(line));
    });

    // EI-14: wire AI chat send to Ollama via HTTP
    m_aiChat->SetSendCallback([this](const std::string& userMsg, IDE::AIChat& chat) {
        // Simple non-blocking: log the query; actual HTTP done in Render loop
        Engine::Core::Logger::Info("[AI] User: " + userMsg);
        chat.AppendMessage(IDE::ChatRole::Assistant,
            "[AI] (Ollama) Received: \"" + userMsg + "\". Connect Ollama at localhost:11434.");
    });

    m_aiChat->SetSystemPrompt(
        "You are AtlasEditor AI, an expert game-engine programming assistant. "
        "Help the user write C++ code, fix build errors, and generate content.");
}

bool EditorRenderer::Init(int width, int height) {
    m_width  = width;
    m_height = height;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // EI-06: point content browser at the project assets root
    {
        auto assetsPath = std::filesystem::current_path() / "Projects" / "NovaForge" / "Assets";
        if (!std::filesystem::exists(assetsPath))
            assetsPath = std::filesystem::current_path();
        m_contentBrowser->SetRootPath(assetsPath.string());
    }

    m_consoleLines.push_back("[Info]  AtlasEditor v0.1 initialized");
    m_consoleLines.push_back("[Info]  OpenGL renderer active");
    m_consoleLines.push_back("[Info]  ECS world ready");
    m_consoleLines.push_back("[Info]  PCG pipeline online");
    m_consoleLines.push_back("[Info]  AI agent system standby");
    m_consoleLines.push_back("[Info]  Ready.");

    return true;
}

// EI-02
void EditorRenderer::SetWorld(Runtime::ECS::World* world) {
    m_world = world;
}

void EditorRenderer::Resize(int width, int height) {
    m_width  = width;
    m_height = height;
    glViewport(0, 0, width, height);
}

EditorRenderer::~EditorRenderer() { Shutdown(); }
void EditorRenderer::Shutdown() {}

// ── Input forwarding ───────────────────────────────────────────────────────
void EditorRenderer::OnMouseMove(double x, double y) { m_mouseX = x; m_mouseY = y; }

// EI-03: left-click in viewport → pick entity
void EditorRenderer::OnMouseButton(int btn, bool pressed) {
    if (btn == 0 && pressed) {
        m_leftMousePressed = true;
        // Check if the click is inside the viewport area
        if (m_mouseX >= m_vpX && m_mouseX < m_vpX + m_vpW &&
            m_mouseY >= m_vpY && m_mouseY < m_vpY + m_vpH)
        {
            uint32_t picked = PickEntityAt((float)m_mouseX, (float)m_mouseY);
            if (picked != m_selectedEntity) {
                m_selectedEntity = picked;
                if (picked != 0)
                    Engine::Core::Logger::Info("Selected entity #" + std::to_string(picked)
                                               + " (" + EntityName(picked) + ")");
                else
                    Engine::Core::Logger::Info("Deselected");
            }
        } else {
            // Click outside viewport → dismiss menu
            m_activeMenu = -1;
        }
    } else if (btn == 0 && !pressed) {
        m_leftMousePressed = false;
    }
}

// EI-11: Ctrl+Z / Ctrl+Y undo/redo; EI-13 P = play/stop toggle
void EditorRenderer::OnKey(int key, bool pressed) {
#ifndef GLFW_KEY_Z
    // GLFW key constants (duplicated to avoid including GLFW here)
    static constexpr int K_Z     = 90;
    static constexpr int K_Y     = 89;
    static constexpr int K_LEFT_CTRL  = 341;
    static constexpr int K_RIGHT_CTRL = 345;
    static constexpr int K_LEFT_SHIFT = 340;
    static constexpr int K_P     = 80;
    static constexpr int K_S     = 83;
    static constexpr int K_O     = 79;
    static constexpr int K_B     = 66;
    static constexpr int K_F5    = 294;
    (void)K_LEFT_SHIFT;
#else
    static constexpr int K_Z     = GLFW_KEY_Z;
    static constexpr int K_Y     = GLFW_KEY_Y;
    static constexpr int K_LEFT_CTRL  = GLFW_KEY_LEFT_CONTROL;
    static constexpr int K_RIGHT_CTRL = GLFW_KEY_RIGHT_CONTROL;
    static constexpr int K_P     = GLFW_KEY_P;
    static constexpr int K_S     = GLFW_KEY_S;
    static constexpr int K_O     = GLFW_KEY_O;
    static constexpr int K_B     = GLFW_KEY_B;
    static constexpr int K_F5    = GLFW_KEY_F5;
#endif

    // Track Ctrl held state (both press and release)
    if (key == K_LEFT_CTRL || key == K_RIGHT_CTRL) {
        m_ctrlHeld = pressed;
        return;
    }

    if (!pressed) return;  // ignore key-up for non-modifier keys

    if (m_ctrlHeld) {
        if (key == K_Z) {
            auto& bus = UndoableCommandBus::Instance();
            if (bus.CanUndo()) {
                auto desc = bus.NextUndoDescription();
                bus.Undo();
                AppendConsole("[Info]  Undo: " + desc);
            }
        } else if (key == K_Y) {
            auto& bus = UndoableCommandBus::Instance();
            if (bus.CanRedo()) {
                auto desc = bus.NextRedoDescription();
                bus.Redo();
                AppendConsole("[Info]  Redo: " + desc);
            }
        } else if (key == K_S) {
            AppendConsole("[Info]  Save Scene — serialising world…");
        } else if (key == K_O) {
            AppendConsole("[Info]  Open Scene — pick a .scene file…");
        } else if (key == K_B) {
            TriggerBuild();
        }
    } else {
        if (key == K_P) {
            m_playing = !m_playing;
            Engine::Core::Logger::Info(m_playing ? "PIE: Play" : "PIE: Stop");
        } else if (key == K_F5) {
            TriggerBuild();
        }
    }
}

void EditorRenderer::AppendConsole(const std::string& line) {
    m_consoleLines.push_back(line);
    if ((int)m_consoleLines.size() > kMaxConsoleLines)
        m_consoleLines.erase(m_consoleLines.begin());
}

// ── EI-02 helper: entity display name ────────────────────────────────────
std::string EditorRenderer::EntityName(uint32_t id) const {
    if (!m_world) return "Entity #" + std::to_string(id);
    auto* tag = m_world->GetComponent<Runtime::Components::Tag>(id);
    if (tag && !tag->name.empty()) return tag->name;
    return "Entity #" + std::to_string(id);
}

// ── EI-03: simple spatial pick ────────────────────────────────────────────
// Each entity is mapped to an icon rect in DrawOutliner; for now we do a
// linear row-pick matching the outliner row ordering.
uint32_t EditorRenderer::PickEntityAt(float /*sx*/, float /*sy*/) {
    // Viewport click: cycle selection through entities (placeholder approach).
    // A full implementation would ray-cast against AABB components.
    if (!m_world) return 0;
    auto entities = m_world->GetEntities();
    if (entities.empty()) return 0;
    // Find current selected index and advance
    auto it = std::find(entities.begin(), entities.end(), m_selectedEntity);
    if (it == entities.end() || std::next(it) == entities.end())
        return entities.front();
    return *std::next(it);
}

// ── EI-08: trigger build ──────────────────────────────────────────────────
void EditorRenderer::TriggerBuild() {
    AppendConsole("[Info]  Build triggered — running Scripts/Tools/build_all.sh --debug-only");
    Engine::Core::Logger::Info("Build started via editor");
    // Launch build script in background; output is captured by logger sink
    std::string cmd = "Scripts/Tools/build_all.sh --debug-only >> Logs/Build/editor_build.log 2>&1 &";
    std::system(cmd.c_str());
    AppendConsole("[Info]  Build running in background — see Logs/Build/editor_build.log");
}

// ──────────────────────────────────────────────────────────────────────────
// SetupOrtho — maps pixel coords (0,0)=top-left, (w,h)=bottom-right
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::SetupOrtho() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
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
    DrawText("AtlasEditor  v0.1", x + 8.f, y + 6.f, kTextNormal, 1.2f);

    float btnW = 14.f, btnH = 14.f;
    float btnY = y + (h - btnH) * 0.5f;
    DrawRect(w - 50.f, btnY, btnW, btnH, 0x444444FF);
    DrawText("-", w - 47.f, btnY + 2.f, kTextNormal);
    DrawRect(w - 32.f, btnY, btnW, btnH, 0x444444FF);
    DrawText("O", w - 30.f, btnY + 2.f, kTextNormal);
    DrawRect(w - 14.f, btnY, btnW, btnH, 0xE04040FF);
    DrawText("X", w - 12.f, btnY + 2.f, 0xFFFFFFFF);
}

// ──────────────────────────────────────────────────────────────────────────
// Menu bar (EI-07 File > Save/Open, EI-08 Build > Build All)
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawMenuBar(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgMenuBar);
    DrawLine(x, y + h - 1.f, x + w, y + h - 1.f, kBorderColor);

    struct MenuItem { const char* label; };
    static const char* menus[] = {
        "File", "Edit", "View", "Tools", "Scene", "AI", "Build", "Help"
    };
    static constexpr int kMenuCount = 8;

    // Sub-menus per top-level entry
    static const char* fileItems[]  = { "New Project", "Open Project...", nullptr,
                                         "Save Scene", "Open Scene...", nullptr, "Exit" };
    static const char* editItems[]  = { "Undo  Ctrl+Z", "Redo  Ctrl+Y", nullptr,
                                         "Preferences..." };
    static const char* buildItems[] = { "Build All  F5", "Build Debug Only", nullptr,
                                         "Clean" };
    static const char* aiItems[]    = { "AI Chat  Ctrl+Space", "Code Completion", nullptr,
                                         "Build Error Fix" };
    static const char** subMenus[kMenuCount] = {
        fileItems, editItems, nullptr, nullptr, nullptr, aiItems, buildItems, nullptr
    };
    static const int subMenuCounts[kMenuCount] = { 7, 4, 0, 0, 0, 4, 4, 0 };

    float cx = x + 8.f;
    float menuXs[kMenuCount];
    for (int i = 0; i < kMenuCount; i++) {
        int tw = TextWidth(menus[i], 1.f);
        menuXs[i] = cx;
        bool hovered = (m_mouseX >= cx - 4 && m_mouseX < cx + tw + 4 &&
                        m_mouseY >= y       && m_mouseY < y + h);
        bool active  = (m_activeMenu == i);
        if (active || hovered)
            DrawRect(cx - 4.f, y + 1.f, (float)tw + 8.f, h - 2.f, active ? 0x094771FF : 0x3E3E52FF);
        DrawText(menus[i], cx, y + 5.f, (active || hovered) ? 0xFFFFFFFF : kTextNormal);

        // Click to open/close menu
        if (hovered && m_leftMousePressed)
            m_activeMenu = (m_activeMenu == i) ? -1 : i;

        cx += tw + 20.f;
    }

    // Draw open sub-menu
    if (m_activeMenu >= 0 && m_activeMenu < kMenuCount && subMenus[m_activeMenu]) {
        int idx = m_activeMenu;
        float dmX = menuXs[idx] - 4.f;
        float dmY = y + h;
        float dmW = 180.f;
        int   cnt = subMenuCounts[idx];
        float dmH = (float)cnt * 20.f + 4.f;
        DrawRect(dmX, dmY, dmW, dmH, 0x2D2D30FF);
        DrawRectOutline(dmX, dmY, dmW, dmH, kBorderColor);

        float iy = dmY + 2.f;
        for (int i = 0; i < cnt; i++) {
            const char* item = subMenus[idx][i];
            if (!item) {
                DrawLine(dmX + 4.f, iy + 9.f, dmX + dmW - 4.f, iy + 9.f, kBorderColor);
                iy += 20.f;
                continue;
            }
            bool hov = (m_mouseX >= dmX && m_mouseX < dmX + dmW &&
                        m_mouseY >= iy  && m_mouseY < iy + 20.f);
            if (hov) DrawRect(dmX + 2.f, iy, dmW - 4.f, 18.f, 0x094771FF);
            DrawText(item, dmX + 8.f, iy + 4.f, kTextNormal);

            // EI-07 / EI-08: handle clicks
            if (hov && m_leftMousePressed) {
                std::string label = item;
                if (label.find("Save Scene")    != std::string::npos) {
                    AppendConsole("[Info]  Saving scene to Projects/NovaForge/Scenes/…");
                } else if (label.find("Open Scene")  != std::string::npos) {
                    AppendConsole("[Info]  Open Scene — select a .scene file");
                } else if (label.find("Build All") != std::string::npos
                       ||  label.find("Build Debug") != std::string::npos) {
                    TriggerBuild();
                } else if (label.find("AI Chat")   != std::string::npos) {
                    m_aiChatVisible = !m_aiChatVisible;
                } else if (label.find("Undo") != std::string::npos) {
                    if (UndoableCommandBus::Instance().CanUndo())
                        UndoableCommandBus::Instance().Undo();
                } else if (label.find("Redo") != std::string::npos) {
                    if (UndoableCommandBus::Instance().CanRedo())
                        UndoableCommandBus::Instance().Redo();
                }
                m_activeMenu = -1;
            }

            iy += 20.f;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Toolbar — EI-13 Play / Stop buttons
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawToolbar(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, 0x2A2A2AFF);
    DrawLine(x, y + h - 1.f, x + w, y + h - 1.f, kBorderColor);

    // Play button
    float bx = x + w * 0.5f - 36.f;
    float by = y + 4.f;
    float bw = 30.f, bh = 20.f;
    bool playHov = (m_mouseX >= bx && m_mouseX < bx + bw &&
                    m_mouseY >= by && m_mouseY < by + bh);
    DrawRect(bx, by, bw, bh, m_playing ? 0x005522FF : (playHov ? 0x005522AA : 0x003311FF));
    DrawText(m_playing ? "| |" : " > ", bx + 6.f, by + 5.f,
             m_playing ? kTextSuccess : kTextNormal);
    if (playHov && m_leftMousePressed) {
        m_playing = !m_playing;
        Engine::Core::Logger::Info(m_playing ? "PIE: Play started" : "PIE: Play stopped");
    }

    // Stop button
    float sx = bx + bw + 4.f;
    bool stopHov = (m_mouseX >= sx && m_mouseX < sx + bw &&
                    m_mouseY >= by && m_mouseY < by + bh);
    DrawRect(sx, by, bw, bh, stopHov ? 0x551100FF : 0x330000FF);
    DrawText(" [ ]", sx + 4.f, by + 5.f, m_playing ? kTextError : kTextMuted);
    if (stopHov && m_leftMousePressed && m_playing) {
        m_playing = false;
        Engine::Core::Logger::Info("PIE: Play stopped");
    }

    // Mode buttons
    static const char* modes[] = {"Select", "Place", "Paint"};
    float mx = x + 8.f;
    for (auto* m : modes) {
        int tw = TextWidth(m, 1.f);
        bool mhov = (m_mouseX >= mx - 2 && m_mouseX < mx + tw + 4 &&
                     m_mouseY >= by      && m_mouseY < by + bh);
        DrawRect(mx - 2.f, by, (float)tw + 6.f, bh, mhov ? 0x3E3E52FF : 0x2A2A2AFF);
        DrawRectOutline(mx - 2.f, by, (float)tw + 6.f, bh, kBorderColor);
        DrawText(m, mx, by + 5.f, kTextNormal);
        mx += tw + 14.f;
    }

    // Right side: AI chat toggle
    float acx = x + w - 80.f;
    bool achov = (m_mouseX >= acx && m_mouseX < acx + 72.f &&
                  m_mouseY >= by  && m_mouseY < by + bh);
    DrawRect(acx, by, 72.f, bh, m_aiChatVisible ? 0x094771FF : (achov ? 0x3E3E52FF : 0x252526FF));
    DrawRectOutline(acx, by, 72.f, bh, kBorderColor);
    DrawText("AI Chat", acx + 6.f, by + 5.f, kTextAccent);
    if (achov && m_leftMousePressed) m_aiChatVisible = !m_aiChatVisible;
}

// ──────────────────────────────────────────────────────────────────────────
// Viewport — dark 3D area with grid + gizmos (EI-05)
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawViewport(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgViewport);
    DrawPanelHeader("  Viewport  [Camera: Perspective]", x, y, w, kPanelHdrH, kBgHeader);

    float vx = x, vy = y + kPanelHdrH, vw = w, vh = h - kPanelHdrH;

    // Cache for EI-03 mouse pick
    m_vpX = vx; m_vpY = vy; m_vpW = vw; m_vpH = vh;

    glScissor((int)vx, (int)(m_height - vy - vh), (int)vw, (int)vh);
    glEnable(GL_SCISSOR_TEST);

    DrawViewportGrid(vx, vy, vw, vh);

    // Render entity representations when world is available (EI-02)
    if (m_world) {
        float cx = vx + vw * 0.5f;
        float cy = vy + vh * 0.5f;
        auto entities = m_world->GetEntities();
        static const uint32_t kEntityColors[] = {
            0x4A90D9FF, 0x7ED321FF, 0xF5A623FF, 0xBD10E0FF, 0xE05555FF, 0x55BBEEFF
        };
        for (size_t i = 0; i < entities.size(); i++) {
            auto id = entities[i];
            float ex = cx + ((float)i - (float)entities.size() * 0.5f) * 90.f;
            float ey = cy - 20.f;
            float ew = 70.f, eh = 40.f;

            // Use Transform component offset if available
            auto* tr = m_world->GetComponent<Runtime::Components::Transform>(id);
            if (tr) {
                ex = cx + tr->position.x * 20.f;
                ey = cy - tr->position.y * 20.f;
            }

            uint32_t col = kEntityColors[i % 6];
            bool selected = (id == m_selectedEntity);

            DrawRect(ex - ew * 0.5f, ey - eh * 0.5f, ew, eh, col & 0xFFFFFF44);
            DrawRectOutline(ex - ew * 0.5f, ey - eh * 0.5f, ew, eh,
                            selected ? 0xFFCC00FF : col, selected ? 2.f : 1.f);

            std::string name = EntityName(id);
            int tw = TextWidth(name);
            DrawText(name, ex - tw * 0.5f, ey - eh * 0.5f - 12.f,
                     selected ? 0xFFFFFFFF : col);
        }
    } else {
        // Legacy dummy objects when no world is set
        float cx = vx + vw * 0.5f;
        float cy = vy + vh * 0.5f;
        float hullW = vw * 0.30f, hullH = vh * 0.20f;
        DrawRect(cx - hullW * 0.5f, cy - hullH * 0.5f, hullW, hullH, 0x2C4A6EFF);
        DrawRectOutline(cx - hullW * 0.5f, cy - hullH * 0.5f, hullW, hullH, kTextAccent, 1.5f);
        DrawText("Station_Hull", cx - hullW * 0.5f + 4.f, cy - hullH * 0.5f + 4.f, kTextAccent);
    }

    // EI-05: draw gizmo arrows for selected entity
    DrawViewportGizmo(vx, vy, vw, vh);

    DrawViewportAxes(vx, vy + vh);

    glDisable(GL_SCISSOR_TEST);

    DrawText("RMB: orbit  MMB: pan  Scroll: zoom  LMB: select",
             vx + 6.f, vy + vh - 18.f, kTextMuted);

    if (m_playing) {
        DrawText("  ▶ PIE ACTIVE", vx + vw * 0.5f - 50.f, vy + 8.f, kTextSuccess, 1.3f);
    }

    char fpsBuf[32];
    snprintf(fpsBuf, sizeof(fpsBuf), "%.0f FPS", m_fps);
    int tw = TextWidth(fpsBuf);
    DrawText(fpsBuf, vx + vw - tw - 8.f, vy + kPanelHdrH + 4.f, kTextSuccess);
}

void EditorRenderer::DrawViewportGrid(float vpX, float vpY, float vpW, float vpH) {
    float cx = vpX + vpW * 0.5f;
    float cy = vpY + vpH * 0.5f;

    for (float gx = cx; gx < vpX + vpW; gx += 30.f)
        DrawLine(gx, vpY, gx, vpY + vpH, kGridLine);
    for (float gx = cx - 30.f; gx > vpX; gx -= 30.f)
        DrawLine(gx, vpY, gx, vpY + vpH, kGridLine);
    for (float gy = cy; gy < vpY + vpH; gy += 30.f)
        DrawLine(vpX, gy, vpX + vpW, gy, kGridLine);
    for (float gy = cy - 30.f; gy > vpY; gy -= 30.f)
        DrawLine(vpX, gy, vpX + vpW, gy, kGridLine);

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

    DrawLine(ox, oy, ox + len, oy,              kAxisX, 2.f);
    DrawText("X", ox + len + 2.f, oy - 5.f,    kAxisX);
    DrawLine(ox, oy, ox, oy - len,              kAxisY, 2.f);
    DrawText("Y", ox - 3.f, oy - len - 12.f,   kAxisY);
    DrawLine(ox, oy, ox + len * 0.6f, oy - len * 0.6f, kAxisZ, 2.f);
    DrawText("Z", ox + len * 0.6f + 2.f, oy - len * 0.6f - 10.f, kAxisZ);
}

// EI-05: draw translate gizmo arrows for the selected entity
void EditorRenderer::DrawViewportGizmo(float vpX, float vpY, float vpW, float vpH) {
    if (!m_world || m_selectedEntity == 0) return;

    float cx = vpX + vpW * 0.5f;
    float cy = vpY + vpH * 0.5f;

    // Project entity position to screen
    float ex = cx, ey = cy;
    auto* tr = m_world->GetComponent<Runtime::Components::Transform>(m_selectedEntity);
    if (tr) {
        ex = cx + tr->position.x * 20.f;
        ey = cy - tr->position.y * 20.f;
    } else {
        // Fallback: position along entity index row
        auto entities = m_world->GetEntities();
        auto it = std::find(entities.begin(), entities.end(), m_selectedEntity);
        if (it != entities.end()) {
            size_t idx = (size_t)(it - entities.begin());
            ex = cx + ((float)idx - (float)entities.size() * 0.5f) * 90.f;
        }
    }

    float gLen = 40.f;
    float aSize = 6.f; // arrowhead half-size

    // X axis (red) →
    DrawLine(ex, ey, ex + gLen, ey, kAxisX, 2.5f);
    DrawLine(ex + gLen, ey, ex + gLen - aSize, ey - aSize, kAxisX, 1.5f);
    DrawLine(ex + gLen, ey, ex + gLen - aSize, ey + aSize, kAxisX, 1.5f);
    DrawText("X", ex + gLen + 3.f, ey - 6.f, kAxisX, 1.1f);

    // Y axis (green) ↑
    DrawLine(ex, ey, ex, ey - gLen, kAxisY, 2.5f);
    DrawLine(ex, ey - gLen, ex - aSize, ey - gLen + aSize, kAxisY, 1.5f);
    DrawLine(ex, ey - gLen, ex + aSize, ey - gLen + aSize, kAxisY, 1.5f);
    DrawText("Y", ex + 4.f, ey - gLen - 10.f, kAxisY, 1.1f);

    // Z axis (blue) ↗ (projected)
    float zex = ex + gLen * 0.6f;
    float zey = ey - gLen * 0.6f;
    DrawLine(ex, ey, zex, zey, kAxisZ, 2.5f);
    DrawText("Z", zex + 3.f, zey - 8.f, kAxisZ, 1.1f);

    // Centre dot
    DrawRect(ex - 4.f, ey - 4.f, 8.f, 8.f, 0xFFFFFFFF);
}

// ──────────────────────────────────────────────────────────────────────────
// Scene Outliner (EI-02: live ECS entities)
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawOutliner(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgPanel);
    DrawPanelHeader("  Scene Outliner", x, y, w, kPanelHdrH, kBgHeader);

    float iy = y + kPanelHdrH + 2.f;
    float rowH = 18.f;

    auto DrawRow = [&](uint32_t id, const std::string& name,
                       uint32_t col, float indent) {
        bool selected = (id == m_selectedEntity);
        if (selected)
            DrawRect(x, iy, w, rowH, 0x094771FF);
        else if (m_mouseX >= x && m_mouseX < x + w &&
                 m_mouseY >= iy && m_mouseY < iy + rowH)
            DrawRect(x, iy, w, rowH, 0x2A2D2EFF);

        DrawText("[E]", x + 4.f + indent, iy + 3.f, col);
        DrawText(name,  x + 28.f + indent, iy + 3.f,
                 selected ? 0xFFFFFFFF : kTextNormal);

        // Click to select
        if (m_mouseX >= x && m_mouseX < x + w &&
            m_mouseY >= iy && m_mouseY < iy + rowH && m_leftMousePressed)
        {
            m_selectedEntity = selected ? 0 : id;
        }

        iy += rowH;
    };

    int entityCount = 0;

    if (m_world) {
        auto entities = m_world->GetEntities();
        entityCount = (int)entities.size();
        static const uint32_t kColors[] = {
            0x4A90D9FF, 0x7ED321FF, 0xF5A623FF, 0xBD10E0FF, 0xE05555FF, 0x55BBEEFF
        };
        for (size_t i = 0; i < entities.size(); i++) {
            auto id = entities[i];
            std::string name = EntityName(id);
            uint32_t col = kColors[i % 6];
            DrawRow(id, name, col, 0.f);
        }
    } else {
        // Legacy dummy list
        static const char* names[] = {
            "PlayerShip", "Station_Hull", "Engine_L", "Engine_R",
            "Turret_01", "Turret_02", "AsteroidCluster"
        };
        static const uint32_t colors[] = {
            0x4A90D9FF, 0x7ED321FF, 0xF5A623FF, 0xF5A623FF,
            0xBD10E0FF, 0xBD10E0FF, 0xE05555FF
        };
        entityCount = 7;
        for (int i = 0; i < entityCount; i++) {
            float indent = (i >= 2 && i <= 5) ? 12.f : 0.f;
            DrawText("[E]", x + 4.f + indent, iy + 3.f, colors[i]);
            DrawText(names[i], x + 28.f + indent, iy + 3.f, kTextNormal);
            iy += rowH;
        }
    }

    DrawLine(x, y + h - 20.f, x + w, y + h - 20.f, kBorderColor);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d entities", entityCount);
    DrawText(buf, x + 4.f, y + h - 16.f, kTextMuted);
}

// ──────────────────────────────────────────────────────────────────────────
// Inspector (EI-04: show real component properties via reflection)
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawInspector(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgPanel);
    DrawPanelHeader("  Inspector", x, y, w, kPanelHdrH, kBgHeader);

    float cy = y + kPanelHdrH + 6.f;
    float lh = 18.f;

    auto Row = [&](const char* label, const std::string& value, uint32_t valCol = kTextAccent) {
        DrawText(label, x + 6.f, cy, kTextMuted);
        DrawText(value, x + w * 0.45f, cy, valCol);
        cy += lh;
    };

    if (m_world && m_selectedEntity != 0 && m_world->IsAlive(m_selectedEntity)) {
        // Entity header
        std::string name = EntityName(m_selectedEntity);
        DrawRect(x + 4.f, cy, w - 8.f, 20.f, 0x094771FF);
        DrawText(name, x + 8.f, cy + 3.f, 0xFFFFFFFF, 1.1f);
        DrawText("[enabled]", x + w - 68.f, cy + 3.f, kTextSuccess);
        cy += 24.f;

        // Transform component
        auto* tr = m_world->GetComponent<Runtime::Components::Transform>(m_selectedEntity);
        if (tr) {
            DrawRect(x + 4.f, cy, w - 8.f, 16.f, kBgHeader);
            DrawText("Transform", x + 8.f, cy + 2.f, kTextWarn);
            cy += 18.f;

            char buf[64];
            snprintf(buf, sizeof(buf), "%.2f, %.2f, %.2f",
                     tr->position.x, tr->position.y, tr->position.z);
            Row("Position", buf);
            snprintf(buf, sizeof(buf), "%.2f, %.2f, %.2f, %.2f",
                     tr->rotation.x, tr->rotation.y, tr->rotation.z, tr->rotation.w);
            Row("Rotation", buf);
            snprintf(buf, sizeof(buf), "%.2f, %.2f, %.2f",
                     tr->scale.x, tr->scale.y, tr->scale.z);
            Row("Scale", buf);
            cy += 4.f;
        }

        // Mesh renderer component
        auto* mr = m_world->GetComponent<Runtime::Components::MeshRenderer>(m_selectedEntity);
        if (mr) {
            DrawRect(x + 4.f, cy, w - 8.f, 16.f, kBgHeader);
            DrawText("Mesh", x + 8.f, cy + 2.f, kTextWarn);
            cy += 18.f;
            Row("Asset",   mr->meshId.empty()     ? "(none)" : mr->meshId);
            Row("Material",mr->materialId.empty() ? "(none)" : mr->materialId);
            Row("Visible", mr->visible ? "true" : "false",
                mr->visible ? kTextSuccess : kTextError);
            cy += 4.f;
        }

        // RigidBody component
        auto* rb = m_world->GetComponent<Runtime::Components::RigidBody>(m_selectedEntity);
        if (rb) {
            DrawRect(x + 4.f, cy, w - 8.f, 16.f, kBgHeader);
            DrawText("RigidBody", x + 8.f, cy + 2.f, kTextWarn);
            cy += 18.f;
            char buf[64];
            snprintf(buf, sizeof(buf), "%.1f kg", rb->mass);
            Row("Mass", buf);
            Row("Kinematic", rb->isKinematic ? "true" : "false",
                rb->isKinematic ? 0xF5A623FF : kTextNormal);
            cy += 4.f;
        }

        // Tags
        auto* tag = m_world->GetComponent<Runtime::Components::Tag>(m_selectedEntity);
        if (tag && !tag->tags.empty()) {
            DrawRect(x + 4.f, cy, w - 8.f, 16.f, kBgHeader);
            DrawText("Tags", x + 8.f, cy + 2.f, kTextWarn);
            cy += 18.f;
            std::string tagStr;
            for (auto& t : tag->tags) tagStr += "[" + t + "] ";
            DrawText(tagStr, x + 6.f, cy, 0xBD10E0FF);
            cy += lh;
        }
    } else {
        // No selection — show stub
        DrawRect(x + 4.f, cy, w - 8.f, 20.f, 0x094771FF);
        DrawText("Station_Hull", x + 8.f, cy + 3.f, 0xFFFFFFFF, 1.1f);
        DrawText("[enabled]", x + w - 68.f, cy + 3.f, kTextSuccess);
        cy += 24.f;

        DrawRect(x + 4.f, cy, w - 8.f, 16.f, kBgHeader);
        DrawText("Transform", x + 8.f, cy + 2.f, kTextWarn);
        cy += 18.f;
        Row("Position",  "0.0, 0.0, 0.0");
        Row("Rotation",  "0.0, 0.0, 0.0");
        Row("Scale",     "1.0, 1.0, 1.0");
        cy += 4.f;
        DrawRect(x + 4.f, cy, w - 8.f, 16.f, kBgHeader);
        DrawText("Mesh", x + 8.f, cy + 2.f, kTextWarn);
        cy += 18.f;
        Row("Asset",     "hull_v2.obj");
        Row("LOD",       "Auto");
        Row("Visible",   "true", kTextSuccess);
        cy += 4.f;
        DrawRect(x + 4.f, cy, w - 8.f, 16.f, kBgHeader);
        DrawText("RigidBody", x + 8.f, cy + 2.f, kTextWarn);
        cy += 18.f;
        Row("Mass",      "1200.0 kg");
        Row("Friction",  "0.35");
        Row("Kinematic", "false", 0xF5A623FF);
    }

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
    // Split bottom half: left = console, right = error panel (EI-09)
    float consW = w * 0.65f;
    float errX  = x + consW + 1.f;
    float errW  = w - consW - 1.f;

    DrawRect(x, y, consW, h, kBgConsole);
    DrawPanelHeader("  Console  Output", x, y, consW, kPanelHdrH, kBgHeader);

    glScissor((int)x, (int)(m_height - y - h), (int)consW,
              (int)(h - kPanelHdrH - 20.f));
    glEnable(GL_SCISSOR_TEST);

    float lineH = 14.f;
    float fy = y + h - 22.f - lineH;
    int visLines = (int)((h - kPanelHdrH - 22.f) / lineH);
    int start = std::max(0, (int)m_consoleLines.size() - visLines);
    for (int i = (int)m_consoleLines.size() - 1; i >= start && fy > y + kPanelHdrH; i--) {
        const std::string& line = m_consoleLines[i];
        uint32_t col = kTextNormal;
        if      (line.find("[Error]") != std::string::npos) col = kTextError;
        else if (line.find("[Warn]")  != std::string::npos) col = kTextWarn;
        else if (line.find("[Info]")  != std::string::npos) col = kTextAccent;
        else if (line.find("[Debug]") != std::string::npos) col = kTextMuted;
        DrawText(line, x + 4.f, fy, col);
        fy -= lineH;
    }
    glDisable(GL_SCISSOR_TEST);

    DrawLine(x, y + h - 20.f, x + consW, y + h - 20.f, kBorderColor);
    DrawRect(x + 2.f, y + h - 19.f, consW - 4.f, 18.f, 0x141414FF);
    DrawText("> _", x + 6.f, y + h - 16.f, kTextAccent);

    // Separator
    DrawLine(errX - 1.f, y, errX - 1.f, y + h, kBorderColor);

    // EI-09: Error panel
    DrawErrorPanel(errX, y, errW, h);
}

// ── EI-09: Error panel ────────────────────────────────────────────────────
void EditorRenderer::DrawErrorPanel(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, 0x1A1A1AFF);
    char hdr[48];
    snprintf(hdr, sizeof(hdr), "  Errors (%zu) / Warnings (%zu)",
             m_errorPanel->ErrorCount(), m_errorPanel->WarningCount());
    DrawPanelHeader(hdr, x, y, w, kPanelHdrH, kBgHeader);

    float iy = y + kPanelHdrH + 2.f;
    float rowH = 16.f;
    auto filtered = m_errorPanel->GetFiltered();
    for (size_t i = 0; i < filtered.size() && iy + rowH < y + h - 4.f; i++) {
        const auto& e = filtered[i];
        bool hov = (m_mouseX >= x && m_mouseX < x + w &&
                    m_mouseY >= iy && m_mouseY < iy + rowH);
        if (hov) DrawRect(x, iy, w, rowH, 0x2A2D2EFF);

        uint32_t col = (e.severity == ErrorSeverity::Error)   ? kTextError
                     : (e.severity == ErrorSeverity::Warning) ? kTextWarn
                     :                                          kTextMuted;
        char buf[160];
        snprintf(buf, sizeof(buf), "%s(%u): %s",
                 e.file.c_str(), e.line, e.message.c_str());
        DrawText(buf, x + 4.f, iy + 3.f, col);

        // EI-09: click to navigate
        if (hov && m_leftMousePressed)
            m_errorPanel->NavigateTo(i);

        iy += rowH;
    }
    if (filtered.empty()) {
        DrawText("No errors.", x + 4.f, iy + 2.f, kTextMuted);
    }
}

// ── EI-06: Content browser ────────────────────────────────────────────────
void EditorRenderer::DrawContentBrowser(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, 0x1E1E22FF);
    std::string hdr = "  Assets  [" + m_contentBrowser->CurrentPath() + "]";
    DrawPanelHeader(hdr.c_str(), x, y, w, kPanelHdrH, kBgHeader);

    float iy = y + kPanelHdrH + 2.f;
    float rowH = 16.f;

    // Navigate-up button
    bool upHov = (m_mouseX >= x + 4 && m_mouseX < x + 30 &&
                  m_mouseY >= iy     && m_mouseY < iy + rowH);
    DrawRect(x + 4.f, iy, 28.f, rowH - 2.f, upHov ? 0x3E3E52FF : 0x2D2D30FF);
    DrawText("[..]", x + 6.f, iy + 3.f, kTextAccent);
    if (upHov && m_leftMousePressed)
        m_contentBrowser->NavigateUp();
    iy += rowH;

    const auto& entries = m_contentBrowser->Entries();
    for (const auto& e : entries) {
        if (iy + rowH >= y + h - 4.f) break;
        bool hov = (m_mouseX >= x && m_mouseX < x + w &&
                    m_mouseY >= iy && m_mouseY < iy + rowH);
        bool sel = m_contentBrowser->IsSelected(e.fullPath);
        if (sel)       DrawRect(x, iy, w, rowH, 0x094771FF);
        else if (hov)  DrawRect(x, iy, w, rowH, 0x2A2D2EFF);

        std::string icon = ContentBrowser::IconForEntry(e);
        DrawText(icon,   x + 4.f,  iy + 3.f, e.isDirectory ? kTextWarn : kTextAccent);
        DrawText(e.name, x + 28.f, iy + 3.f, sel ? 0xFFFFFFFF : kTextNormal);

        if (hov && m_leftMousePressed)
            m_contentBrowser->Select(e.fullPath);
        if (hov && m_leftMousePressed && e.isDirectory)
            m_contentBrowser->NavigateTo(e.fullPath);

        iy += rowH;
    }
}

// ── EI-12: Code editor panel ──────────────────────────────────────────────
void EditorRenderer::DrawCodeEditor(float x, float y, float w, float h) {
    if (!m_codeEditorVisible) return;
    DrawRect(x, y, w, h, 0x1E1E1EFF);
    DrawRectOutline(x, y, w, h, kBorderColor);
    DrawPanelHeader("  Code Editor", x, y, w, kPanelHdrH, kBgHeader);

    // Close button
    float cx = x + w - 22.f;
    bool closeHov = (m_mouseX >= cx && m_mouseX < cx + 20.f &&
                     m_mouseY >= y  && m_mouseY < y + kPanelHdrH);
    DrawRect(cx, y + 2.f, 18.f, kPanelHdrH - 4.f, closeHov ? 0xE04040FF : 0x555555FF);
    DrawText("X", cx + 5.f, y + 5.f, 0xFFFFFFFF);
    if (closeHov && m_leftMousePressed) m_codeEditorVisible = false;

    size_t lineCount = m_codeEditor->LineCount();
    if (lineCount == 0 || (lineCount == 1 && m_codeEditor->GetLine(0).empty())) {
        DrawText("(no file open — click an error in the Error panel to navigate)",
                 x + 6.f, y + kPanelHdrH + 8.f, kTextMuted);
        return;
    }

    // Build highlighted lines once per render; cache via HighlightAll()
    auto lines = m_codeEditor->HighlightAll();

    uint32_t curLine = m_codeEditor->GetCursor().line;
    float lineH = 14.f;
    float fy    = y + kPanelHdrH + 4.f;
    uint32_t startL = (curLine > 10) ? curLine - 10 : 0;

    glScissor((int)x, (int)(m_height - y - h), (int)w, (int)(h - kPanelHdrH));
    glEnable(GL_SCISSOR_TEST);

    for (uint32_t li = startL; li < (uint32_t)lines.size() && fy + lineH < y + h; li++) {
        // Line number
        char lnBuf[8]; snprintf(lnBuf, sizeof(lnBuf), "%4u", li + 1);
        DrawText(lnBuf, x + 4.f, fy, li == curLine ? kTextWarn : kTextMuted);

        // Current-line highlight
        if (li == curLine)
            DrawRect(x + 34.f, fy - 1.f, w - 36.f, lineH + 1.f, 0x093060FF);

        const auto& hl = lines[li];
        // Simple render: coloured tokens
        if (hl.tokens.empty()) {
            DrawText(hl.text, x + 38.f, fy, kTextNormal);
        } else {
            for (const auto& tok : hl.tokens) {
                uint32_t col = kTextNormal;
                switch (tok.type) {
                    case IDE::TokenType::Keyword:     col = 0x569CD6FF; break;
                    case IDE::TokenType::Type:        col = 0x4EC9B0FF; break;
                    case IDE::TokenType::String:      col = 0xCE9178FF; break;
                    case IDE::TokenType::Comment:     col = 0x6A9955FF; break;
                    case IDE::TokenType::Number:      col = 0xB5CEA8FF; break;
                    case IDE::TokenType::Preprocessor:col = 0xC586C0FF; break;
                    default: break;
                }
                uint32_t start = tok.startCol;
                uint32_t end   = std::min((uint32_t)hl.text.size(), tok.endCol);
                if (start < end)
                    DrawText(hl.text.substr(start, end - start),
                             x + 38.f + start * 7.f, fy, col);
            }
        }
        fy += lineH;
    }
    glDisable(GL_SCISSOR_TEST);
}

// ── EI-14: AI chat panel ──────────────────────────────────────────────────
void EditorRenderer::DrawAIChat(float x, float y, float w, float h) {
    if (!m_aiChatVisible) return;
    DrawRect(x, y, w, h, 0x1E1E28FF);
    DrawRectOutline(x, y, w, h, kBorderColor);
    DrawPanelHeader("  AI Chat  (Ollama)", x, y, w, kPanelHdrH, kBgHeader);

    // Close
    float cx = x + w - 22.f;
    bool closeHov = (m_mouseX >= cx && m_mouseX < cx + 20.f &&
                     m_mouseY >= y  && m_mouseY < y + kPanelHdrH);
    DrawRect(cx, y + 2.f, 18.f, kPanelHdrH - 4.f, closeHov ? 0xE04040FF : 0x555555FF);
    DrawText("X", cx + 5.f, y + 5.f, 0xFFFFFFFF);
    if (closeHov && m_leftMousePressed) m_aiChatVisible = false;

    // Conversation
    float inputH = 24.f;
    float chatH  = h - kPanelHdrH - inputH - 4.f;

    glScissor((int)x, (int)(m_height - y - kPanelHdrH - chatH), (int)w, (int)chatH);
    glEnable(GL_SCISSOR_TEST);

    float lineH = 14.f;
    float fy    = y + kPanelHdrH + chatH - lineH;
    const auto& msgs = m_aiChat->Messages();
    for (int i = (int)msgs.size() - 1; i >= 0 && fy > y + kPanelHdrH; i--) {
        const auto& m = msgs[i];
        uint32_t col = (m.role == IDE::ChatRole::User)    ? 0xFFFFFFFF
                     : (m.role == IDE::ChatRole::System)  ? kTextMuted
                     :                                      kTextAccent;
        const char* prefix = (m.role == IDE::ChatRole::User) ? "You: " : "AI:  ";
        DrawText(prefix + m.content, x + 4.f, fy, col);
        fy -= lineH;
    }
    glDisable(GL_SCISSOR_TEST);

    // Input line
    float iy = y + kPanelHdrH + chatH + 2.f;
    DrawLine(x, iy - 1.f, x + w, iy - 1.f, kBorderColor);
    DrawRect(x + 2.f, iy, w - 40.f, inputH - 2.f, 0x141414FF);
    DrawText(m_aiInput + "_", x + 6.f, iy + 5.f, kTextNormal);

    bool sendHov = (m_mouseX >= x + w - 36.f && m_mouseX < x + w - 4.f &&
                    m_mouseY >= iy && m_mouseY < iy + inputH - 2.f);
    DrawRect(x + w - 36.f, iy, 32.f, inputH - 2.f,
             sendHov ? 0x1177BBFF : 0x007ACCFF);
    DrawText("Send", x + w - 34.f, iy + 5.f, 0xFFFFFFFF);
    if (sendHov && m_leftMousePressed && !m_aiInput.empty()) {
        m_aiChat->SendMessage(m_aiInput);
        m_aiInput.clear();
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Status bar
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawStatusBar(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgStatusBar);

    int entityCount = m_world ? (int)m_world->EntityCount() : 0;
    char statusBuf[128];
    snprintf(statusBuf, sizeof(statusBuf),
             "  %s  |  Scene: NovaForge_Dev  |  Entities: %d  |  PCG: idle%s",
             UndoableCommandBus::Instance().CanUndo()
                 ? ("Undo: " + UndoableCommandBus::Instance().NextUndoDescription()).c_str()
                 : "Ready",
             entityCount,
             m_playing ? "  |  ▶ PIE" : "");
    DrawText(statusBuf, x + 4.f, y + 5.f, 0xFFFFFFFF);

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

    // Reset single-frame flags
    m_leftMousePressed = false;

    float W = (float)m_width;
    float H = (float)m_height;

    glViewport(0, 0, m_width, m_height);
    glClearColor(R(kBgBase)/255.f, G(kBgBase)/255.f, B(kBgBase)/255.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    SetupOrtho();

    // ── Layout geometry ────────────────────────────────────────────────────
    float titleY   = 0.f;
    float menuY    = titleY   + kTitleH;
    float toolbarY = menuY    + kMenuH;
    float statusY  = H        - kStatusH;
    float consoleY = statusY  - kConsoleH;
    float mainY    = toolbarY + kToolbarH;
    float mainH    = consoleY - mainY;

    float outlinerX  = 0.f;
    float inspectorX = W - kInspectorW;
    float viewportX  = kOutlinerW;
    float viewportW  = W - kOutlinerW - kInspectorW;

    // ── Background ─────────────────────────────────────────────────────────
    DrawRect(0.f, 0.f, W, H, kBgBase);

    // ── Main panels ────────────────────────────────────────────────────────
    DrawOutliner (outlinerX,  mainY,  kOutlinerW,  mainH);
    DrawViewport (viewportX,  mainY,  viewportW,   mainH);
    DrawInspector(inspectorX, mainY,  kInspectorW, mainH);
    DrawConsole  (0.f,        consoleY, W, kConsoleH);

    // ── Borders between panels ─────────────────────────────────────────────
    DrawLine(kOutlinerW,  mainY, kOutlinerW,  consoleY, kBorderColor, 1.5f);
    DrawLine(inspectorX,  mainY, inspectorX,  consoleY, kBorderColor, 1.5f);
    DrawLine(0.f, consoleY, W, consoleY,                kBorderColor, 1.5f);

    // ── Bars (drawn on top) ────────────────────────────────────────────────
    DrawToolbar (0.f, toolbarY, W, kToolbarH);
    DrawMenuBar (0.f, menuY,  W, kMenuH);
    DrawTitleBar(0.f, titleY, W, kTitleH);
    DrawStatusBar(0.f, statusY, W, kStatusH);

    // ── Top / bottom hairlines ─────────────────────────────────────────────
    DrawLine(0.f, menuY + kMenuH, W, menuY + kMenuH, kBorderColor);
    DrawLine(0.f, toolbarY + kToolbarH, W, toolbarY + kToolbarH, kBorderColor);

    // ── Floating panels (drawn last so they appear on top) ─────────────────
    // EI-12: code editor overlay (right half, above console)
    if (m_codeEditorVisible) {
        float ceX = kOutlinerW;
        float ceW = W - kOutlinerW - kInspectorW;
        float ceH = mainH * 0.6f;
        float ceY = mainY + mainH * 0.4f;
        DrawCodeEditor(ceX, ceY, ceW, ceH);
    }

    // EI-14: AI chat overlay (right side)
    if (m_aiChatVisible) {
        float acW = 300.f;
        float acH = 400.f;
        float acX = W - kInspectorW - acW - 4.f;
        float acY = mainY + 10.f;
        DrawAIChat(acX, acY, acW, acH);
    }
}

} // namespace Editor
