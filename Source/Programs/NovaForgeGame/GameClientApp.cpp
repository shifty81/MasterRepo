#include "GameClientApp.h"
#include "Core/Logging/Log.h"
#include "Game/Interaction/RigState.h"
#include "Game/Interaction/Inventory.h"
#include "Game/Interaction/ResourceItem.h"
#include <chrono>
#include <algorithm>
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
    // Headless loop for CI / non-Windows platforms — run for a fixed number of
    // frames and exit so automated tests can validate the game systems.
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
    m_Orchestrator.Tick(dt);

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
    const float hudY  = static_cast<float>(m_ClientHeight) - (lineH * 6.f + 12.f * dpi);
    const float scale = 2.f;

    // HUD background panel
    m_UIRenderer.DrawRect({hudX - 4.f * dpi, hudY - 4.f * dpi,
                            hudW + 8.f * dpi, lineH * 6.f + 16.f * dpi}, kHudBg);

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
}

} // namespace NF::Game
