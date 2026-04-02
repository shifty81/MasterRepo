#include "GameClientApp.h"
#include "Core/Logging/Log.h"
#include "Core/Math/Matrix.h"
#include "Core/Math/Vector.h"
#include "Game/Interaction/RigState.h"
#include "Game/Interaction/Inventory.h"
#include "Game/Interaction/ResourceItem.h"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

#ifdef _WIN32
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0A00
#  undef  _WIN32_WINNT
#  define _WIN32_WINNT 0x0A00
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#endif

namespace NF::Game {

// ---------------------------------------------------------------------------
// Colour palette (0xRRGGBBAA)
// ---------------------------------------------------------------------------
static constexpr uint32_t kBgColor      = 0x0D0D0FFF; // near-black sky
static constexpr uint32_t kHudBg        = 0x1A1A1ECC; // semi-transparent HUD strip
static constexpr uint32_t kHPColor      = 0x22CC44FF; // green health
static constexpr uint32_t kENColor      = 0x2277FFFF; // blue energy
static constexpr uint32_t kBarBg        = 0x2A2A2AFF; // bar background
static constexpr uint32_t kTextColor    = 0xCCCCCCFF;
static constexpr uint32_t kTitleColor   = 0xFFFFFFFF;
static constexpr uint32_t kSepColor     = 0x444444FF;
static constexpr uint32_t kItemColor    = 0xA87A3BFF; // amber inventory

#ifdef _WIN32

// ---------------------------------------------------------------------------
// Win32 window procedure
// ---------------------------------------------------------------------------

static LRESULT CALLBACK GameClientWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }

    GameClientApp* app = reinterpret_cast<GameClientApp*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (app)
        app->DispatchOsEvent(msg,
            static_cast<uintptr_t>(wParam),
            static_cast<intptr_t>(lParam));

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// DispatchOsEvent
// ---------------------------------------------------------------------------

void GameClientApp::DispatchOsEvent(unsigned msg, uintptr_t wParam, intptr_t lParam) noexcept
{
    const WPARAM wp = static_cast<WPARAM>(wParam);
    const LPARAM lp = static_cast<LPARAM>(lParam);

    switch (msg)
    {
    case WM_LBUTTONDOWN:
        m_MouseX = static_cast<float>(GET_X_LPARAM(lp));
        m_MouseY = static_cast<float>(GET_Y_LPARAM(lp));
        m_LeftDown        = true;
        m_LeftJustPressed = true;
        SetCapture(static_cast<HWND>(m_Hwnd));
        break;

    case WM_LBUTTONUP:
        m_LeftDown = false;
        ReleaseCapture();
        break;

    case WM_RBUTTONDOWN:
        m_RightDown = true;
        SetCapture(static_cast<HWND>(m_Hwnd));
        break;

    case WM_RBUTTONUP:
        m_RightDown = false;
        ReleaseCapture();
        break;

    case WM_MOUSEMOVE:
    {
        const float newX = static_cast<float>(GET_X_LPARAM(lp));
        const float newY = static_cast<float>(GET_Y_LPARAM(lp));
        if (m_MouseTracking) {
            m_MouseDeltaX += newX - m_PrevMouseX;
            m_MouseDeltaY += newY - m_PrevMouseY;
        }
        m_PrevMouseX = newX;
        m_PrevMouseY = newY;
        m_MouseX = newX;
        m_MouseY = newY;
        m_MouseTracking = true;
        break;
    }

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        const unsigned vk = static_cast<unsigned>(wParam) & 0xFFu;
        if (!m_Keys[vk])
            m_KeysJustPressed[vk] = true;
        m_Keys[vk] = true;
        break;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        const unsigned vk = static_cast<unsigned>(wParam) & 0xFFu;
        m_Keys[vk] = false;
        break;
    }

    case WM_DPICHANGED:
    {
        m_DpiScale = static_cast<float>(HIWORD(wp)) / 96.f;
        m_UIRenderer.SetDpiScale(m_DpiScale);
        const RECT* r = reinterpret_cast<const RECT*>(lp);
        SetWindowPos(static_cast<HWND>(m_Hwnd), nullptr,
                     r->left, r->top,
                     r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        break;
    }

    default:
        break;
    }
}

#endif // _WIN32

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

bool GameClientApp::Init()
{
    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "[1/4] Creating platform window");

#ifdef _WIN32
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HINSTANCE hInst = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = GameClientWndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"NovaForgeGame";

    if (!RegisterClassExW(&wc)) {
        NF::Logger::Log(NF::LogLevel::Error, "GameClient", "Failed to register window class");
        return false;
    }

    HWND hwnd = CreateWindowExW(
        0, L"NovaForgeGame", L"NovaForge",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        nullptr, nullptr, hInst, nullptr);

    if (!hwnd) {
        NF::Logger::Log(NF::LogLevel::Error, "GameClient", "Failed to create window");
        UnregisterClassW(L"NovaForgeGame", hInst);
        return false;
    }

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    {
        UINT dpi = GetDpiForWindow(hwnd);
        m_DpiScale = static_cast<float>(dpi) / 96.f;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    m_Hwnd = hwnd;
#endif

    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "[2/4] Initialising render device");
    m_RenderDevice = std::make_unique<NF::RenderDevice>();

#ifdef NF_HAS_OPENGL
    const NF::GraphicsAPI api = NF::GraphicsAPI::OpenGL;
#else
    const NF::GraphicsAPI api = NF::GraphicsAPI::Null;
#endif

    if (!m_RenderDevice->Init(api, m_Hwnd)) {
        NF::Logger::Log(NF::LogLevel::Error, "GameClient", "RenderDevice init failed");
        return false;
    }
    m_RenderDevice->Resize(m_ClientWidth, m_ClientHeight);

    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "[3/4] Initialising UI renderer");
    m_UIRenderer.Init();
    m_UIRenderer.SetViewportSize(static_cast<float>(m_ClientWidth),
                                  static_cast<float>(m_ClientHeight));
    m_UIRenderer.SetDpiScale(m_DpiScale);

    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "[4/4] Initialising game orchestrator");
    if (!m_Orchestrator.Init(m_RenderDevice.get())) {
        NF::Logger::Log(NF::LogLevel::Error, "GameClient", "Orchestrator init failed");
        return false;
    }

    // Phase 4: initialise the forward renderer and chunk mesh cache.
    m_ForwardRenderer.Init(m_RenderDevice.get());
    m_MeshCache.Init(&m_ForwardRenderer);
    m_MeshCache.RebuildDirty(m_Orchestrator.GetGameWorld().GetChunkMap());

    m_Running = true;
    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "GameClientApp::Init complete");
    return true;
}

// ---------------------------------------------------------------------------
// Run
// ---------------------------------------------------------------------------

void GameClientApp::Run()
{
    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "Entering game loop");

    using Clock = std::chrono::steady_clock;
    auto lastTime = Clock::now();

#ifdef _WIN32
    MSG msg{};
    while (m_Running)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) {
                m_Running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Detect window resize
        {
            RECT rc{};
            if (GetClientRect(static_cast<HWND>(m_Hwnd), &rc))
            {
                int newW = rc.right  - rc.left;
                int newH = rc.bottom - rc.top;
                if (newW != m_ClientWidth || newH != m_ClientHeight)
                {
                    m_ClientWidth  = newW;
                    m_ClientHeight = newH;
                    m_RenderDevice->Resize(m_ClientWidth, m_ClientHeight);
                }
            }
        }

        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        TickFrame(dt);
        FlushFrameInput();
    }
#else
    // Headless loop for CI / non-Windows platforms — run 120 frames
    // (approximately 2 seconds at 60 FPS) to exercise all game systems
    // then exit so automated tests can validate correctness.
    constexpr int kHeadlessFrames = 120;
    for (int frame = 0; frame < kHeadlessFrames && m_Running; ++frame)
    {
        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        TickFrame(dt);
    }
    m_Running = false;
#endif
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

void GameClientApp::Shutdown()
{
    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "GameClientApp::Shutdown");
    m_MeshCache.Shutdown();
    m_ForwardRenderer.Shutdown();
    m_Orchestrator.Shutdown();
    m_UIRenderer.Shutdown();
    m_RenderDevice->Shutdown();

#ifdef _WIN32
    if (m_Hwnd && IsWindow(static_cast<HWND>(m_Hwnd)))
        DestroyWindow(static_cast<HWND>(m_Hwnd));
    m_Hwnd = nullptr;
    UnregisterClassW(L"NovaForgeGame", GetModuleHandleW(nullptr));
#endif
}

// ---------------------------------------------------------------------------
// TickFrame
// ---------------------------------------------------------------------------

void GameClientApp::TickFrame(float dt)
{
    // --- Phase 5: read input and feed to PlayerMovement ---
    {
        auto& pm = m_Orchestrator.GetPlayerMovement();

        // WASD movement (Win32 VK codes: W=0x57, A=0x41, S=0x53, D=0x44)
        float forward = 0.f, right = 0.f;
        if (m_Keys[0x57]) forward += 1.f; // W
        if (m_Keys[0x53]) forward -= 1.f; // S
        if (m_Keys[0x44]) right   += 1.f; // D
        if (m_Keys[0x41]) right   -= 1.f; // A

        const bool jump   = m_KeysJustPressed[0x20]; // Space
        const bool sprint = m_Keys[0x10];             // Shift

        pm.SetMoveInput(forward, right, jump, sprint);

        // Mouse look: apply accumulated delta while RMB held (or always).
        if (m_RightDown && (m_MouseDeltaX != 0.f || m_MouseDeltaY != 0.f))
            pm.ApplyMouseLook(m_MouseDeltaX, m_MouseDeltaY);
    }

    m_Orchestrator.Tick(dt);

    // ---- Phase 4: render voxel chunk meshes ----
    m_MeshCache.RebuildDirty(m_Orchestrator.GetGameWorld().GetChunkMap());
    {
        NF::Matrix4x4 view = GetViewMatrix();
        NF::Matrix4x4 proj = GetProjectionMatrix();
        m_ForwardRenderer.BeginScene(view, proj);
        m_MeshCache.Render();
        m_ForwardRenderer.EndScene();
    }

    // ---- 2-D HUD overlay ----
    m_UIRenderer.SetViewportSize(static_cast<float>(m_ClientWidth),
                                  static_cast<float>(m_ClientHeight));
    m_UIRenderer.BeginFrame();
    DrawHUD();
    m_UIRenderer.EndFrame();
}

// ---------------------------------------------------------------------------
// DrawHUD
// ---------------------------------------------------------------------------

void GameClientApp::DrawHUD()
{
    const float dpi   = m_UIRenderer.GetDpiScale();
    const float lineH = 18.f * dpi;
    const float barH  = 14.f * dpi;
    const float padX  = 8.f  * dpi;
    const float hudW  = 260.f * dpi;
    const float hudX  = padX;
    const float hudY  = static_cast<float>(m_ClientHeight) - (lineH * 7.f + 12.f * dpi);
    const float scale = 2.f;

    // HUD background panel
    m_UIRenderer.DrawRect({hudX - 4.f * dpi, hudY - 4.f * dpi,
                            hudW + 8.f * dpi, lineH * 7.f + 16.f * dpi}, kHudBg);

    const NF::Game::RigState&   rig = m_Orchestrator.GetInteractionLoop().GetRig();
    const NF::Game::Inventory&  inv = m_Orchestrator.GetInteractionLoop().GetInventory();

    float cy = hudY;

    // Title
    m_UIRenderer.DrawText("NovaForge", hudX, cy, kTitleColor, scale);
    cy += lineH;
    m_UIRenderer.DrawRect({hudX, cy, hudW, 1.f}, kSepColor);
    cy += 4.f * dpi;

    // Rig name
    m_UIRenderer.DrawText(("Rig: " + rig.GetName()).c_str(), hudX, cy, kTextColor, scale);
    cy += lineH;

    // Health bar
    {
        const float hp = rig.GetHealth();
        const float frac = std::max(0.f, std::min(1.f, hp / NF::Game::RigState::kMaxHealth));
        m_UIRenderer.DrawRect({hudX, cy, hudW, barH}, kBarBg);
        if (frac > 0.f)
            m_UIRenderer.DrawRect({hudX, cy, hudW * frac, barH}, kHPColor);
        const std::string hpLabel = "HP " + std::to_string(static_cast<int>(hp))
                                  + " / " + std::to_string(static_cast<int>(NF::Game::RigState::kMaxHealth));
        m_UIRenderer.DrawText(hpLabel.c_str(), hudX + 4.f * dpi, cy + 2.f * dpi, kTextColor, scale);
        cy += barH + 3.f * dpi;
    }

    // Energy bar
    {
        const float en = rig.GetEnergy();
        const float frac = std::max(0.f, std::min(1.f, en / NF::Game::RigState::kMaxEnergy));
        m_UIRenderer.DrawRect({hudX, cy, hudW, barH}, kBarBg);
        if (frac > 0.f)
            m_UIRenderer.DrawRect({hudX, cy, hudW * frac, barH}, kENColor);
        const std::string enLabel = "EN " + std::to_string(static_cast<int>(en))
                                  + " / " + std::to_string(static_cast<int>(NF::Game::RigState::kMaxEnergy));
        m_UIRenderer.DrawText(enLabel.c_str(), hudX + 4.f * dpi, cy + 2.f * dpi, kTextColor, scale);
        cy += barH + 3.f * dpi;
    }

    // Inventory — show total item count across all slots
    {
        uint32_t total = 0;
        for (int i = 0; i < NF::Game::Inventory::kMaxSlots; ++i) {
            const auto& slot = inv.GetSlot(i);
            if (!slot.IsEmpty()) total += slot.count;
        }
        const std::string invLabel = "Inventory: " + std::to_string(total) + " items";
        m_UIRenderer.DrawText(invLabel.c_str(), hudX, cy, kItemColor, scale);
        cy += lineH;
    }

    // Position info (Phase 5)
    {
        const auto& pm = m_Orchestrator.GetPlayerMovement();
        const auto& pos = pm.GetPosition();
        const std::string posLabel = "Pos: " + std::to_string(static_cast<int>(pos.X))
                                   + ", " + std::to_string(static_cast<int>(pos.Y))
                                   + ", " + std::to_string(static_cast<int>(pos.Z))
                                   + (pm.IsGrounded() ? " [GND]" : " [AIR]");
        m_UIRenderer.DrawText(posLabel.c_str(), hudX, cy, kTextColor, scale);
    }
}

// ---------------------------------------------------------------------------
// FlushFrameInput
// ---------------------------------------------------------------------------

void GameClientApp::FlushFrameInput() noexcept
{
    for (int i = 0; i < 256; ++i)
        m_KeysJustPressed[i] = false;
    m_LeftJustPressed = false;
    m_MouseDeltaX = 0.f;
    m_MouseDeltaY = 0.f;
}

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------

NF::Matrix4x4 GameClientApp::GetViewMatrix() const noexcept
{
    const auto& pm = m_Orchestrator.GetPlayerMovement();
    const NF::Vector3 eye = pm.GetEyePosition();
    const NF::Vector3 viewDir = pm.GetViewDirection();

    // Target is eye + viewDir.
    const NF::Vector3 target{eye.X + viewDir.X, eye.Y + viewDir.Y, eye.Z + viewDir.Z};

    NF::Vector3 forward = viewDir;
    NF::Vector3 right   = forward.Cross({0.f, 1.f, 0.f}).Normalized();
    NF::Vector3 up      = right.Cross(forward);

    NF::Matrix4x4 v = NF::Matrix4x4::Identity();
    v.M[0][0] = right.X;    v.M[1][0] = right.Y;    v.M[2][0] = right.Z;
    v.M[0][1] = up.X;       v.M[1][1] = up.Y;       v.M[2][1] = up.Z;
    v.M[0][2] = -forward.X; v.M[1][2] = -forward.Y; v.M[2][2] = -forward.Z;
    v.M[3][0] = -right.Dot(eye);
    v.M[3][1] = -up.Dot(eye);
    v.M[3][2] =  forward.Dot(eye);
    return v;
}

NF::Matrix4x4 GameClientApp::GetProjectionMatrix() const noexcept
{
    const float aspect = (m_ClientHeight > 0)
        ? static_cast<float>(m_ClientWidth) / static_cast<float>(m_ClientHeight) : 1.f;
    const float fovY  = 0.7854f; // ~45 degrees
    const float nearZ = 0.1f, farZ = 1000.f;
    const float f     = 1.f / std::tan(fovY * 0.5f);

    NF::Matrix4x4 p{};
    p.M[0][0] = f / aspect;
    p.M[1][1] = f;
    p.M[2][2] = (farZ + nearZ) / (nearZ - farZ);
    p.M[2][3] = -1.f;
    p.M[3][2] = (2.f * farZ * nearZ) / (nearZ - farZ);
    return p;
}

} // namespace NF::Game
