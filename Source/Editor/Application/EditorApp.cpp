#include "Editor/Application/EditorApp.h"
#include "Core/Config/ProjectManifest.h"
#include "Core/Logging/Log.h"
#include <chrono>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace NF::Editor {

#ifdef _WIN32
static LRESULT CALLBACK EditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
#endif

bool EditorApp::Init() {
    Logger::Log(LogLevel::Info, "Editor", "[1/6] EditorApp — loading project manifest");

    // Step 1 — load project manifest
    ProjectManifest manifest;
    {
        const std::string manifestPath = "Config/novaforge.project.json";
        if (manifest.LoadFromFile(manifestPath))
        {
            manifest.LogSummary();
        }
        else
        {
            Logger::Log(LogLevel::Warning, "Editor",
                        "Project manifest not found; using defaults");
        }
    }

    Logger::Log(LogLevel::Info, "Editor", "[2/6] EditorApp — creating platform window");

#ifdef _WIN32
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = EditorWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"NovaForgeEditor";

    if (!RegisterClassExW(&wc))
    {
        Logger::Log(LogLevel::Error, "Editor", "Failed to register window class");
        return false;
    }

    HWND hwnd = CreateWindowExW(
        0,
        L"NovaForgeEditor",
        L"NovaForge Editor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 720,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        Logger::Log(LogLevel::Error, "Editor", "Failed to create window");
        UnregisterClassW(L"NovaForgeEditor", hInstance);
        return false;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    m_Hwnd = hwnd;
#endif

    Logger::Log(LogLevel::Info, "Editor", "[3/6] EditorApp — initialising render device");
    m_RenderDevice = std::make_unique<RenderDevice>();
    if (!m_RenderDevice->Init(GraphicsAPI::Null)) {
        Logger::Log(LogLevel::Error, "Editor", "RenderDevice init failed");
        return false;
    }

    Logger::Log(LogLevel::Info, "Editor", "[4/6] EditorApp — loading level");
    m_Level.Load("untitled.level");

    Logger::Log(LogLevel::Info, "Editor", "[5/6] EditorApp — setting up viewport");
    // Viewport
    m_Viewport.Init(m_RenderDevice.get());
    m_Viewport.Resize(m_ClientWidth, m_ClientHeight);

    Logger::Log(LogLevel::Info, "Editor", "[6/6] EditorApp — wiring panels and layout");
    // Panels
    m_SceneOutliner.SetWorld(&m_Level.GetWorld());
    m_SceneOutliner.SetOnSelectionChanged([this](EntityId id) {
        m_Inspector.SetSelectedEntity(id, &m_Level.GetWorld());
    });

    // Use manifest content root if available, else default
    m_ContentBrowser.SetRootPath(
        manifest.IsValid() ? manifest.ContentRoot : "Content");

    // Register panels with the docking system
    m_DockingSystem.RegisterPanel("SceneOutliner",
        [this](float, float, float, float) { m_SceneOutliner.Draw(); });
    m_DockingSystem.RegisterPanel("Viewport",
        [this](float, float, float, float) { m_Viewport.Draw(); });
    m_DockingSystem.RegisterPanel("Inspector",
        [this](float, float, float, float) { m_Inspector.Draw(); });
    m_DockingSystem.RegisterPanel("ContentBrowser",
        [this](float, float, float, float) { m_ContentBrowser.Draw(); });
    m_DockingSystem.RegisterPanel("Console",
        [this](float, float, float, float) { m_ConsolePanel.Draw(); });

    // Default layout:
    //   SceneOutliner (20%) | Viewport (56%) | Inspector (24%)
    //                          Viewport splits vertically: Viewport (75%) / Console (25%)
    //                                           Inspector splits vertically: Inspector (60%) / ContentBrowser (40%)
    m_DockingSystem.SetRootSplit("SceneOutliner", "Viewport", 0.20f);
    m_DockingSystem.SplitPanel("Viewport",  "Inspector",     SplitAxis::Horizontal, 0.70f);
    m_DockingSystem.SplitPanel("Inspector", "ContentBrowser", SplitAxis::Vertical,  0.60f);
    m_DockingSystem.SplitPanel("Viewport",  "Console",        SplitAxis::Vertical,  0.75f);

    m_Running = true;
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Init complete");
    return true;
}

void EditorApp::TickFrame(float dt)
{
    m_RenderDevice->BeginFrame();
    m_RenderDevice->Clear(0.18f, 0.18f, 0.18f, 1.f);
    m_Level.Update(dt);

    m_DockingSystem.Update(dt);
    m_SceneOutliner.Update(dt);
    m_Inspector.Update(dt);
    m_ContentBrowser.Update(dt);
    m_ConsolePanel.Update(dt);
    m_Viewport.Update(dt);
    m_DockingSystem.Draw(static_cast<float>(m_ClientWidth),
                         static_cast<float>(m_ClientHeight));

    m_RenderDevice->EndFrame();
}

void EditorApp::Run() {
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Run – entering editor loop");

    using Clock = std::chrono::steady_clock;
    auto lastTime = Clock::now();

#ifdef _WIN32
    MSG msg{};
    while (m_Running)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                m_Running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Propagate window resize to the viewport
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
                    m_Viewport.Resize(m_ClientWidth, m_ClientHeight);
                }
            }
        }

        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        TickFrame(dt);
    }
#else
    while (m_Running) {
        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        TickFrame(dt);
    }
#endif
}

void EditorApp::Shutdown() {
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Shutdown");
    m_Level.Unload();
    m_RenderDevice->Shutdown();

#ifdef _WIN32
    if (m_Hwnd && IsWindow(static_cast<HWND>(m_Hwnd)))
        DestroyWindow(static_cast<HWND>(m_Hwnd));
    m_Hwnd = nullptr;
    UnregisterClassW(L"NovaForgeEditor", GetModuleHandleW(nullptr));
#endif
}

} // namespace NF::Editor
