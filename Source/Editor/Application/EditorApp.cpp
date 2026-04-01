#include "Editor/Application/EditorApp.h"
#include "Core/Logging/Log.h"

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
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Init");

#ifdef _WIN32
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = EditorWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
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

    m_RenderDevice = std::make_unique<RenderDevice>();
    if (!m_RenderDevice->Init(GraphicsAPI::Null)) {
        Logger::Log(LogLevel::Error, "Editor", "RenderDevice init failed");
        return false;
    }
    m_Level.Load("untitled.level");
    m_Running = true;
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Init complete");
    return true;
}

void EditorApp::Run() {
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Run – entering editor loop");

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

        m_RenderDevice->BeginFrame();
        m_RenderDevice->Clear(0.18f, 0.18f, 0.18f, 1.f);
        m_Level.Update(0.016f);
        m_RenderDevice->EndFrame();
    }
#else
    while (m_Running) {
        m_RenderDevice->BeginFrame();
        m_RenderDevice->Clear(0.18f, 0.18f, 0.18f, 1.f);
        m_Level.Update(0.016f);
        m_RenderDevice->EndFrame();
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
