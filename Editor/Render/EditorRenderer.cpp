#include "Editor/Render/EditorRenderer.h"
#include "Runtime/ECS/ECS.h"
#include "Runtime/Components/Components.h"
#include "Editor/Panels/ContentBrowser/ContentBrowser.h"
#include "Editor/Panels/ErrorPanel/ErrorPanel.h"
#include "Editor/UndoableCommandBus.h"
#include "IDE/CodeEditor/CodeEditor.h"
#include "IDE/AIChat/AIChat.h"
#include "Engine/Core/Logger.h"
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX          // prevent min/max macro pollution
#  include <windows.h>     // must precede GL/gl.h on Windows (defines WINGDIAPI/APIENTRY)
#  undef DrawText           // prevent DrawText → DrawTextA macro expansion
#  undef SendMessage        // prevent SendMessage → SendMessageA macro expansion
#endif
#include <GL/gl.h>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include "AI/OllamaClient/OllamaClient.h"
#include <future>
#include <thread>
#include <chrono>
#include <atomic>

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

// ── Editor defaults ────────────────────────────────────────────────────────
static constexpr float    kRotSnapDeg        = 15.f;  // rotation snap angle (degrees)
static constexpr char     kDefaultScenePath[] = "Projects/NovaForge/Scenes/editor_save.scene";
static constexpr float    kPIEOrbitSpeedFactor = 0.06f; // orbit speed divisor in PIE animation
static constexpr float    kScrollToBottom      = 99999.f; // sentinel to auto-scroll to bottom
static constexpr float    kApproxCharWidth     = 7.f;    // pixels per char for stb_easy_font at scale 1

// ── Layout constants ───────────────────────────────────────────────────────
static constexpr float kTitleH    = 26.f;
static constexpr float kMenuH     = 22.f;
static constexpr float kToolbarH  = 28.f;
static constexpr float kStatusH   = 22.f;
static constexpr float kConsoleH  = 160.f;
static constexpr float kOutlinerW = 220.f;
static constexpr float kInspectorW= 240.f;
static constexpr float kPanelHdrH = 20.f;

// ── Undoable command classes ───────────────────────────────────────────────

struct CreateEntityCmd final : IUndoableCommand {
    Runtime::ECS::World*           world;
    uint32_t&                      selRef;
    Runtime::Components::Tag       tag;
    Runtime::Components::Transform transform;
    uint32_t                       createdId = 0;

    CreateEntityCmd(Runtime::ECS::World* w, uint32_t& sel,
                    Runtime::Components::Tag t, Runtime::Components::Transform tr)
        : world(w), selRef(sel), tag(std::move(t)), transform(tr) {}

    void Execute() override {
        createdId = world->CreateEntity();
        world->AddComponent(createdId, tag);
        world->AddComponent(createdId, transform);
        selRef = createdId;
    }
    void Undo() override {
        world->DestroyEntity(createdId);
        if (selRef == createdId) selRef = 0;
    }
    const char* Description() const override { return "Create Entity"; }
};

struct DeleteEntityCmd final : IUndoableCommand {
    Runtime::ECS::World*           world;
    uint32_t&                      selRef;
    uint32_t                       entityId;
    Runtime::Components::Tag       tag;
    Runtime::Components::Transform transform;
    bool                           hadTag = false, hadTransform = false;

    DeleteEntityCmd(Runtime::ECS::World* w, uint32_t& sel, uint32_t id)
        : world(w), selRef(sel), entityId(id) {
        if (auto* t  = w->GetComponent<Runtime::Components::Tag>(id))       { tag = *t;  hadTag = true; }
        if (auto* tr = w->GetComponent<Runtime::Components::Transform>(id)) { transform = *tr; hadTransform = true; }
    }
    void Execute() override {
        world->DestroyEntity(entityId);
        if (selRef == entityId) selRef = 0;
    }
    void Undo() override {
        uint32_t newId = world->CreateEntity();
        if (hadTag)       world->AddComponent(newId, tag);
        if (hadTransform) world->AddComponent(newId, transform);
        entityId = newId;
        selRef   = newId;
    }
    const char* Description() const override { return "Delete Entity"; }
};

struct MoveEntityCmd final : IUndoableCommand {
    Runtime::ECS::World*           world;
    uint32_t                       entityId;
    Runtime::Components::Transform oldTr, newTr;

    MoveEntityCmd(Runtime::ECS::World* w, uint32_t id,
                  Runtime::Components::Transform o, Runtime::Components::Transform n)
        : world(w), entityId(id), oldTr(o), newTr(n) {}

    void Execute() override {
        if (auto* tr = world->GetComponent<Runtime::Components::Transform>(entityId)) *tr = newTr;
    }
    void Undo() override {
        if (auto* tr = world->GetComponent<Runtime::Components::Transform>(entityId)) *tr = oldTr;
    }
    const char* Description() const override { return "Move Entity"; }
};

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

    // EI-14: wire AI chat to local Ollama (non-blocking async call)
    m_aiChat->SetSendCallback([this](const std::string& userMsg, IDE::AIChat& /*chat*/) {
        Engine::Core::Logger::Info("[AI] User: " + userMsg);
        if (m_aiFuture.valid() &&
            m_aiFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
            m_aiChat->AppendMessage(IDE::ChatRole::Assistant,
                "[AI] Still processing previous request — please wait.");
            return;
        }
        m_aiChat->AppendMessage(IDE::ChatRole::Assistant, "⏳ Thinking…");
        std::string sysPrompt = m_aiChat->SystemPrompt();
        m_aiFuture = std::async(std::launch::async, [userMsg, sysPrompt]() -> std::string {
            AI::OllamaClient client("localhost", 11434);
            if (!client.Ping())
                return "⚠ Ollama not reachable at localhost:11434.\n"
                       "Start it with:  ollama serve\n"
                       "Then pull a model:  ollama pull codellama";
            auto resp = client.Generate("codellama", userMsg, sysPrompt);
            return resp.success ? resp.text : ("⚠ Error: " + resp.error);
        });
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

    // ProjectAI: prime with a welcome context message on startup
    if (m_aiChat) {
        m_aiChat->AppendMessage(IDE::ChatRole::Assistant,
            "Welcome to Atlas AI — your local project assistant.\n\n"
            "I have access to the project folder. You can ask me to:\n"
            "  • Explain any system or file  (\"explain Runtime/ECS/ECS.h\")\n"
            "  • Help plan new features      (\"how should I add multiplayer?\")\n"
            "  • Review code or architecture (\"review the PIE simulation\")\n"
            "  • Generate code snippets\n\n"
            "I run fully offline via Ollama at localhost:11434.\n"
            "Run 'ollama serve' and 'ollama pull codellama' to get started.");
    }

    return true;
}

// EI-02
void EditorRenderer::SetWorld(Runtime::ECS::World* world) {
    m_world = world;
}

void EditorRenderer::StopPIE() {
    if (!m_playing) return;
    m_playing = false;
    m_pieTime = 0.f;
    Engine::Core::Logger::Info("PIE: stopped");
    AppendConsole("[Info]  PIE stopped");
}

void EditorRenderer::Resize(int width, int height) {
    m_width  = width;
    m_height = height;
    glViewport(0, 0, width, height);
}

EditorRenderer::~EditorRenderer() { Shutdown(); }
void EditorRenderer::Shutdown() {}

// ── Input forwarding ───────────────────────────────────────────────────────
void EditorRenderer::OnMouseMove(double x, double y) {
    double dx = x - m_dragLastX;
    double dy = y - m_dragLastY;

    // ── RMB: orbit camera (rotate yaw/pitch around target) ───────────────
    if (m_rmbDown) {
        m_vpYaw   += (float)dx * 0.4f;
        m_vpPitch += (float)dy * 0.3f;
        m_vpPitch  = std::max(-88.f, std::min(88.f, m_vpPitch));
        m_mouseX   = x; m_mouseY   = y;
        m_dragLastX = x; m_dragLastY = y;
        return;
    }

    // ── MMB: pan target (camera-relative) ────────────────────────────────
    if (m_mmbDown) {
        // Move target in the camera's right and up directions
        float yawR = m_vpYaw   * 3.14159265f / 180.f;
        float pitR = m_vpPitch * 3.14159265f / 180.f;
        // right = (-cos(yaw), 0, sin(yaw)) in the orbit plane, scaled by dist
        float speed = m_vpDist * 0.002f;
        m_vpTargX -= (float)dx * speed * std::cos(yawR);
        m_vpTargZ -= (float)dx * speed * std::sin(yawR);
        m_vpTargY += (float)dy * speed * std::cos(pitR);
        m_mouseX   = x; m_mouseY   = y;
        m_dragLastX = x; m_dragLastY = y;
        return;
    }

    // ── Gizmo drag — move entity along the dragged axis ──────────────────
    if (m_gizmoDragging && m_gizmoDragAxis != 0 &&
        m_world && m_selectedEntity != 0 && m_world->IsAlive(m_selectedEntity))
    {
        auto* tr = m_world->GetComponent<Runtime::Components::Transform>(m_selectedEntity);
        if (tr) {
            // pixels → world units: scale by orbit distance so far objects aren't oversensitive
            float sensitivity = m_vpDist * 0.001f;
            if (m_gizmoMode == 0) { // Move
                if (m_gizmoDragAxis == 1) tr->position.x += (float)(dx * sensitivity);
                if (m_gizmoDragAxis == 2) tr->position.y -= (float)(dy * sensitivity);
                if (m_gizmoDragAxis == 3) tr->position.z += (float)(dx * sensitivity);
                if (m_gridSnap) {
                    tr->position.x = SnapToGrid(tr->position.x);
                    tr->position.y = SnapToGrid(tr->position.y);
                    tr->position.z = SnapToGrid(tr->position.z);
                }
            } else if (m_gizmoMode == 1) { // Rotate
                float angle = (float)(dx * 1.5f);
                if (m_gridSnap) angle = std::round(angle / kRotSnapDeg) * kRotSnapDeg;
                Engine::Math::Vec3 axis{0, 0, 0};
                if (m_gizmoDragAxis == 1) axis = {1, 0, 0};
                if (m_gizmoDragAxis == 2) axis = {0, 1, 0};
                if (m_gizmoDragAxis == 3) axis = {0, 0, 1};
                auto delta = Engine::Math::Quaternion::FromAxisAngle(axis, angle);
                tr->rotation = (tr->rotation * delta).Normalized();
            } else { // Scale
                float delta = (float)(dx * sensitivity);
                if (m_gizmoDragAxis == 1) tr->scale.x = std::max(0.01f, tr->scale.x + delta);
                if (m_gizmoDragAxis == 2) tr->scale.y = std::max(0.01f, tr->scale.y - (float)(dy * sensitivity));
                if (m_gizmoDragAxis == 3) tr->scale.z = std::max(0.01f, tr->scale.z + delta);
                if (m_gridSnap) {
                    tr->scale.x = SnapToGrid(tr->scale.x);
                    tr->scale.y = SnapToGrid(tr->scale.y);
                    tr->scale.z = SnapToGrid(tr->scale.z);
                }
            }
        }
    }

    m_mouseX    = x;
    m_mouseY    = y;
    m_dragLastX = x;
    m_dragLastY = y;
}

// EI-03: left-click in viewport → pick entity; RMB/MMB → camera drag
void EditorRenderer::OnMouseButton(int btn, bool pressed) {
    if (btn == 0) { // ── Left mouse ────────────────────────────────────────
        if (pressed) {
            m_leftMousePressed = true;

            // ── Compute floating-panel bounds so we can block click-through ──
            float W = (float)m_width;
            float mainY = kMenuH + kToolbarH;

            // AI chat panel bounds
            float acW = 300.f, acH = 400.f;
            float acX = W - kInspectorW - acW - 4.f;
            float acY = mainY + 10.f;
            bool inAIPanel = m_aiChatVisible &&
                             m_mouseX >= acX && m_mouseX < acX + acW &&
                             m_mouseY >= acY && m_mouseY < acY + acH;

            if (inAIPanel) {
                // Focus the AI input field if the click is on it
                float inputH = 24.f;
                float chatH  = acH - kPanelHdrH - inputH - 4.f;
                float iy     = acY + kPanelHdrH + chatH + 2.f;
                if (m_mouseX >= acX + 2.f && m_mouseX < acX + acW - 40.f &&
                    m_mouseY >= iy         && m_mouseY < iy + inputH - 2.f) {
                    m_aiInputFocused  = true;
                    m_consoleFocused  = false;
                }
                return; // consumed by AI panel – do not pick entities
            }

            // Console input bar bounds
            float consoleY = (float)m_height - kStatusH - kConsoleH;
            float consW    = (float)m_width * 0.65f;
            float inputBarY = consoleY + kConsoleH - 19.f;
            if (m_mouseX >= 2.f && m_mouseX < 2.f + consW - 4.f &&
                m_mouseY >= inputBarY && m_mouseY < inputBarY + 18.f) {
                m_consoleFocused = true;
                m_aiInputFocused = false;
                return; // consumed by console input
            }

            // If click is outside all text inputs, clear focus
            m_consoleFocused = false;
            m_aiInputFocused = false;

            // Check if the click is inside the viewport area
            if (m_mouseX >= m_vpX && m_mouseX < m_vpX + m_vpW &&
                m_mouseY >= m_vpY && m_mouseY < m_vpY + m_vpH)
            {
                // If an entity is selected, check if user clicked on a gizmo axis
                if (m_selectedEntity != 0 && m_world && m_world->IsAlive(m_selectedEntity)) {
                    auto* tr = m_world->GetComponent<Runtime::Components::Transform>(m_selectedEntity);
                    float ex = 0.f, ey = 0.f;
                    float posX = tr ? tr->position.x : 0.f;
                    float posY = tr ? tr->position.y : 0.f;
                    float posZ = tr ? tr->position.z : 0.f;
                    if (Project3D(posX, posY, posZ, ex, ey)) {
                        float gLen = 40.f;
                        for (int axis = 1; axis <= 3; axis++) {
                            if (GizmoAxisHit(ex, ey, axis, gLen)) {
                                m_gizmoDragging = true;
                                m_gizmoDragAxis = axis;
                                m_dragLastX = m_mouseX;
                                m_dragLastY = m_mouseY;
                                // snapshot transform for undo on release
                                if (tr) m_gizmoDragStartTransform = *tr;
                                return; // consumed by gizmo
                            }
                        }
                    }
                }

                // Not on gizmo — pick entity
                uint32_t picked = PickEntityAt((float)m_mouseX, (float)m_mouseY);
                if (picked != m_selectedEntity) {
                    m_selectedEntity = picked;
                    if (picked != 0)
                        Engine::Core::Logger::Info("Selected entity #"
                            + std::to_string(picked) + " (" + EntityName(picked) + ")");
                    else
                        Engine::Core::Logger::Info("Deselected");
                }
            }
        } else {
            // Mouse button released — commit gizmo drag to undo stack
            if (m_gizmoDragging) {
                if (m_world && m_selectedEntity != 0 && m_world->IsAlive(m_selectedEntity)) {
                    if (auto* tr = m_world->GetComponent<Runtime::Components::Transform>(m_selectedEntity)) {
                        UndoableCommandBus::Instance().Execute(
                            std::make_unique<MoveEntityCmd>(m_world, m_selectedEntity,
                                                            m_gizmoDragStartTransform, *tr));
                    }
                }
                m_gizmoDragging = false;
                m_gizmoDragAxis = 0;
            }
        }
    } else if (btn == 1) { // ── Right mouse (orbit / pan) ──────────────────
        m_rmbDown = pressed;
        if (pressed) {
            m_dragLastX = m_mouseX;
            m_dragLastY = m_mouseY;
        }
    } else if (btn == 2) { // ── Middle mouse (pan) ────────────────────────
        m_mmbDown = pressed;
        if (pressed) {
            m_dragLastX = m_mouseX;
            m_dragLastY = m_mouseY;
        }
    }
}

// EI-11: Ctrl+Z / Ctrl+Y undo/redo; EI-13 P = play/stop toggle
void EditorRenderer::OnKey(int key, bool pressed) {
#ifndef GLFW_KEY_Z
    // GLFW key constants (duplicated to avoid including GLFW here)
    static constexpr int K_Z          = 90;
    static constexpr int K_Y          = 89;
    static constexpr int K_W          = 87;
    static constexpr int K_E          = 69;
    static constexpr int K_R          = 82;
    static constexpr int K_G          = 71;
    static constexpr int K_DELETE     = 261;
    static constexpr int K_LEFT_CTRL  = 341;
    static constexpr int K_RIGHT_CTRL = 345;
    static constexpr int K_LEFT_SHIFT = 340;
    static constexpr int K_P          = 80;
    static constexpr int K_S          = 83;
    static constexpr int K_O          = 79;
    static constexpr int K_B          = 66;
    static constexpr int K_F5         = 294;
    static constexpr int K_BACKSPACE  = 259;
    static constexpr int K_ENTER      = 257;
    static constexpr int K_ESCAPE     = 256;
    static constexpr int K_F          = 70;
    static constexpr int K_C          = 67;
    static constexpr int K_V          = 86;
    static constexpr int K_D          = 68;
    static constexpr int K_F1         = 290;
    (void)K_LEFT_SHIFT;
#else
    static constexpr int K_Z          = GLFW_KEY_Z;
    static constexpr int K_Y          = GLFW_KEY_Y;
    static constexpr int K_W          = GLFW_KEY_W;
    static constexpr int K_E          = GLFW_KEY_E;
    static constexpr int K_R          = GLFW_KEY_R;
    static constexpr int K_G          = GLFW_KEY_G;
    static constexpr int K_DELETE     = GLFW_KEY_DELETE;
    static constexpr int K_LEFT_CTRL  = GLFW_KEY_LEFT_CONTROL;
    static constexpr int K_RIGHT_CTRL = GLFW_KEY_RIGHT_CONTROL;
    static constexpr int K_P          = GLFW_KEY_P;
    static constexpr int K_S          = GLFW_KEY_S;
    static constexpr int K_O          = GLFW_KEY_O;
    static constexpr int K_B          = GLFW_KEY_B;
    static constexpr int K_F5         = GLFW_KEY_F5;
    static constexpr int K_BACKSPACE  = GLFW_KEY_BACKSPACE;
    static constexpr int K_ENTER      = GLFW_KEY_ENTER;
    static constexpr int K_ESCAPE     = GLFW_KEY_ESCAPE;
    static constexpr int K_F          = GLFW_KEY_F;
    static constexpr int K_C          = GLFW_KEY_C;
    static constexpr int K_V          = GLFW_KEY_V;
    static constexpr int K_D          = GLFW_KEY_D;
    static constexpr int K_F1         = GLFW_KEY_F1;
#endif

    // Track Ctrl held state (both press and release)
    if (key == K_LEFT_CTRL || key == K_RIGHT_CTRL) {
        m_ctrlHeld = pressed;
        return;
    }

    if (!pressed) return;  // ignore key-up for non-modifier keys

    // ESC: stop PIE first; if not playing, dismiss keybinds/project AI panels
    if (key == K_ESCAPE) {
        if (m_playing) { StopPIE(); return; }
        if (m_keybindsVisible)    { m_keybindsVisible = false; return; }
        if (m_projectAIVisible)   { m_projectAIVisible = false; return; }
        m_consoleFocused   = false;
        m_aiInputFocused   = false;
        m_projectAIFocused = false;
        return;
    }

    // ── Text-input fields take priority (console or AI chat) ──────────────
    if (m_consoleFocused || m_aiInputFocused || m_projectAIFocused) {
        std::string& buf = m_consoleFocused ? m_consoleInput : (m_aiInputFocused ? m_aiInput : m_projectAIInput);
        if (key == K_BACKSPACE) {
            if (!buf.empty()) buf.pop_back();
            return;
        }
        if (key == K_ENTER) {
            if (!buf.empty()) {
                if (m_consoleFocused) {
                    AppendConsole("> " + buf);
                    Engine::Core::Logger::Info(buf);
                } else {
                    // Both AI chat and ProjectAI use m_aiChat->SendMessage
                    if (m_projectAIFocused) m_projectAIScrollY = kScrollToBottom;
                    m_aiChat->SendMessage(buf);
                }
                buf.clear();
            }
            return;
        }
        if (key == K_ESCAPE) {
            if (m_playing) { StopPIE(); return; }
            m_consoleFocused   = false;
            m_aiInputFocused   = false;
            m_projectAIFocused = false;
            return;
        }
        // Allow Ctrl shortcuts even when text field is focused
        if (!m_ctrlHeld) return;
    }

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
            SaveScene(kDefaultScenePath);
        } else if (key == K_O) {
            AppendConsole("[Info]  Open Scene — loading from: " + std::string(kDefaultScenePath));
            LoadScene(kDefaultScenePath);
        } else if (key == K_B) {
            TriggerBuild();
        } else if (key == K_C) {
            // Copy selected entity
            if (m_world && m_selectedEntity != 0 && m_world->IsAlive(m_selectedEntity)) {
                auto* tag = m_world->GetComponent<Runtime::Components::Tag>(m_selectedEntity);
                auto* tr  = m_world->GetComponent<Runtime::Components::Transform>(m_selectedEntity);
                m_clipboard.tag       = tag ? *tag : Runtime::Components::Tag{};
                m_clipboard.transform = tr  ? *tr  : Runtime::Components::Transform{};
                m_clipboard.valid     = true;
                AppendConsole("[Info]  Copied: " + EntityName(m_selectedEntity));
            }
        } else if (key == K_V) {
            // Paste clipboard entity
            if (m_clipboard.valid && m_world) {
                auto t  = m_clipboard.tag;  t.name += "_paste";
                auto tr = m_clipboard.transform; tr.position.x += 2.f;
                UndoableCommandBus::Instance().Execute(
                    std::make_unique<CreateEntityCmd>(m_world, m_selectedEntity, t, tr));
                AppendConsole("[Info]  Pasted: " + t.name);
            }
        } else if (key == K_D) {
            // Duplicate selected entity
            if (m_world && m_selectedEntity != 0 && m_world->IsAlive(m_selectedEntity)) {
                auto* tag = m_world->GetComponent<Runtime::Components::Tag>(m_selectedEntity);
                auto* tr  = m_world->GetComponent<Runtime::Components::Transform>(m_selectedEntity);
                Runtime::Components::Tag       newTag = tag ? *tag : Runtime::Components::Tag{};
                Runtime::Components::Transform newTr  = tr  ? *tr  : Runtime::Components::Transform{};
                newTag.name += "_copy";
                newTr.position.x += 2.f;
                UndoableCommandBus::Instance().Execute(
                    std::make_unique<CreateEntityCmd>(m_world, m_selectedEntity, newTag, newTr));
                AppendConsole("[Info]  Duplicated: " + newTag.name);
            }
        }
    } else {
        if (key == K_P) {
            if (m_playing) { StopPIE(); }
            else { m_playing = true; m_pieTime = 0.f; Engine::Core::Logger::Info("PIE: Play started"); AppendConsole("[Info]  PIE started  (P or ESC to stop)"); }
        } else if (key == K_F5) {
            TriggerBuild();
        }
        // Gizmo mode shortcuts (like Unity/Unreal)
        else if (key == K_W) {
            m_gizmoMode = 0;
            AppendConsole("[Info]  Gizmo mode: Move");
        } else if (key == K_E) {
            m_gizmoMode = 1;
            AppendConsole("[Info]  Gizmo mode: Rotate");
        } else if (key == K_R) {
            m_gizmoMode = 2;
            AppendConsole("[Info]  Gizmo mode: Scale");
        }
        // Grid snap toggle (G key like many editors)
        else if (key == K_G) {
            m_gridSnap = !m_gridSnap;
            AppendConsole(m_gridSnap
                ? "[Info]  Grid snap ON  (size=" + std::to_string(m_gridSize) + ")"
                : "[Info]  Grid snap OFF");
        }
        // F key: reset 3D orbit camera to default view
        else if (key == K_F) {
            m_vpYaw   =  35.f;
            m_vpPitch =  30.f;
            m_vpDist  = 200.f;
            m_vpTargX =   8.f;
            m_vpTargY =   0.f;
            m_vpTargZ =   0.f;
            AppendConsole("[Info]  View reset (3D orbit)");
        }
        // Delete selected entity
        else if (key == K_DELETE) {
            if (m_world && m_selectedEntity != 0 && m_world->IsAlive(m_selectedEntity)) {
                std::string name = EntityName(m_selectedEntity);
                UndoableCommandBus::Instance().Execute(
                    std::make_unique<DeleteEntityCmd>(m_world, m_selectedEntity, m_selectedEntity));
                AppendConsole("[Info]  Deleted entity: " + name);
                Engine::Core::Logger::Info("Deleted entity: " + name);
            }
        }
        else if (key == K_F1) {
            m_keybindsVisible = !m_keybindsVisible;
            if (m_keybindsVisible) AppendConsole("[Info]  Keybinds panel opened  (F1 to close)");
        }
    }
}

void EditorRenderer::AppendConsole(const std::string& line) {
    m_consoleLines.push_back(line);
    if ((int)m_consoleLines.size() > kMaxConsoleLines)
        m_consoleLines.erase(m_consoleLines.begin());
}

// ── Character input — feed focused text field ──────────────────────────────
void EditorRenderer::OnChar(unsigned int codepoint) {
    // Accept printable ASCII and common extended characters
    if (codepoint < 32 || codepoint > 126) return;
    char ch = static_cast<char>(codepoint);
    if (m_consoleFocused) {
        m_consoleInput += ch;
    } else if (m_aiInputFocused) {
        m_aiInput += ch;
    } else if (m_projectAIFocused) {
        m_projectAIInput += ch;
    }
}

// ── Scroll wheel — zoom viewport (change orbit distance) ─────────────────
void EditorRenderer::OnScroll(double /*dx*/, double dy) {
    // Scroll ProjectAI chat history when mouse is over the right sidebar AI panel
    if (m_projectAIVisible) {
        float aiX = (float)m_width - kInspectorW;
        float aiY = kMenuH + kToolbarH;
        float aiH = (float)m_height - kStatusH - kConsoleH - aiY;
        if (m_mouseX >= aiX && m_mouseX < (float)m_width &&
            m_mouseY >= aiY && m_mouseY < aiY + aiH) {
            m_projectAIScrollY = std::max(0.f, m_projectAIScrollY - (float)dy * 20.f);
            return;
        }
    }

    // Only zoom when mouse is over the viewport
    if (m_mouseX >= m_vpX && m_mouseX < m_vpX + m_vpW &&
        m_mouseY >= m_vpY && m_mouseY < m_vpY + m_vpH)
    {
        float factor = (dy > 0) ? 0.88f : (1.f / 0.88f);
        m_vpDist = std::max(1.f, std::min(5000.f, m_vpDist * factor));
    }
}

// ── EI-02 helper: entity display name ────────────────────────────────────
std::string EditorRenderer::EntityName(uint32_t id) const {
    if (!m_world) return "Entity #" + std::to_string(id);
    auto* tag = m_world->GetComponent<Runtime::Components::Tag>(id);
    if (tag && !tag->name.empty()) return tag->name;
    return "Entity #" + std::to_string(id);
}

// ── EI-03: simple spatial pick using 3D projection ────────────────────────
uint32_t EditorRenderer::PickEntityAt(float sx, float sy) {
    if (!m_world) return 0;
    auto entities = m_world->GetEntities();
    if (entities.empty()) return 0;

    // Find the entity whose projected centre is closest to the click
    uint32_t best     = 0;
    float    bestDist = 24.f;  // pixel threshold
    for (auto id : entities) {
        float wx = 0.f, wy = 0.f, wz = 0.f;
        auto* tr = m_world->GetComponent<Runtime::Components::Transform>(id);
        if (tr) { wx = tr->position.x; wy = tr->position.y; wz = tr->position.z; }
        float px, py;
        if (Project3D(wx, wy, wz, px, py)) {
            float d = std::sqrt((px - sx)*(px - sx) + (py - sy)*(py - sy));
            if (d < bestDist) { bestDist = d; best = id; }
        }
    }
    return best;  // 0 = deselect when nothing close enough
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

// ── GizmoAxisHit — returns true if mouse is within ~8px of the axis arrow ─
bool EditorRenderer::GizmoAxisHit(float ex, float ey, int axis, float gLen) const {
    float ax = ex, ay = ey;  // arrow tip
    if (axis == 1) { ax = ex + gLen; ay = ey; }          // X →
    if (axis == 2) { ax = ex;        ay = ey - gLen; }   // Y ↑
    if (axis == 3) { ax = ex + gLen * 0.6f; ay = ey - gLen * 0.6f; } // Z ↗
    // Test: is mouse within a rectangle along the axis shaft?
    float mx = (float)m_mouseX, my = (float)m_mouseY;
    float threshold = 8.f;
    // Simplified hit: distance from mouse to segment (ex,ey)→(ax,ay)
    float dx = ax - ex, dy = ay - ey;
    float len2 = dx * dx + dy * dy;
    if (len2 < 0.0001f) return false;
    float t = ((mx - ex) * dx + (my - ey) * dy) / len2;
    t = std::max(0.f, std::min(1.f, t));
    float px = ex + t * dx, py = ey + t * dy;
    float distSq = (mx - px) * (mx - px) + (my - py) * (my - py);
    return distSq <= threshold * threshold;
}

// ── SnapToGrid ────────────────────────────────────────────────────────────
float EditorRenderer::SnapToGrid(float v) const {
    if (m_gridSize < 0.0001f) return v;
    return std::round(v / m_gridSize) * m_gridSize;
}

// ── SaveScene — write entities to a simple JSON file ─────────────────────
void EditorRenderer::SaveScene(const std::string& path) {
    if (!m_world) { AppendConsole("[Warn]  Cannot save scene: world object is null"); return; }
    try {
        // Make sure the output directory exists
        auto dir = std::filesystem::path(path).parent_path();
        if (!dir.empty()) std::filesystem::create_directories(dir);

        std::ofstream f(path);
        if (!f.is_open()) {
            AppendConsole("[Error] Cannot write scene to: " + path);
            return;
        }
        f << "{\n  \"scene\": \"NovaForge_Dev\",\n  \"entities\": [\n";
        auto entities = m_world->GetEntities();
        for (size_t i = 0; i < entities.size(); i++) {
            auto id = entities[i];
            f << "    {\"id\":" << id;
            auto* tag = m_world->GetComponent<Runtime::Components::Tag>(id);
            if (tag) {
                f << ",\"name\":\"" << tag->name << "\"";
                if (!tag->tags.empty()) {
                    f << ",\"tags\":[";
                    for (size_t ti = 0; ti < tag->tags.size(); ti++) {
                        f << "\"" << tag->tags[ti] << "\"";
                        if (ti + 1 < tag->tags.size()) f << ",";
                    }
                    f << "]";
                }
            }
            auto* tr = m_world->GetComponent<Runtime::Components::Transform>(id);
            if (tr) {
                f << ",\"pos\":[" << tr->position.x << "," << tr->position.y
                  << "," << tr->position.z << "]";
                f << ",\"rot\":[" << tr->rotation.x << "," << tr->rotation.y
                  << "," << tr->rotation.z << "," << tr->rotation.w << "]";
                f << ",\"scale\":[" << tr->scale.x << "," << tr->scale.y
                  << "," << tr->scale.z << "]";
            }
            f << "}";
            if (i + 1 < entities.size()) f << ",";
            f << "\n";
        }
        f << "  ]\n}\n";
        AppendConsole("[Info]  Scene saved → " + path);
        Engine::Core::Logger::Info("Scene saved to " + path);
    } catch (const std::exception& ex) {
        AppendConsole("[Error] Save failed: " + std::string(ex.what()));
    }
}

// ── LoadScene — read entities from a simple JSON file ────────────────────
void EditorRenderer::LoadScene(const std::string& path) {
    if (!m_world) { AppendConsole("[Warn]  Cannot load scene: world object is null"); return; }
    std::ifstream f(path);
    if (!f.is_open()) {
        AppendConsole("[Error] Cannot open scene: " + path);
        return;
    }
    // Minimal JSON parse: look for "name", "pos", "rot", "scale" per entity
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    // Clear existing world
    for (auto id : m_world->GetEntities()) m_world->DestroyEntity(id);
    m_selectedEntity = 0;

    int count = 0;
    size_t pos = 0;
    while ((pos = content.find("\"name\":", pos)) != std::string::npos) {
        size_t s = content.find('"', pos + 7);
        if (s == std::string::npos) break;
        size_t e = content.find('"', s + 1);
        if (e == std::string::npos) break;
        std::string name = content.substr(s + 1, e - s - 1);

        auto id = m_world->CreateEntity();
        Runtime::Components::Tag entityTag;
        entityTag.name = name;
        // Parse tags array if present — search within a window around current pos
        size_t tagSearchStart = (pos > 200) ? pos - 200 : 0;
        size_t tp = content.find("\"tags\":[", tagSearchStart);
        if (tp != std::string::npos && tp < pos + 600) {
            size_t ts = tp + 8;
            size_t te = content.find(']', ts);
            if (te != std::string::npos) {
                size_t tp2 = ts;
                while ((tp2 = content.find('"', tp2)) != std::string::npos && tp2 < te) {
                    size_t te2 = content.find('"', tp2 + 1);
                    if (te2 == std::string::npos || te2 > te) break;
                    entityTag.tags.push_back(content.substr(tp2 + 1, te2 - tp2 - 1));
                    tp2 = te2 + 1;
                }
            }
        }
        m_world->AddComponent(id, entityTag);
        Runtime::Components::Transform tr;
        // Parse pos — guard against npos+1 wrap
        size_t pp = content.find("\"pos\":[", pos);
        if (pp != std::string::npos && pp < pos + 400) {
            try {
                size_t pb = pp + 7;
                tr.position.x = std::stof(content.substr(pb));
                size_t comma1 = content.find(',', pb);
                if (comma1 != std::string::npos) {
                    pb = comma1 + 1;
                    tr.position.y = std::stof(content.substr(pb));
                    size_t comma2 = content.find(',', pb);
                    if (comma2 != std::string::npos) {
                        pb = comma2 + 1;
                        tr.position.z = std::stof(content.substr(pb));
                    }
                }
            } catch (...) {}
        }
        m_world->AddComponent(id, tr);
        ++count;
        pos = e + 1;
    }
    AppendConsole("[Info]  Scene loaded from " + path
                  + " — " + std::to_string(count) + " entities");
    Engine::Core::Logger::Info("Scene loaded: " + path);
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
// 3-D viewport helpers — OpenGL 2.1 immediate-mode
// ──────────────────────────────────────────────────────────────────────────
static constexpr float kVpFOV = 60.f;

// Build column-major perspective matrix and load it
static void GL_LoadPerspective(float fovDeg, float aspect, float nearZ, float farZ) {
    float f  = 1.0f / std::tan(fovDeg * 0.5f * 3.14159265f / 180.f);
    float nf = 1.0f / (nearZ - farZ);
    float m[16] = {
        f / aspect, 0.f,  0.f,                         0.f,
        0.f,        f,    0.f,                         0.f,
        0.f,        0.f,  (farZ + nearZ) * nf,        -1.f,
        0.f,        0.f,  2.0f * farZ * nearZ * nf,    0.f
    };
    glLoadMatrixf(m);
}

// Build column-major lookAt view matrix and load it
static void GL_LoadLookAt(float ex, float ey, float ez,
                           float cx, float cy, float cz) {
    float fx = cx-ex, fy = cy-ey, fz = cz-ez;
    float fl = std::sqrt(fx*fx + fy*fy + fz*fz);
    if (fl < 1e-6f) fl = 1.f;
    fx /= fl; fy /= fl; fz /= fl;

    // right = cross(forward, world_up=(0,1,0))
    float rx = -fz, ry = 0.f, rz = fx;
    float rl = std::sqrt(rx*rx + rz*rz);
    if (rl < 1e-6f) { rx = 1.f; rl = 1.f; }
    rx /= rl; rz /= rl;

    // true up = cross(right, forward)
    float ux = ry*fz - rz*fy;
    float uy = rz*fx - rx*fz;
    float uz = rx*fy - ry*fx;

    // Column-major for glLoadMatrixf
    float m[16] = {
        rx,                       ux,                       -fx,                   0.f,
        ry,                       uy,                       -fy,                   0.f,
        rz,                       uz,                       -fz,                   0.f,
        -(rx*ex+ry*ey+rz*ez),    -(ux*ex+uy*ey+uz*ez),     fx*ex+fy*ey+fz*ez,    1.f
    };
    glLoadMatrixf(m);
}

// Project a world-space point to viewport pixel coordinates using stored camera state.
bool EditorRenderer::Project3D(float wx, float wy, float wz, float& sx, float& sy) const {
    float yawR = m_vpYaw   * 3.14159265f / 180.f;
    float pitR = m_vpPitch * 3.14159265f / 180.f;
    float eyeX = m_vpTargX + m_vpDist * std::cos(pitR) * std::sin(yawR);
    float eyeY = m_vpTargY + m_vpDist * std::sin(pitR);
    float eyeZ = m_vpTargZ + m_vpDist * std::cos(pitR) * std::cos(yawR);

    // Forward (toward target)
    float fx = m_vpTargX - eyeX, fy = m_vpTargY - eyeY, fz = m_vpTargZ - eyeZ;
    float fl = std::sqrt(fx*fx + fy*fy + fz*fz);
    if (fl < 1e-6f) return false;
    fx /= fl; fy /= fl; fz /= fl;

    // Right = cross(forward, up=(0,1,0))
    float rx = -fz, ry = 0.f, rz = fx;
    float rl = std::sqrt(rx*rx + rz*rz);
    if (rl < 1e-6f) rl = 1.f;
    rx /= rl; rz /= rl;

    // True up = cross(right, forward)
    float ux = ry*fz - rz*fy;
    float uy = rz*fx - rx*fz;
    float uz = rx*fy - ry*fx;

    // World to view space
    float dx = wx - eyeX, dy = wy - eyeY, dz = wz - eyeZ;
    float vx = rx*dx + ry*dy + rz*dz;
    float vy = ux*dx + uy*dy + uz*dz;
    float vz = -(fx*dx + fy*dy + fz*dz);  // negated: camera looks in -Z (OpenGL)

    if (vz >= -0.01f) return false;  // behind or on camera (kNearPlaneEpsilon)

    float aspect = (m_vpH > 0.f) ? m_vpW / m_vpH : 1.f;
    float f = 1.0f / std::tan(kVpFOV * 0.5f * 3.14159265f / 180.f);
    float ndcX = (f / aspect) * vx / (-vz);
    float ndcY = f * vy / (-vz);

    if (ndcX < -1.1f || ndcX > 1.1f || ndcY < -1.1f || ndcY > 1.1f) return false;

    sx = m_vpX + (ndcX + 1.f) * 0.5f * m_vpW;
    sy = m_vpY + (1.f - ndcY) * 0.5f * m_vpH;
    return true;
}

// Compute the orbit camera's eye position (shared by renderer and project helper)
static void OrbitEye(float yaw, float pitch, float dist,
                     float tX, float tY, float tZ,
                     float& ex, float& ey, float& ez) {
    float yR = yaw   * 3.14159265f / 180.f;
    float pR = pitch * 3.14159265f / 180.f;
    ex = tX + dist * std::cos(pR) * std::sin(yR);
    ey = tY + dist * std::sin(pR);
    ez = tZ + dist * std::cos(pR) * std::cos(yR);
}

// ── Draw3DViewportScene ────────────────────────────────────────────────────
void EditorRenderer::Draw3DViewportScene(float vx, float vy, float vw, float vh) {
    // Save current GL viewport
    GLint savedVp[4];
    glGetIntegerv(GL_VIEWPORT, savedVp);

    // Restrict to the viewport panel (GL uses bottom-left origin)
    int glX  = (int)vx;
    int glY  = (int)(m_height - vy - vh);
    int glW  = (int)vw;
    int glH  = (int)vh;
    glViewport(glX, glY, glW, glH);

    // Scissor + clear depth only in this region
    glEnable(GL_SCISSOR_TEST);
    glScissor(glX, glY, glW, glH);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    // ── Projection ──────────────────────────────────────────────────────
    glMatrixMode(GL_PROJECTION);
    GL_LoadPerspective(kVpFOV, (glH > 0 ? (float)glW / glH : 1.f), 0.1f, 10000.f);

    // ── View (orbit camera) ─────────────────────────────────────────────
    float ex, ey, ez;
    OrbitEye(m_vpYaw, m_vpPitch, m_vpDist, m_vpTargX, m_vpTargY, m_vpTargZ,
             ex, ey, ez);
    glMatrixMode(GL_MODELVIEW);
    GL_LoadLookAt(ex, ey, ez, m_vpTargX, m_vpTargY, m_vpTargZ);

    // ── Draw 3D scene ──────────────────────────────────────────────────
    DrawFloorGrid3D();
    DrawEntities3D();
    DrawGizmo3D();

    // ── Restore GL state ───────────────────────────────────────────────
    glDisable(GL_DEPTH_TEST);
    glViewport(savedVp[0], savedVp[1], savedVp[2], savedVp[3]);

    // Restore 2-D ortho for UI overlays
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, m_width, m_height, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// ── Floor grid on the Y=0 plane ───────────────────────────────────────────
void EditorRenderer::DrawFloorGrid3D() {
    // Minor grid (every 10 world units, −200 to +200 in X and Z)
    float ext  = 200.f;
    float step = 10.f;

    glLineWidth(1.f);
    glBegin(GL_LINES);
    for (float t = -ext; t <= ext; t += step) {
        uint32_t col = (std::fabs(t) < 0.01f) ? kGridLineMain : kGridLine;
        float r = (float)((col >> 24) & 0xFF) / 255.f;
        float g = (float)((col >> 16) & 0xFF) / 255.f;
        float b = (float)((col >>  8) & 0xFF) / 255.f;
        float a = (float)( col        & 0xFF) / 255.f;
        glColor4f(r, g, b, a);
        glVertex3f(t,    0.f, -ext);  glVertex3f(t,    0.f,  ext);
        glVertex3f(-ext, 0.f,  t);    glVertex3f( ext, 0.f,  t);
    }
    glEnd();

    // Axis lines: X = red, Z = blue
    glLineWidth(2.f);
    glBegin(GL_LINES);
    // X axis
    glColor4ub(0xE0, 0x55, 0x55, 0xFF);
    glVertex3f(-ext, 0.f, 0.f); glVertex3f(ext, 0.f, 0.f);
    // Z axis
    glColor4ub(0x55, 0x55, 0xEE, 0xFF);
    glVertex3f(0.f, 0.f, -ext); glVertex3f(0.f, 0.f, ext);
    // Y axis (green, short)
    glColor4ub(0x55, 0xBB, 0x55, 0xFF);
    glVertex3f(0.f, 0.f, 0.f); glVertex3f(0.f, 30.f, 0.f);
    glEnd();
    glLineWidth(1.f);
}

// Draw a filled sphere using latitude/longitude subdivision
static void DrawFilledSphere3D(float cx, float cy, float cz, float radius,
                                float r, float g, float b, float a,
                                int slices = 18, int stacks = 12) {
    const float pi = 3.14159265f;
    for (int i = 0; i < stacks; i++) {
        float lat0 = pi * (-0.5f + (float) i      / stacks);
        float lat1 = pi * (-0.5f + (float)(i + 1) / stacks);
        float sinL0 = std::sin(lat0), cosL0 = std::cos(lat0);
        float sinL1 = std::sin(lat1), cosL1 = std::cos(lat1);
        // Slightly vary shading per stack for banding effect
        float shade = 0.7f + 0.3f * (float)(i + 1) / stacks;
        glColor4f(r * shade, g * shade, b * shade, a);
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float lng = 2.f * pi * (float)j / slices;
            float cosLng = std::cos(lng), sinLng = std::sin(lng);
            glVertex3f(cx + radius * cosL0 * cosLng, cy + radius * sinL0, cz + radius * cosL0 * sinLng);
            glVertex3f(cx + radius * cosL1 * cosLng, cy + radius * sinL1, cz + radius * cosL1 * sinLng);
        }
        glEnd();
    }
    // Equator highlight ring
    glColor4f(r * 1.1f, g * 1.1f, b * 1.1f, a * 0.5f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    for (int j = 0; j < 32; j++) {
        float ang = 2.f * pi * (float)j / 32;
        glVertex3f(cx + radius * std::cos(ang), cy, cz + radius * std::sin(ang));
    }
    glEnd();
    glLineWidth(1.f);
}

// Draw a wireframe sphere (3 circles) for ships / stations
static void DrawWireSphere3D(float cx, float cy, float cz, float radius,
                              float r, float g, float b, int segments = 20) {
    const float pi = 3.14159265f;
    glColor4f(r, g, b, 1.f);
    for (int plane = 0; plane < 3; plane++) {
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; i++) {
            float ang = 2.f * pi * (float)i / segments;
            float s = std::sin(ang) * radius, c2 = std::cos(ang) * radius;
            if (plane == 0) glVertex3f(cx + c2, cy + s, cz);
            else if (plane == 1) glVertex3f(cx + c2, cy, cz + s);
            else                 glVertex3f(cx, cy + s, cz + c2);
        }
        glEnd();
    }
}

// Helper: draw a wireframe box centred at (x,y,z) with half-size hs
static void DrawWireBox(float x, float y, float z, float hs, bool selected = false) {
    float lw = selected ? 2.5f : 1.f;
    glLineWidth(lw);
    // Bottom ring
    glBegin(GL_LINE_LOOP);
    glVertex3f(x-hs, y-hs, z-hs); glVertex3f(x+hs, y-hs, z-hs);
    glVertex3f(x+hs, y-hs, z+hs); glVertex3f(x-hs, y-hs, z+hs);
    glEnd();
    // Top ring
    glBegin(GL_LINE_LOOP);
    glVertex3f(x-hs, y+hs, z-hs); glVertex3f(x+hs, y+hs, z-hs);
    glVertex3f(x+hs, y+hs, z+hs); glVertex3f(x-hs, y+hs, z+hs);
    glEnd();
    // Verticals
    glBegin(GL_LINES);
    glVertex3f(x-hs, y-hs, z-hs); glVertex3f(x-hs, y+hs, z-hs);
    glVertex3f(x+hs, y-hs, z-hs); glVertex3f(x+hs, y+hs, z-hs);
    glVertex3f(x+hs, y-hs, z+hs); glVertex3f(x+hs, y+hs, z+hs);
    glVertex3f(x-hs, y-hs, z+hs); glVertex3f(x-hs, y+hs, z+hs);
    glEnd();
    glLineWidth(1.f);
}

// ── Draw all ECS entities: spheres for celestial bodies, boxes for others ──
void EditorRenderer::DrawEntities3D() {
    if (!m_world) return;
    auto entities = m_world->GetEntities();

    for (size_t i = 0; i < entities.size(); i++) {
        auto id = entities[i];
        float wx = 0.f, wy = 0.f, wz = 0.f;

        auto* tr  = m_world->GetComponent<Runtime::Components::Transform>(id);
        auto* tag = m_world->GetComponent<Runtime::Components::Tag>(id);
        if (tr) { wx = tr->position.x; wy = tr->position.y; wz = tr->position.z; }

        // PIE orbital animation: orbit non-star entities around Y=0 plane origin
        if (m_playing) {
            float r = std::sqrt(wx*wx + wz*wz);
            if (r > 0.5f) {
                float orbSpeed = kPIEOrbitSpeedFactor / std::max(r, 1.f);
                float theta    = std::atan2(wz, wx) + orbSpeed * m_pieTime;
                wx = r * std::cos(theta);
                wz = r * std::sin(theta);
            }
        }

        bool selected = (id == m_selectedEntity);

        // Determine entity type and visual properties from tags
        enum class EntityKind { Generic, Star, Planet, GasGiant, IceGiant, Moon,
                                 DwarfPlanet, Asteroid, Station, Ship, Background };
        EntityKind kind = EntityKind::Generic;
        float radius = 0.5f;
        float cr = 0.5f, cg = 0.5f, cb = 1.f; // default: blue

        if (tag) {
            for (auto& t : tag->tags) {
                if (t == "Star")         { kind = EntityKind::Star;       radius = 4.0f; cr=1.f;  cg=0.85f; cb=0.2f;  break; }
                if (t == "GasGiant")     { kind = EntityKind::GasGiant;   radius = 2.8f; cr=0.9f; cg=0.6f;  cb=0.3f;  break; }
                if (t == "IceGiant")     { kind = EntityKind::IceGiant;   radius = 2.0f; cr=0.3f; cg=0.7f;  cb=1.0f;  break; }
                if (t == "Planet") {
                    kind = EntityKind::Planet; radius = 1.5f;
                    // Color by planet name
                    std::string n = tag->name;
                    if      (n == "Earth")   { cr=0.2f; cg=0.5f; cb=1.f; }
                    else if (n == "Mars")    { cr=0.9f; cg=0.3f; cb=0.2f; }
                    else if (n == "Venus")   { cr=0.9f; cg=0.8f; cb=0.4f; }
                    else if (n == "Mercury") { cr=0.6f; cg=0.6f; cb=0.6f; }
                    else                     { cr=0.5f; cg=0.6f; cb=0.7f; }
                    break;
                }
                if (t == "Moon")         { kind = EntityKind::Moon;       radius = 0.4f; cr=0.7f; cg=0.7f;  cb=0.7f;  break; }
                if (t == "DwarfPlanet")  { kind = EntityKind::DwarfPlanet;radius = 0.5f; cr=0.6f; cg=0.5f;  cb=0.5f;  break; }
                if (t == "Asteroid")     { kind = EntityKind::Asteroid;   radius = 0.2f; cr=0.5f; cg=0.45f; cb=0.4f;  break; }
                if (t == "AsteroidBelt") { kind = EntityKind::Asteroid;   radius = 0.3f; cr=0.55f;cg=0.5f;  cb=0.45f; break; }
                if (t == "Station")      { kind = EntityKind::Station;    radius = 1.0f; cr=0.7f; cg=0.8f;  cb=0.9f;  break; }
                if (t == "JumpGate")     { kind = EntityKind::Station;    radius = 1.2f; cr=0.4f; cg=0.9f;  cb=0.6f;  break; }
                if (t == "Ship" || t == "Fighter" || t == "PlayerShip" || t == "Freighter") {
                    kind = EntityKind::Ship; radius = 0.4f; cr=0.4f; cg=0.8f; cb=0.4f; break;
                }
                if (t == "Background" || t == "Galaxy") { kind = EntityKind::Background; break; }
            }
        }

        if (kind == EntityKind::Background) continue; // skip skybox entity

        // Selection colour tint
        if (selected) { cr = std::min(1.f,cr*1.3f+0.2f); cg = std::min(1.f,cg*1.3f+0.2f); cb = 0.f; }

        float selLineW = selected ? 2.5f : 1.f;

        switch (kind) {
            case EntityKind::Star:
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                DrawFilledSphere3D(wx, wy, wz, radius, cr, cg, cb, 1.0f, 24, 14);
                // Glow halo
                DrawFilledSphere3D(wx, wy, wz, radius * 1.25f, cr, cg*0.7f, 0.f, 0.18f, 16, 10);
                glDisable(GL_BLEND);
                break;
            case EntityKind::GasGiant:
            case EntityKind::IceGiant:
            case EntityKind::Planet:
            case EntityKind::Moon:
            case EntityKind::DwarfPlanet:
                DrawFilledSphere3D(wx, wy, wz, radius, cr, cg, cb, 1.0f, 20, 12);
                if (selected) {
                    glLineWidth(selLineW);
                    DrawWireSphere3D(wx, wy, wz, radius * 1.15f, 1.f, 0.8f, 0.f, 24);
                    glLineWidth(1.f);
                }
                break;
            case EntityKind::Asteroid:
                glColor4f(cr, cg, cb, 1.f);
                glLineWidth(selLineW);
                DrawWireBox(wx, wy, wz, radius, selected);
                glLineWidth(1.f);
                break;
            case EntityKind::Station:
            case EntityKind::Ship:
            case EntityKind::Generic:
            default:
                glColor4f(cr, cg, cb, 1.f);
                glLineWidth(selLineW);
                DrawWireBox(wx, wy, wz, radius, selected);
                glLineWidth(1.f);
                break;
        }

        // Name label (2D projected overlay)
        float sx = 0.f, sy = 0.f;
        if (tag && Project3D(wx, wy + radius + 0.3f, wz, sx, sy)) {
            DrawText(tag->name, sx - (float)TextWidth(tag->name) * 0.5f, sy - 14.f,
                     selected ? 0xFFDD00FF : 0x888899FF, 0.85f);
        }
    }
}

// ── Draw 3D gizmo (move/rotate/scale arrows) for selected entity ──────────
void EditorRenderer::DrawGizmo3D() {
    if (!m_world || m_selectedEntity == 0) return;
    if (!m_world->IsAlive(m_selectedEntity)) return;

    auto* tr = m_world->GetComponent<Runtime::Components::Transform>(m_selectedEntity);
    float ex = tr ? tr->position.x : 0.f;
    float ey = tr ? tr->position.y : 0.f;
    float ez = tr ? tr->position.z : 0.f;

    float gLen = std::max(1.5f, m_vpDist * 0.05f);  // scale with distance

    glLineWidth(2.5f);
    glBegin(GL_LINES);
    // X axis (red)
    float xc = (m_gizmoDragging && m_gizmoDragAxis == 1) ? 1.f : 0.9f;
    glColor4f(xc, 0.2f, 0.2f, 1.f);
    glVertex3f(ex, ey, ez); glVertex3f(ex + gLen, ey, ez);
    // Y axis (green)
    float yc = (m_gizmoDragging && m_gizmoDragAxis == 2) ? 1.f : 0.9f;
    glColor4f(0.2f, yc, 0.2f, 1.f);
    glVertex3f(ex, ey, ez); glVertex3f(ex, ey + gLen, ez);
    // Z axis (blue)
    float zc = (m_gizmoDragging && m_gizmoDragAxis == 3) ? 1.f : 0.9f;
    glColor4f(0.2f, 0.2f, zc, 1.f);
    glVertex3f(ex, ey, ez); glVertex3f(ex, ey, ez + gLen);
    glEnd();
    glLineWidth(1.f);
}

// ── 2-D axis widget (bottom-left corner of viewport, drawn after 3D) ──────
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

// ── 2-D gizmo arrow overlay (draws screen-space arrows over projected pos) ─
void EditorRenderer::DrawViewportGizmo(float /*vpX*/, float /*vpY*/,
                                        float /*vpW*/, float /*vpH*/) {
    // The 3D gizmo is now drawn in DrawGizmo3D() during the 3D pass.
    // This 2-D overlay only draws a selection ring around the projected entity.
    if (!m_world || m_selectedEntity == 0) return;
    if (!m_world->IsAlive(m_selectedEntity)) return;

    auto* tr = m_world->GetComponent<Runtime::Components::Transform>(m_selectedEntity);
    float px = 0.f, py = 0.f;
    float wx = tr ? tr->position.x : 0.f;
    float wy = tr ? tr->position.y : 0.f;
    float wz = tr ? tr->position.z : 0.f;

    if (Project3D(wx, wy, wz, px, py)) {
        // Small ring around entity centre
        int segs = 16;
        float r = 10.f;
        glColor4f(1.f, 0.8f, 0.f, 0.9f);
        glLineWidth(2.f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segs; i++) {
            float a = (float)i / segs * 6.28318f;
            glVertex2f(px + r * std::cos(a), py + r * std::sin(a));
        }
        glEnd();
        glLineWidth(1.f);

        // Mode label
        static const char* modeNames[] = { "[Move]", "[Rotate]", "[Scale]" };
        DrawText(modeNames[m_gizmoMode], px + 12.f, py - 8.f, 0xFFCC00FF, 0.9f);
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Drawing primitives
// ──────────────────────────────────────────────────────────────────────────
// Draw a rounded rectangle using a fan of triangles at each corner
void EditorRenderer::DrawRoundedRect(float x, float y, float w, float h,
                                      float radius, uint32_t fill) {
    SetColor(fill);
    radius = std::min(radius, std::min(w * 0.5f, h * 0.5f));
    const int segs = 6; // segments per corner
    const float pi = 3.14159265f;
    // Centre quad + 4 corner fans
    glBegin(GL_TRIANGLES);
    // Fill with two centre rects (horizontal and vertical strips)
    auto quad = [&](float qx, float qy, float qw, float qh) {
        glVertex2f(qx,      qy);       glVertex2f(qx + qw, qy);
        glVertex2f(qx + qw, qy + qh);
        glVertex2f(qx,      qy);
        glVertex2f(qx + qw, qy + qh);
        glVertex2f(qx,      qy + qh);
    };
    quad(x + radius, y,          w - 2*radius, h);           // vertical strip
    quad(x,          y + radius, radius,        h - 2*radius); // left strip
    quad(x + w - radius, y + radius, radius, h - 2*radius);   // right strip
    // Corner fans
    float cx2[4] = { x+radius,   x+w-radius, x+w-radius, x+radius   };
    float cy2[4] = { y+radius,   y+radius,   y+h-radius, y+h-radius  };
    float a0[4]  = { pi,         pi*1.5f,    0.f,         pi*0.5f    };
    for (int c = 0; c < 4; c++) {
        for (int i = 0; i < segs; i++) {
            float a1 = a0[c] + (float)i     / segs * (pi * 0.5f);
            float a2 = a0[c] + (float)(i+1) / segs * (pi * 0.5f);
            glVertex2f(cx2[c], cy2[c]);
            glVertex2f(cx2[c] + radius * std::cos(a1), cy2[c] + radius * std::sin(a1));
            glVertex2f(cx2[c] + radius * std::cos(a2), cy2[c] + radius * std::sin(a2));
        }
    }
    glEnd();
}

void EditorRenderer::DrawRoundedRectOutline(float x, float y, float w, float h,
                                             float radius, uint32_t col, float lw) {
    SetColor(col);
    glLineWidth(lw);
    radius = std::min(radius, std::min(w * 0.5f, h * 0.5f));
    const int segs = 6;
    const float pi = 3.14159265f;
    float cx2[4] = { x+radius,   x+w-radius, x+w-radius, x+radius   };
    float cy2[4] = { y+radius,   y+radius,   y+h-radius, y+h-radius  };
    float a0[4]  = { pi,         pi*1.5f,    0.f,         pi*0.5f    };
    glBegin(GL_LINE_LOOP);
    for (int c = 0; c < 4; c++) {
        for (int i = 0; i <= segs; i++) {
            float a = a0[c] + (float)i / segs * (pi * 0.5f);
            glVertex2f(cx2[c] + radius * std::cos(a), cy2[c] + radius * std::sin(a));
        }
    }
    glEnd();
    glLineWidth(1.f);
}

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
    // Slightly brighter top edge for depth illusion
    DrawRect(x, y, w, h, headerCol);
    DrawLine(x, y, x + w, y, (headerCol & 0xFFFFFF00) | 0xFF, 1.f); // brighter top
    DrawLine(x, y + h - 1.f, x + w, y + h - 1.f, 0x00000088, 1.f); // darker bottom
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
    static const char* viewItems[]  = { "Toggle Outliner", "Toggle Inspector",
                                         "Toggle Console", nullptr,
                                         "Toggle Content Browser", "Toggle Code Editor" };
    static const char* toolsItems[] = { "Code Editor", "Content Browser", nullptr,
                                         "Error Panel", "AI Assistant" };
    static const char* sceneItems[] = { "New Entity", "Duplicate Entity", nullptr,
                                         "Save Scene", "Load Scene", nullptr,
                                         "Clear Scene" };
    static const char* buildItems[] = { "Build All  F5", "Build Debug Only", nullptr,
                                         "Clean" };
    static const char* aiItems[]    = { "AI Chat  Ctrl+Space", "Code Completion", nullptr,
                                         "Build Error Fix" };
    static const char** subMenus[kMenuCount] = {
        fileItems, editItems, viewItems, toolsItems, sceneItems, aiItems, buildItems, nullptr
    };
    static const int subMenuCounts[kMenuCount] = { 7, 4, 6, 5, 7, 4, 4, 0 };

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
                if (label.find("Save Scene") != std::string::npos) {
                    SaveScene(kDefaultScenePath);
                } else if (label.find("Load Scene") != std::string::npos
                        || label.find("Open Scene") != std::string::npos) {
                    LoadScene(kDefaultScenePath);
                } else if (label.find("Build All") != std::string::npos
                       ||  label.find("Build Debug") != std::string::npos) {
                    TriggerBuild();
                } else if (label.find("AI Chat") != std::string::npos
                        || label.find("AI Assistant") != std::string::npos) {
                    m_aiChatVisible = !m_aiChatVisible;
                } else if (label.find("Undo") != std::string::npos) {
                    if (UndoableCommandBus::Instance().CanUndo())
                        UndoableCommandBus::Instance().Undo();
                } else if (label.find("Redo") != std::string::npos) {
                    if (UndoableCommandBus::Instance().CanRedo())
                        UndoableCommandBus::Instance().Redo();
                } else if (label.find("Toggle Outliner") != std::string::npos) {
                    m_outlinerVisible = !m_outlinerVisible;
                } else if (label.find("Toggle Inspector") != std::string::npos) {
                    m_inspectorVisible = !m_inspectorVisible;
                } else if (label.find("Toggle Console") != std::string::npos) {
                    m_consoleVisible = !m_consoleVisible;
                } else if (label.find("Toggle Content Browser") != std::string::npos) {
                    m_contentBrowserVisible = !m_contentBrowserVisible;
                } else if (label.find("Toggle Code Editor") != std::string::npos
                        || label.find("Code Editor") != std::string::npos) {
                    m_codeEditorVisible = !m_codeEditorVisible;
                } else if (label.find("Content Browser") != std::string::npos) {
                    m_contentBrowserVisible = !m_contentBrowserVisible;
                } else if (label.find("Error Panel") != std::string::npos) {
                    // error panel is always shown in console area; toggle console
                    m_consoleVisible = !m_consoleVisible;
                } else if (label.find("New Entity") != std::string::npos) {
                    if (m_world) {
                        auto ne = m_world->CreateEntity();
                        Runtime::Components::Tag t; t.name = "New Entity";
                        t.tags.push_back("Entity");
                        m_world->AddComponent(ne, t);
                        Runtime::Components::Transform tr;
                        m_world->AddComponent(ne, tr);
                        m_selectedEntity = ne;
                        Engine::Core::Logger::Info("Scene: New Entity created id=" + std::to_string(ne));
                    }
                } else if (label.find("Duplicate Entity") != std::string::npos) {
                    if (m_world && m_selectedEntity != 0 && m_world->IsAlive(m_selectedEntity)) {
                        auto* srcTag = m_world->GetComponent<Runtime::Components::Tag>(m_selectedEntity);
                        auto* srcTr  = m_world->GetComponent<Runtime::Components::Transform>(m_selectedEntity);
                        Runtime::Components::Tag       newTag = srcTag ? *srcTag : Runtime::Components::Tag{};
                        Runtime::Components::Transform newTr  = srcTr  ? *srcTr  : Runtime::Components::Transform{};
                        newTag.name += "_copy";
                        newTr.position.x += 2.f;
                        UndoableCommandBus::Instance().Execute(
                            std::make_unique<CreateEntityCmd>(m_world, m_selectedEntity, newTag, newTr));
                        Engine::Core::Logger::Info("Scene: Duplicated entity (undoable)");
                    }
                } else if (label.find("Clear Scene") != std::string::npos) {
                    // keep just logging a warning - destructive action
                    Engine::Core::Logger::Warn("Scene: Clear Scene — not implemented (would destroy all entities)");
                }
                m_activeMenu = -1;
            }

            iy += 20.f;
        }
    }

    // Dismiss menu if click happened outside the menu bar and any open sub-menu
    if (m_leftMousePressed && m_activeMenu >= 0) {
        bool inMenuBar = (m_mouseY >= y && m_mouseY < y + h);
        bool inSubMenu = false;
        if (!inMenuBar && subMenus[m_activeMenu]) {
            float dmX = menuXs[m_activeMenu] - 4.f;
            float dmY = y + h;
            float dmW = 180.f;
            float dmH = (float)subMenuCounts[m_activeMenu] * 20.f + 4.f;
            inSubMenu = (m_mouseX >= dmX && m_mouseX < dmX + dmW &&
                         m_mouseY >= dmY && m_mouseY < dmY + dmH);
        }
        if (!inMenuBar && !inSubMenu)
            m_activeMenu = -1;
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Toolbar — EI-13 Play / Stop buttons
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawToolbar(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, 0x2A2A2AFF);
    DrawLine(x, y + h - 1.f, x + w, y + h - 1.f, kBorderColor);

    float by = y + 4.f;
    float bh = 20.f;

    // ── Play / Stop buttons (centre) ────────────────────────────────────
    float bx = x + w * 0.5f - 36.f;
    float bw = 30.f;
    bool playHov = (m_mouseX >= bx && m_mouseX < bx + bw &&
                    m_mouseY >= by && m_mouseY < by + bh);
    DrawRect(bx, by, bw, bh, m_playing ? 0x005522FF : (playHov ? 0x005522AA : 0x003311FF));
    DrawText(m_playing ? "| |" : " > ", bx + 6.f, by + 5.f,
             m_playing ? kTextSuccess : kTextNormal);
    if (playHov && m_leftMousePressed) {
        m_playing = !m_playing;
        Engine::Core::Logger::Info(m_playing ? "PIE: Play started" : "PIE: Play stopped");
    }

    float sx = bx + bw + 4.f;
    bool stopHov = (m_mouseX >= sx && m_mouseX < sx + bw &&
                    m_mouseY >= by && m_mouseY < by + bh);
    DrawRect(sx, by, bw, bh, stopHov ? 0x551100FF : 0x330000FF);
    DrawText(" [ ]", sx + 4.f, by + 5.f, m_playing ? kTextError : kTextMuted);
    if (stopHov && m_leftMousePressed && m_playing) {
        m_playing = false;
        Engine::Core::Logger::Info("PIE: Play stopped");
    }

    // ── Gizmo mode buttons (W/E/R — left side, after Select/Place/Paint) ─
    static const char* gizmoLabels[] = { "W Move", "E Rotate", "R Scale" };
    float gx = x + 8.f;
    for (int i = 0; i < 3; i++) {
        int tw = TextWidth(gizmoLabels[i], 1.f);
        bool active = (m_gizmoMode == i);
        bool ghov   = (m_mouseX >= gx - 2 && m_mouseX < gx + tw + 4 &&
                       m_mouseY >= by      && m_mouseY < by + bh);
        uint32_t bg = active ? 0x094771FF : (ghov ? 0x3E3E52FF : 0x2A2A2AFF);
        DrawRect(gx - 2.f, by, (float)tw + 6.f, bh, bg);
        DrawRectOutline(gx - 2.f, by, (float)tw + 6.f, bh, active ? kHighlight : kBorderColor);
        DrawText(gizmoLabels[i], gx, by + 5.f, active ? 0xFFFFFFFF : kTextNormal);
        if (ghov && m_leftMousePressed) { m_gizmoMode = i; }
        gx += tw + 10.f;
    }

    // ── Grid snap toggle ──────────────────────────────────────────────────
    gx += 4.f;
    const char* snapLabel = m_gridSnap ? "G Snap ON" : "G Snap";
    int stw = TextWidth(snapLabel, 1.f);
    bool snapHov = (m_mouseX >= gx - 2 && m_mouseX < gx + stw + 4 &&
                    m_mouseY >= by      && m_mouseY < by + bh);
    DrawRect(gx - 2.f, by, (float)stw + 6.f, bh,
             m_gridSnap ? 0x226622FF : (snapHov ? 0x3E3E52FF : 0x2A2A2AFF));
    DrawRectOutline(gx - 2.f, by, (float)stw + 6.f, bh,
                    m_gridSnap ? 0x44AA44FF : kBorderColor);
    DrawText(snapLabel, gx, by + 5.f, m_gridSnap ? kTextSuccess : kTextNormal);
    if (snapHov && m_leftMousePressed) {
        m_gridSnap = !m_gridSnap;
        AppendConsole(m_gridSnap ? "[Info]  Grid snap ON" : "[Info]  Grid snap OFF");
    }

    // ── New Entity button ─────────────────────────────────────────────────
    float nex = gx + stw + 18.f;
    bool neHov = (m_mouseX >= nex - 2 && m_mouseX < nex + 62 &&
                  m_mouseY >= by       && m_mouseY < by + bh);
    DrawRect(nex - 2.f, by, 62.f, bh, neHov ? 0x226633FF : 0x1A3322FF);
    DrawRectOutline(nex - 2.f, by, 62.f, bh, 0x44AA66FF);
    DrawText("+ Entity", nex + 2.f, by + 5.f, 0x4EC94EFF);
    if (neHov && m_leftMousePressed && m_world) {
        Runtime::Components::Tag tag;
        tag.name = "NewEntity";
        tag.tags.push_back("Entity");
        Runtime::Components::Transform tr;
        UndoableCommandBus::Instance().Execute(
            std::make_unique<CreateEntityCmd>(m_world, m_selectedEntity, tag, tr));
        AppendConsole("[Info]  Created entity: " + tag.name);
        Engine::Core::Logger::Info("New entity created (undoable)");
    }

    // ── Right side: AI chat toggle ────────────────────────────────────────
    float acx = x + w - 80.f;
    bool achov = (m_mouseX >= acx && m_mouseX < acx + 72.f &&
                  m_mouseY >= by  && m_mouseY < by + bh);
    DrawRect(acx, by, 72.f, bh, m_projectAIVisible ? 0x094771FF : (achov ? 0x3E3E52FF : 0x252526FF));
    DrawRectOutline(acx, by, 72.f, bh, kBorderColor);
    DrawText("AI Chat", acx + 6.f, by + 5.f, kTextAccent);
    if (achov && m_leftMousePressed) m_projectAIVisible = !m_projectAIVisible;
}

// ──────────────────────────────────────────────────────────────────────────
// Viewport — 3-D perspective view (EI-05)
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawViewport(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgViewport);

    // Show yaw/pitch/dist in the header
    char hdrBuf[80];
    snprintf(hdrBuf, sizeof(hdrBuf),
             "  Viewport  [3D Perspective  Yaw:%.0f  Pitch:%.0f  Dist:%.0f]",
             m_vpYaw, m_vpPitch, m_vpDist);
    DrawPanelHeader(hdrBuf, x, y, w, kPanelHdrH, kBgHeader);

    float vx = x, vy = y + kPanelHdrH, vw = w, vh = h - kPanelHdrH;

    // Cache for EI-03 mouse pick and gizmo hit
    m_vpX = vx; m_vpY = vy; m_vpW = vw; m_vpH = vh;

    // ── 3-D scene pass ──────────────────────────────────────────────────
    Draw3DViewportScene(vx, vy, vw, vh);

    // ── 2-D overlays (scissor to viewport region) ───────────────────────
    glScissor((int)vx, (int)(m_height - vy - vh), (int)vw, (int)vh);
    glEnable(GL_SCISSOR_TEST);

    // 2-D gizmo overlay (selection ring + mode label)
    DrawViewportGizmo(vx, vy, vw, vh);
    // 2-D axis indicator (bottom-left corner)
    DrawViewportAxes(vx, vy + vh);

    glDisable(GL_SCISSOR_TEST);

    // Help text (bottom of panel)
    DrawText("RMB: orbit  MMB: pan  Scroll: zoom  LMB: select  F: reset",
             vx + 6.f, vy + vh - 18.f, kTextMuted);

    // Camera info (top-right)
    {
        char distBuf[32];
        snprintf(distBuf, sizeof(distBuf), "Dist: %.0f", m_vpDist);
        int dtw = TextWidth(distBuf);
        DrawText(distBuf, vx + vw - dtw - 8.f, vy + 6.f, kTextMuted);
    }

    if (m_playing) {
        // Green border around viewport to indicate PIE is running
        DrawRectOutline(vx, vy, vw, vh, 0x00CC44FF, 3.f);
        DrawText("  ▶ PIE  (ESC to stop)", vx + vw * 0.5f - 70.f, vy + 8.f, 0x00FF55FF, 1.3f);
    }

    char fpsBuf[32];
    snprintf(fpsBuf, sizeof(fpsBuf), "%.0f FPS", m_fps);
    int tw = TextWidth(fpsBuf);
    DrawText(fpsBuf, vx + vw - tw - 8.f, vy + kPanelHdrH + 4.f, kTextSuccess);
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
    if (hovBtn && m_leftMousePressed && m_selectedEntity != 0) {
        if (!m_addCompMenuOpen) {
            m_addCompMenuOpen       = true;
            m_addCompMenuJustOpened = true;  // suppress close in same frame
        } else {
            m_addCompMenuOpen = false;
        }
    }

    // Add-component popup (drawn as overlay)
    if (m_addCompMenuOpen && m_selectedEntity != 0) {
        DrawAddCompMenu(x, cy - 80.f, w - 12.f, 78.f);
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Add-Component popup — appears above the button, lists available components
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawAddCompMenu(float x, float y, float w, float h) {
    DrawRect(x + 6.f, y, w, h, 0x252526FF);
    DrawRectOutline(x + 6.f, y, w, h, kHighlight);

    struct CompEntry { const char* label; int id; };
    static const CompEntry entries[] = {
        { "Transform",    0 },
        { "MeshRenderer", 1 },
        { "RigidBody",    2 },
        { "Tag",          3 },
    };
    float iy = y + 4.f;
    for (auto& ce : entries) {
        bool hov = (m_mouseX >= x + 8 && m_mouseX < x + 8 + w - 4 &&
                    m_mouseY >= iy    && m_mouseY < iy + 16.f);
        if (hov) DrawRect(x + 8.f, iy, w - 4.f, 16.f, 0x094771FF);
        DrawText(ce.label, x + 12.f, iy + 2.f, hov ? 0xFFFFFFFF : kTextNormal);

        if (hov && m_leftMousePressed && m_world && m_world->IsAlive(m_selectedEntity)) {
            std::string ename = EntityName(m_selectedEntity);
            bool added = false;
            if (ce.id == 0 && !m_world->GetComponent<Runtime::Components::Transform>(m_selectedEntity)) {
                m_world->AddComponent(m_selectedEntity, Runtime::Components::Transform{});
                added = true;
            } else if (ce.id == 1 && !m_world->GetComponent<Runtime::Components::MeshRenderer>(m_selectedEntity)) {
                m_world->AddComponent(m_selectedEntity, Runtime::Components::MeshRenderer{});
                added = true;
            } else if (ce.id == 2 && !m_world->GetComponent<Runtime::Components::RigidBody>(m_selectedEntity)) {
                m_world->AddComponent(m_selectedEntity, Runtime::Components::RigidBody{});
                added = true;
            } else if (ce.id == 3 && !m_world->GetComponent<Runtime::Components::Tag>(m_selectedEntity)) {
                Runtime::Components::Tag t; t.name = ename;
                m_world->AddComponent(m_selectedEntity, t);
                added = true;
            }
            if (added)
                AppendConsole("[Info]  Added " + std::string(ce.label) + " to " + ename);
            else
                AppendConsole("[Info]  " + std::string(ce.label) + " already exists on " + ename);
            m_addCompMenuOpen = false;
        }
        iy += 18.f;
    }

    // Close popup if click outside (but not in the same frame it was opened)
    if (m_leftMousePressed && !m_addCompMenuJustOpened) {
        bool inside = (m_mouseX >= x + 6 && m_mouseX < x + 6 + w &&
                       m_mouseY >= y      && m_mouseY < y + h);
        if (!inside) m_addCompMenuOpen = false;
    }
    m_addCompMenuJustOpened = false;
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
    // Input bar — highlight border when focused
    uint32_t inputBg = m_consoleFocused ? 0x1E2030FF : 0x141414FF;
    uint32_t inputBorder = m_consoleFocused ? kHighlight : kBorderColor;
    DrawRect(x + 2.f, y + h - 19.f, consW - 4.f, 18.f, inputBg);
    DrawRectOutline(x + 2.f, y + h - 19.f, consW - 4.f, 18.f, inputBorder);
    std::string displayText = "> " + m_consoleInput + (m_consoleFocused ? "_" : "");
    DrawText(displayText, x + 6.f, y + h - 16.f, kTextAccent);

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

// ── EI-14: AI chat panel ─────────────────────────────────────────────────
// Helper: wrap a string to fit within maxChars characters per line,
// splitting on spaces. Returns the wrapped lines.
static std::vector<std::string> WrapText(const std::string& text, int maxChars) {
    std::vector<std::string> lines;
    if (text.empty()) { lines.push_back(""); return lines; }
    std::istringstream stream(text);
    std::string word, current;
    while (stream >> word) {
        if (!current.empty() && (int)(current.size() + 1 + word.size()) > maxChars) {
            lines.push_back(current);
            current = word;
        } else {
            if (!current.empty()) current += ' ';
            current += word;
        }
    }
    if (!current.empty()) lines.push_back(current);
    if (lines.empty()) lines.push_back("");
    return lines;
}

void EditorRenderer::DrawAIChat(float x, float y, float w, float h) {
    if (!m_aiChatVisible) return;

    // Panel background with subtle gradient feel
    DrawRoundedRect(x, y, w, h, 6.f, 0x1A1A24FF);
    DrawRoundedRectOutline(x, y, w, h, 6.f, 0x4A4A6AFF, 1.5f);

    // Header
    DrawRoundedRect(x, y, w, kPanelHdrH, 6.f, 0x2A2A3EFF);
    DrawLine(x, y + kPanelHdrH, x + w, y + kPanelHdrH, 0x4A4A6AFF);
    // "\xE2\x9C\xBF" = U+270F (pencil), "\xe2\x80\x94" = U+2014 (em-dash)
    DrawText("  \xE2\x9C\xBF AI Chat  \xe2\x80\x94  Ollama", x + 6.f, y + 4.f, 0x88AAFFFF, 1.1f);

    // Close button
    float cbx = x + w - 22.f;
    bool closeHov = (m_mouseX >= cbx && m_mouseX < cbx + 20.f &&
                     m_mouseY >= y   && m_mouseY < y + kPanelHdrH);
    DrawRoundedRect(cbx, y + 2.f, 18.f, kPanelHdrH - 4.f, 3.f,
                    closeHov ? 0xE04040FF : 0x3A3A4AFF);
    DrawText("x", cbx + 5.f, y + 5.f, 0xFFFFFFFF);
    if (closeHov && m_leftMousePressed) m_aiChatVisible = false;

    float inputH = 30.f;
    float chatH  = h - kPanelHdrH - inputH - 8.f;
    float bubblePad = 6.f;
    float bubbleMaxW = w * 0.78f;
    // Approximate pixels-per-character for stb_easy_font at scale 1
    static constexpr float kApproxCharW = 7.f;
    int   charsPerLine = std::max(10, (int)(bubbleMaxW / kApproxCharW));

    // Scissor the conversation area
    glScissor((int)x, (int)(m_height - y - kPanelHdrH - chatH), (int)w, (int)chatH);
    glEnable(GL_SCISSOR_TEST);

    const auto& msgs = m_aiChat->Messages();

    // First pass: compute total height to render from bottom up
    float fy = y + kPanelHdrH + chatH - 4.f;
    for (int i = (int)msgs.size() - 1; i >= 0 && fy > y + kPanelHdrH; i--) {
        const auto& msg = msgs[i];
        bool isUser = (msg.role == IDE::ChatRole::User);
        const std::string prefix = isUser ? "You" : "AI";
        auto lines = WrapText(msg.content, charsPerLine);
        float lineH = 13.f;
        float bubbleH = (float)lines.size() * lineH + bubblePad * 2.f + 12.f; // +12 for label
        fy -= bubbleH + 6.f;
        if (fy < y + kPanelHdrH) break;

        float bubbleW = 0.f;
        for (auto& l : lines)
            bubbleW = std::max(bubbleW, (float)TextWidth(l, 1.f) + bubblePad * 2.f + 4.f);
        bubbleW = std::min(bubbleW, bubbleMaxW);
        bubbleW = std::max(bubbleW, 60.f);

        float bx = isUser ? (x + w - bubbleW - 8.f) : (x + 8.f);

        // Bubble background
        uint32_t bubbleBg  = isUser ? 0x1155AACC : 0x2A2A3ACC;
        uint32_t bubbleBdr = isUser ? 0x3388FFFF : 0x5555AAFF;
        DrawRoundedRect(bx, fy, bubbleW, bubbleH, 8.f, bubbleBg);
        DrawRoundedRectOutline(bx, fy, bubbleW, bubbleH, 8.f, bubbleBdr, 1.f);

        // Sender label
        uint32_t labelCol = isUser ? 0x88CCFFFF : 0xAABBFFFF;
        DrawText(prefix + ":", bx + bubblePad, fy + 4.f, labelCol, 0.9f);

        // Message lines
        float ly = fy + 16.f;
        uint32_t textCol = isUser ? 0xEEEEFFFF : 0xCCDDFFFF;
        for (auto& l : lines) {
            DrawText(l, bx + bubblePad, ly, textCol, 1.f);
            ly += lineH;
        }
    }
    glDisable(GL_SCISSOR_TEST);

    // Input area
    float iy = y + kPanelHdrH + chatH + 4.f;
    DrawLine(x + 4.f, iy - 2.f, x + w - 4.f, iy - 2.f, 0x3A3A5AFF);

    uint32_t aiBg     = m_aiInputFocused ? 0x1E2040FF : 0x14141EFF;
    uint32_t aiBorder = m_aiInputFocused ? 0x5577FFFF : 0x3A3A5AFF;
    DrawRoundedRect(x + 4.f, iy, w - 48.f, inputH - 2.f, 5.f, aiBg);
    DrawRoundedRectOutline(x + 4.f, iy, w - 48.f, inputH - 2.f, 5.f, aiBorder, 1.5f);
    DrawText(m_aiInput + (m_aiInputFocused ? "_" : ""), x + 10.f, iy + 8.f, kTextNormal);
    if (m_aiInput.empty() && !m_aiInputFocused)
        DrawText("Ask the AI...", x + 10.f, iy + 8.f, 0x555577FF);

    bool sendHov = (m_mouseX >= x + w - 42.f && m_mouseX < x + w - 4.f &&
                    m_mouseY >= iy && m_mouseY < iy + inputH - 2.f);
    DrawRoundedRect(x + w - 42.f, iy, 38.f, inputH - 2.f, 5.f,
                    sendHov ? 0x2288DDFF : 0x1166BBFF);
    DrawText("Send", x + w - 38.f, iy + 8.f, 0xFFFFFFFF);
    if (sendHov && m_leftMousePressed && !m_aiInput.empty()) {
        // Project-aware quick responses for NovaForge game design questions
        std::string q = m_aiInput;
        std::string lower = q;
        for (auto& c : lower) c = (char)std::tolower((unsigned char)c);

        std::string autoReply;
        if (lower.find("faction") != std::string::npos ||
            lower.find("solari") != std::string::npos  ||
            lower.find("veyren") != std::string::npos  ||
            lower.find("aurelian")!= std::string::npos ||
            lower.find("keldari") != std::string::npos) {
            autoReply = "NovaForge has 4 factions: Solari (armor+energy), Veyren (shield+hybrids), Aurelian (speed+drones+ECM), Keldari (missiles+ECM). Void Syndicate is the main pirate faction. Starting system: Thyrkstad (Veyren high-sec). Rep range: -1000 to +1000.";
        } else if (lower.find("gas") != std::string::npos ||
                   lower.find("harvest") != std::string::npos) {
            autoReply = "Gas harvesting: anchor near a Gas Giant and deploy harvest drones. Gas types: Nebuline-C50 (common, polymers), Vaporin (uncommon lowsec, combat boosters), Helion-3 (rare, fusion fuel). Drones collect gas into cargo hold.";
        } else if (lower.find("land") != std::string::npos ||
                   lower.find("planet") != std::string::npos) {
            autoReply = "Landing system: approach within 50 world units of a Landable planet (Rocky/Habitable) to see the landing prompt [F]. Gas Giants and Ice Giants are orbital-anchor-only — deploy harvest drones. Planet surface uses same modular PCG as ship interiors.";
        } else if (lower.find("ship") != std::string::npos ||
                   lower.find("class") != std::string::npos) {
            autoReply = "Ship classes: Frigate < Destroyer < Cruiser < Battlecruiser < Battleship < Capital < Titan. Ships = module graphs (interior-driven hull). Tags: Fighter/Freighter/Miner/Industrial. Player starts in Nova Fighter Mk1 (Frigate class).";
        } else if (lower.find("module") != std::string::npos ||
                   lower.find("build") != std::string::npos) {
            autoReply = "Ship Builder: drag-and-drop snap-grid. Modules: Engine, Reactor, WeaponSpine, BellyBay, DroneBay, RigDock. Size classes: XS/S/M/L/XL/XXL/XXXL/Titan. Interior layout drives exterior hull shape. F2 = toggle build mode.";
        } else if (lower.find("mine") != std::string::npos ||
                   lower.find("ore") != std::string::npos ||
                   lower.find("asteroid") != std::string::npos) {
            autoReply = "Mining: approach an asteroid belt (AsteroidBelt tag), activate mining laser. Ore types by security: highsec=dustite/ferrite, lowsec=corite/crystite/nocxium_ore. Use mining barges for volume; exhumers for deep ore.";
        } else if (lower.find("warp") != std::string::npos ||
                   lower.find("jump") != std::string::npos ||
                   lower.find("travel") != std::string::npos) {
            autoReply = "Travel: approach a Jump Gate (JumpGate tag) to warp to connected systems. Warp channels are cinematic — layered audio + visual tunnel. Fleet warps in formation. Wormholes: scan anomalies to find unstable connections to null-sec regions.";
        } else if (lower.find("editor") != std::string::npos ||
                   lower.find("atlas") != std::string::npos) {
            autoReply = "AtlasEditor: W=Move, E=Rotate, R=Scale gizmos. G=grid snap. Del=delete entity. F5=quicksave. F9=quickload. View menu=toggle panels. Scene menu=New/Duplicate Entity. Build menu=compile project. AI Chat=this panel (Ctrl+Space).";
        }

        if (!autoReply.empty()) {
            m_aiChat->AppendMessage(IDE::ChatRole::User,      q);
            m_aiChat->AppendMessage(IDE::ChatRole::Assistant, autoReply);
        } else {
            m_aiChat->SendMessage(m_aiInput);
        }
        m_aiInput.clear();
    }

    // ── Quick-action buttons below input ──────────────────────────────────────
    static const char* kQuickLabels[] = {
        "Factions","Gas Harvest","Planet Land","Ship Classes","Mining","Warp"
    };
    static const char* kQuickQueries[] = {
        "What are the factions in NovaForge?",
        "How does gas harvesting work?",
        "How do I land on a planet?",
        "What are the ship classes?",
        "How does mining work?",
        "How does warp travel work?"
    };
    float qbY = iy + inputH + 2.f;
    float qbW = (w - 8.f) / 3.f;
    float qbH = 14.f;
    for (int qi = 0; qi < 6; qi++) {
        int col   = qi % 3;
        int row   = qi / 3;
        float qbX = x + 4.f + col * qbW;
        float qbYi = qbY + row * (qbH + 2.f);
        bool qhov = (m_mouseX >= qbX && m_mouseX < qbX + qbW - 2.f &&
                     m_mouseY >= qbYi && m_mouseY < qbYi + qbH);
        DrawRoundedRect(qbX, qbYi, qbW - 2.f, qbH, 3.f,
                        qhov ? 0x223355FF : 0x161625FF);
        DrawRoundedRectOutline(qbX, qbYi, qbW - 2.f, qbH, 3.f, 0x334466FF);
        DrawText(kQuickLabels[qi], qbX + 3.f, qbYi + 3.f, 0x7799BBFF, 0.85f);
        if (qhov && m_leftMousePressed && m_aiInput.empty()) {
            m_aiInput = kQuickQueries[qi];
        }
    }

    // Click on input to focus
    bool inputClick = (m_mouseX >= x + 4.f && m_mouseX < x + w - 48.f &&
                       m_mouseY >= iy && m_mouseY < iy + inputH - 2.f);
    if (inputClick && m_leftMousePressed) m_aiInputFocused = true;
}

// ──────────────────────────────────────────────────────────────────────────
// Status bar
// ──────────────────────────────────────────────────────────────────────────
void EditorRenderer::DrawStatusBar(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, kBgStatusBar);

    static const char* gizmoNames[] = { "Move", "Rotate", "Scale" };
    int entityCount = m_world ? (int)m_world->EntityCount() : 0;
    char statusBuf[256];
    snprintf(statusBuf, sizeof(statusBuf),
             "  %s  |  Scene: NovaForge_Dev  |  Entities: %d  |  Gizmo: %s%s%s%s",
             UndoableCommandBus::Instance().CanUndo()
                 ? ("Undo: " + UndoableCommandBus::Instance().NextUndoDescription()).c_str()
                 : "Ready",
             entityCount,
             gizmoNames[m_gizmoMode],
             m_gridSnap ? "  |  Grid ON" : "",
             m_playing  ? "  |  ▶ PIE"  : "",
             m_gizmoDragging ? "  |  DRAGGING" : "");
    DrawText(statusBuf, x + 4.f, y + 5.f, 0xFFFFFFFF);

    char buf[80];
    snprintf(buf, sizeof(buf), "W/E/R:gizmo  G:snap  Del:delete  %.0f FPS  |  GL 2.1  ", m_fps);
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

    // EI-13: tick ECS world when PIE is active
    if (m_playing && m_world) {
        m_world->Update((float)dt);
    }

    // Accumulate PIE simulation time
    if (m_playing) m_pieTime += (float)dt;

    // Poll AI future and flush response to chat
    if (m_aiFuture.valid() &&
        m_aiFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        std::string reply = m_aiFuture.get();
        m_aiChat->AppendMessage(IDE::ChatRole::Assistant, reply);
    }

    float W = (float)m_width;
    float H = (float)m_height;

    glViewport(0, 0, m_width, m_height);
    glClearColor(R(kBgBase)/255.f, G(kBgBase)/255.f, B(kBgBase)/255.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    SetupOrtho();

    // ── Layout geometry ────────────────────────────────────────────────────
    float menuY    = 0.f;
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
    // Right sidebar: ProjectAI panel (AI Chat) when toggled, otherwise Inspector
    if (m_projectAIVisible)
        DrawProjectAIPanel(inspectorX, mainY, kInspectorW, mainH);
    else
        DrawInspector(inspectorX, mainY,  kInspectorW, mainH);
    DrawConsole  (0.f,        consoleY, W, kConsoleH);

    // ── Borders between panels ─────────────────────────────────────────────
    DrawLine(kOutlinerW,  mainY, kOutlinerW,  consoleY, kBorderColor, 1.5f);
    DrawLine(inspectorX,  mainY, inspectorX,  consoleY, kBorderColor, 1.5f);
    DrawLine(0.f, consoleY, W, consoleY,                kBorderColor, 1.5f);

    // ── Bars (drawn on top) ────────────────────────────────────────────────
    DrawToolbar (0.f, toolbarY, W, kToolbarH);
    DrawMenuBar (0.f, menuY,  W, kMenuH);
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

    // Keybinds reference panel — F1
    if (m_keybindsVisible) {
        float kbW = std::min(700.f, W - 40.f);
        float kbH = std::min(460.f, H - 60.f);
        float kbX = (W - kbW) * 0.5f;
        float kbY = (H - kbH) * 0.5f;
        DrawKeybindsPanel(kbX, kbY, kbW, kbH);
    }

    // Reset single-frame click flag after all UI has had a chance to see it
    m_leftMousePressed = false;
}

// ── Keybinds reference panel (F1) ─────────────────────────────────────────
void EditorRenderer::DrawKeybindsPanel(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, 0x1A1A2EF0);
    DrawRectOutline(x, y, w, h, kHighlight, 2.f);
    DrawPanelHeader("  Keybinds Reference  —  F1 to close", x, y, w, kPanelHdrH, 0x094771FF);

    float cy = y + kPanelHdrH + 8.f;
    float col1 = x + 12.f;
    float col2 = x + w * 0.5f + 4.f;
    float rh = 16.f;

    struct KBRow { const char* key; const char* action; };
    static const KBRow rows[] = {
        { "RMB drag",        "Orbit camera"               },
        { "MMB drag",        "Pan camera"                 },
        { "Scroll",          "Zoom in / out"              },
        { "LMB",             "Select entity"              },
        { "F",               "Reset camera to scene"      },
        { "W",               "Move gizmo"                 },
        { "E",               "Rotate gizmo"               },
        { "R",               "Scale gizmo"                },
        { "G",               "Toggle grid snap"           },
        { "P",               "Play (PIE)"                 },
        { "ESC  (in PIE)",   "Stop PIE"                   },
        { "Ctrl+Z",          "Undo"                       },
        { "Ctrl+Y",          "Redo"                       },
        { "Ctrl+C",          "Copy entity"                },
        { "Ctrl+V",          "Paste entity"               },
        { "Ctrl+D",          "Duplicate entity"           },
        { "Delete",          "Delete entity"              },
        { "Ctrl+S",          "Save scene"                 },
        { "Ctrl+O",          "Open scene"                 },
        { "Ctrl+B",          "Build project"              },
        { "F5",              "Build project"              },
        { "F1",              "Keybinds panel"             },
        { "Ctrl+Space",      "Toggle AI Chat sidebar"     },
        { "ESC  (panels)",   "Close keybinds / AI sidebar"},
        { nullptr, nullptr }
    };

    int  i = 0;
    bool left = true;
    DrawText("Action", col1 + 80.f, cy, kTextMuted, 0.9f);
    DrawText("Action", col2 + 80.f, cy, kTextMuted, 0.9f);
    DrawText("Key",    col1,        cy, kTextMuted, 0.9f);
    DrawText("Key",    col2,        cy, kTextMuted, 0.9f);
    cy += rh;
    DrawLine(x + 8.f, cy - 2.f, x + w - 8.f, cy - 2.f, kBorderColor);

    for (const KBRow* r = rows; r->key; ++r, ++i) {
        float cx2 = left ? col1 : col2;
        float ry  = cy;
        if (!left) ry -= rh;
        if ((i & 1) == 0) DrawRect(cx2 - 4.f, ry, w * 0.5f - 8.f, rh, 0x0D0D1A80);
        DrawText(r->key,    cx2,         ry + 2.f, 0xFFCC44FF, 0.9f);
        DrawText(r->action, cx2 + 80.f,  ry + 2.f, kTextNormal, 0.9f);
        if (!left) cy += rh;
        left = !left;
    }
    if (!left) cy += rh;

    cy += 4.f;
    DrawLine(x + 8.f, cy, x + w - 8.f, cy, kBorderColor);
    DrawText("Press F1 or ESC to close", x + w * 0.5f - 70.f, cy + 4.f, kTextMuted, 0.9f);
}

// ── ProjectAI — ChatGPT-style panel with project file access ───────────────
void EditorRenderer::DrawProjectAIPanel(float x, float y, float w, float h) {
    DrawRect(x, y, w, h, 0x0D0D1AFF);
    DrawPanelHeader("  Atlas AI  (Ollama)", x, y, w, kPanelHdrH, 0x0A2040FF);

    // Close button (top-right) — returns to Inspector
    float cbx = x + w - 22.f, cby = y + 3.f;
    bool  cbHov = (m_mouseX >= cbx && m_mouseX < cbx + 18.f &&
                   m_mouseY >= cby && m_mouseY < cby + 14.f);
    DrawRect(cbx, cby, 18.f, 14.f, cbHov ? 0x881111FF : 0x441111FF);
    DrawText("X", cbx + 4.f, cby + 2.f, 0xFF6655FF);
    if (cbHov && m_leftMousePressed) {
        m_projectAIVisible = false;
        return;
    }

    float inputH     = 28.f;
    float statusH    = 18.f;
    float chatAreaH  = h - kPanelHdrH - inputH - statusH - 8.f;
    float chatY      = y + kPanelHdrH + 2.f;
    float inputY     = chatY + chatAreaH + 4.f;
    float statusY    = inputY + inputH + 2.f;

    // ── Chat history area ────────────────────────────────────────────────
    DrawRect(x + 2.f, chatY, w - 4.f, chatAreaH, 0x08080EFF);
    glScissor((int)(x + 2), (int)(m_height - chatY - chatAreaH), (int)(w - 4), (int)chatAreaH);
    glEnable(GL_SCISSOR_TEST);

    const auto& msgs = m_aiChat->Messages();
    float lineH   = 15.f;
    float totalH  = 0.f;
    for (auto& msg : msgs) {
        int lines = std::max(1, (int)(msg.content.size() / 60) + 1);
        totalH += lineH * lines + 4.f;
    }
    float maxScroll = std::max(0.f, totalH - chatAreaH + 8.f);
    m_projectAIScrollY = std::min(m_projectAIScrollY, maxScroll);

    float ty = chatY + 4.f - m_projectAIScrollY;
    for (auto& msg : msgs) {
        bool  isUser = (msg.role == IDE::ChatRole::User);
        uint32_t col = isUser ? 0x88BBFFFF : (msg.role == IDE::ChatRole::System ? 0x888888FF : 0xCCDDCCFF);
        const char* prefix = isUser ? "You: " : "AI:  ";
        uint32_t bgCol = isUser ? 0x0A1828FF : 0x081208FF;

        std::string full = std::string(prefix) + msg.content;
        int charsPerLine = std::max(1, (int)((w - 20.f) / kApproxCharWidth));
        for (size_t ci = 0; ci < full.size(); ci += charsPerLine) {
            std::string line = full.substr(ci, charsPerLine);
            if (ty + lineH >= chatY && ty < chatY + chatAreaH) {
                DrawRect(x + 4.f, ty, w - 8.f, lineH, bgCol);
                DrawText(line, x + 8.f, ty + 2.f, col, 0.9f);
            }
            ty += lineH;
        }
        ty += 4.f;
    }

    glDisable(GL_SCISSOR_TEST);

    // Scroll hint
    if (maxScroll > 0.f) {
        float scrollFrac = m_projectAIScrollY / maxScroll;
        float sbH = std::max(20.f, chatAreaH * chatAreaH / (totalH + 1.f));
        float sbY = chatY + scrollFrac * (chatAreaH - sbH);
        DrawRect(x + w - 6.f, chatY, 4.f, chatAreaH, 0x1A1A2EFF);
        DrawRect(x + w - 6.f, sbY,   4.f, sbH,       0x336699FF);
    }

    // ── Input box ────────────────────────────────────────────────────────
    bool inputHov = (m_mouseX >= x + 4 && m_mouseX < x + w - 44.f &&
                     m_mouseY >= inputY && m_mouseY < inputY + inputH - 2.f);
    if (inputHov && m_leftMousePressed) {
        m_projectAIFocused = true;
        m_consoleFocused   = false;
        m_aiInputFocused   = false;
    }
    DrawRect(x + 4.f, inputY, w - 48.f, inputH - 2.f,
             m_projectAIFocused ? 0x0A1828FF : 0x080810FF);
    DrawRectOutline(x + 4.f, inputY, w - 48.f, inputH - 2.f,
                    m_projectAIFocused ? kHighlight : kBorderColor);
    std::string display = m_projectAIInput;
    if (m_projectAIFocused && ((int)(m_fps * 1.5) % 2 == 0)) display += "|";
    DrawText(display.empty() ? "Ask about the project, code, or planning…"
                             : display,
             x + 8.f, inputY + 7.f,
             display.empty() ? kTextMuted : kTextNormal, 0.95f);

    // Send button
    float sbx = x + w - 42.f;
    bool  sbHov = (m_mouseX >= sbx && m_mouseX < sbx + 38.f &&
                   m_mouseY >= inputY && m_mouseY < inputY + inputH - 2.f);
    DrawRect(sbx, inputY, 38.f, inputH - 2.f, sbHov ? 0x1155BBFF : 0x0A3377FF);
    DrawRectOutline(sbx, inputY, 38.f, inputH - 2.f, 0x336699FF);
    DrawText("Send", sbx + 4.f, inputY + 7.f, 0xAADDFFFF);

    bool sendNow = (sbHov && m_leftMousePressed && !m_projectAIInput.empty());

    if (sendNow) {
        std::string msg = m_projectAIInput;
        m_projectAIInput.clear();
        m_aiChat->SendMessage(msg);
        m_projectAIScrollY = kScrollToBottom;
    }

    // ── Status bar ───────────────────────────────────────────────────────
    DrawRect(x + 2.f, statusY, w - 4.f, statusH - 2.f, 0x050510FF);
    bool pending = m_aiFuture.valid() &&
                   m_aiFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready;
    DrawText(pending ? "  ⏳ Waiting for Ollama…"
                     : "  codellama | localhost:11434 | Ctrl+Space",
             x + 6.f, statusY + 3.f, pending ? kTextWarn : kTextMuted, 0.9f);
}

} // namespace Editor
