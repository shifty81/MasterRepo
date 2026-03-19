#include "Win32Window.h"

#ifdef _WIN32

#include "../core/Logger.h"
#include <GL/gl.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

namespace atlas::platform {

static const wchar_t* ATLAS_WINDOW_CLASS = L"AtlasWindowClass";

/// Translate a Win32 virtual-key code to a platform-independent key code.
static uint32_t TranslateVK(uint32_t vk) {
    if (vk == VK_F3) return kKeyF3;
    return vk;
}

Win32Window::~Win32Window() {
    Shutdown();
}

void Win32Window::PushEvent(const WindowEvent& event) {
    int next = (m_eventTail + 1) % MAX_EVENTS;
    if (next == m_eventHead) return; // Queue full, drop event
    m_eventQueue[m_eventTail] = event;
    m_eventTail = next;
}

LRESULT CALLBACK Win32Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Win32Window* self = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return 0;
        }
        case WM_CLOSE: {
            if (self) {
                WindowEvent event;
                event.type = WindowEvent::Type::Close;
                self->PushEvent(event);
                self->m_open = false;
            }
            return 0;
        }
        case WM_SIZE: {
            if (self) {
                int32_t w = LOWORD(lParam);
                int32_t h = HIWORD(lParam);
                if (w != self->m_width || h != self->m_height) {
                    self->m_width = w;
                    self->m_height = h;
                    WindowEvent event;
                    event.type = WindowEvent::Type::Resize;
                    event.width = w;
                    event.height = h;
                    self->PushEvent(event);
                }
            }
            return 0;
        }
        case WM_KEYDOWN: {
            if (self) {
                WindowEvent event;
                event.type = WindowEvent::Type::KeyDown;
                event.keyCode = TranslateVK(static_cast<uint32_t>(wParam));
                event.modifiers = 0;
                if (GetKeyState(VK_CONTROL) & 0x8000) event.modifiers |= kModCtrl;
                if (GetKeyState(VK_SHIFT) & 0x8000)   event.modifiers |= kModShift;
                if (GetKeyState(VK_MENU) & 0x8000)    event.modifiers |= kModAlt;
                self->PushEvent(event);
            }
            return 0;
        }
        case WM_KEYUP: {
            if (self) {
                WindowEvent event;
                event.type = WindowEvent::Type::KeyUp;
                event.keyCode = TranslateVK(static_cast<uint32_t>(wParam));
                event.modifiers = 0;
                if (GetKeyState(VK_CONTROL) & 0x8000) event.modifiers |= kModCtrl;
                if (GetKeyState(VK_SHIFT) & 0x8000)   event.modifiers |= kModShift;
                if (GetKeyState(VK_MENU) & 0x8000)    event.modifiers |= kModAlt;
                self->PushEvent(event);
            }
            return 0;
        }
        case WM_CHAR: {
            if (self) {
                WindowEvent event;
                event.type = WindowEvent::Type::TextInput;
                event.textChar = static_cast<char>(wParam);
                self->PushEvent(event);
            }
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN: {
            if (self) {
                WindowEvent event;
                event.type = WindowEvent::Type::MouseButtonDown;
                event.mouseX = static_cast<int32_t>(LOWORD(lParam));
                event.mouseY = static_cast<int32_t>(HIWORD(lParam));
                if (msg == WM_LBUTTONDOWN) event.mouseButton = 0;
                else if (msg == WM_RBUTTONDOWN) event.mouseButton = 2;
                else event.mouseButton = 1;
                self->PushEvent(event);
            }
            return 0;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP: {
            if (self) {
                WindowEvent event;
                event.type = WindowEvent::Type::MouseButtonUp;
                event.mouseX = static_cast<int32_t>(LOWORD(lParam));
                event.mouseY = static_cast<int32_t>(HIWORD(lParam));
                if (msg == WM_LBUTTONUP) event.mouseButton = 0;
                else if (msg == WM_RBUTTONUP) event.mouseButton = 2;
                else event.mouseButton = 1;
                self->PushEvent(event);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (self) {
                WindowEvent event;
                event.type = WindowEvent::Type::MouseMove;
                event.mouseX = static_cast<int32_t>(LOWORD(lParam));
                event.mouseY = static_cast<int32_t>(HIWORD(lParam));
                self->PushEvent(event);
            }
            return 0;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        default:
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool Win32Window::Init(const PlatformWindowConfig& config) {
    m_hInstance = GetModuleHandle(nullptr);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = ATLAS_WINDOW_CLASS;

    if (!RegisterClassExW(&wc)) {
        // Class may already be registered from a previous window
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            atlas::Logger::Error("Win32: Failed to register window class");
            return false;
        }
    }

    // Calculate window size to get the desired client area
    RECT rect = { 0, 0, config.width, config.height };
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!config.resizable) {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    AdjustWindowRect(&rect, style, FALSE);

    // Convert title from std::string to wide string
    int titleLen = MultiByteToWideChar(CP_UTF8, 0, config.title.c_str(), -1, nullptr, 0);
    std::wstring wideTitle(titleLen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, config.title.c_str(), -1, wideTitle.data(), titleLen);

    m_hwnd = CreateWindowExW(
        0,
        ATLAS_WINDOW_CLASS,
        wideTitle.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        m_hInstance,
        this  // Pass 'this' as lpParam for WM_CREATE
    );

    if (!m_hwnd) {
        atlas::Logger::Error("Win32: Failed to create window");
        return false;
    }

    m_hdc = GetDC(m_hwnd);
    if (!m_hdc) {
        atlas::Logger::Error("Win32: Failed to get device context");
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
        return false;
    }

    // Set up pixel format for OpenGL
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(m_hdc, &pfd);
    if (pixelFormat == 0) {
        atlas::Logger::Error("Win32: Failed to choose pixel format");
        ReleaseDC(m_hwnd, m_hdc);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
        m_hdc = nullptr;
        return false;
    }

    if (!SetPixelFormat(m_hdc, pixelFormat, &pfd)) {
        atlas::Logger::Error("Win32: Failed to set pixel format");
        ReleaseDC(m_hwnd, m_hdc);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
        m_hdc = nullptr;
        return false;
    }

    m_glContext = wglCreateContext(m_hdc);
    if (!m_glContext) {
        atlas::Logger::Error("Win32: Failed to create OpenGL context");
        ReleaseDC(m_hwnd, m_hdc);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
        m_hdc = nullptr;
        return false;
    }

    if (!wglMakeCurrent(m_hdc, m_glContext)) {
        atlas::Logger::Error("Win32: Failed to make OpenGL context current");
        wglDeleteContext(m_glContext);
        m_glContext = nullptr;
        ReleaseDC(m_hwnd, m_hdc);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
        m_hdc = nullptr;
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    m_width = config.width;
    m_height = config.height;
    m_open = true;

    atlas::Logger::Info("Win32 window created (" + std::to_string(m_width) + "x" + std::to_string(m_height) + ")");
    return true;
}

void Win32Window::Shutdown() {
    if (m_glContext) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(m_glContext);
        m_glContext = nullptr;
    }
    if (m_hdc && m_hwnd) {
        ReleaseDC(m_hwnd, m_hdc);
        m_hdc = nullptr;
    }
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    m_open = false;
}

bool Win32Window::PollEvent(WindowEvent& event) {
    if (!m_hwnd || !m_open) return false;

    // Process pending Windows messages
    MSG msg;
    while (PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Return buffered events
    if (m_eventHead != m_eventTail) {
        event = m_eventQueue[m_eventHead];
        m_eventHead = (m_eventHead + 1) % MAX_EVENTS;
        return true;
    }

    event.type = WindowEvent::Type::NoEvent;
    return false;
}

void Win32Window::SwapBuffers() {
    if (m_hdc) {
        ::SwapBuffers(m_hdc);
    }
}

} // namespace atlas::platform

#endif // _WIN32
