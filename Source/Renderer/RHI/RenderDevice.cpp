#include "Renderer/RHI/RenderDevice.h"
#include "Core/Logging/Log.h"

#ifdef NF_HAS_OPENGL
#include "Renderer/RHI/GLHeaders.h"
#endif

#if defined(NF_HAS_OPENGL) && defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace NF {

bool RenderDevice::Init(GraphicsAPI api, void* nativeWindowHandle) {
    m_API = api;
    m_NativeWindowHandle = nativeWindowHandle;

#if defined(NF_HAS_OPENGL) && defined(_WIN32)
    if (api == GraphicsAPI::OpenGL) {
        if (!nativeWindowHandle) {
            NF_LOG_ERROR("Renderer", "RenderDevice: OpenGL requires a native window handle");
            return false;
        }

        HWND hwnd = static_cast<HWND>(nativeWindowHandle);
        HDC hdc = GetDC(hwnd);
        if (!hdc) {
            NF_LOG_ERROR("Renderer", "RenderDevice: GetDC failed");
            return false;
        }

        PIXELFORMATDESCRIPTOR pfd{};
        pfd.nSize        = sizeof(pfd);
        pfd.nVersion     = 1;
        pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType   = PFD_TYPE_RGBA;
        pfd.cColorBits   = 32;
        pfd.cDepthBits   = 24;
        pfd.cStencilBits = 8;
        pfd.iLayerType   = PFD_MAIN_PLANE;

        int pixelFormat = ChoosePixelFormat(hdc, &pfd);
        if (pixelFormat == 0) {
            NF_LOG_ERROR("Renderer", "RenderDevice: ChoosePixelFormat failed");
            ReleaseDC(hwnd, hdc);
            return false;
        }
        if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
            NF_LOG_ERROR("Renderer", "RenderDevice: SetPixelFormat failed");
            ReleaseDC(hwnd, hdc);
            return false;
        }

        HGLRC hglrc = wglCreateContext(hdc);
        if (!hglrc) {
            NF_LOG_ERROR("Renderer", "RenderDevice: wglCreateContext failed");
            ReleaseDC(hwnd, hdc);
            return false;
        }
        if (!wglMakeCurrent(hdc, hglrc)) {
            NF_LOG_ERROR("Renderer", "RenderDevice: wglMakeCurrent failed");
            wglDeleteContext(hglrc);
            ReleaseDC(hwnd, hdc);
            return false;
        }

        m_DeviceContext  = hdc;
        m_RenderContext  = hglrc;

        NF_LOG_INFO("Renderer", "RenderDevice: WGL context created");

        if (!gladLoaderLoadGL()) {
            NF_LOG_ERROR("Renderer", "RenderDevice: gladLoaderLoadGL failed");
            return false;
        }

        NF_LOG_INFO("Renderer", "RenderDevice: OpenGL function pointers loaded via GLAD");
        NF_LOG_INFO("Renderer",
            std::string("RenderDevice: GL_RENDERER = ")
            + reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        NF_LOG_INFO("Renderer",
            std::string("RenderDevice: GL_VERSION  = ")
            + reinterpret_cast<const char*>(glGetString(GL_VERSION)));

        glEnable(GL_DEPTH_TEST);
    }
#elif defined(NF_HAS_OPENGL)
    if (api == GraphicsAPI::OpenGL) {
        // Non-Windows OpenGL path: assume context is already current (e.g. via GLFW).
        if (!gladLoaderLoadGL()) {
            NF_LOG_ERROR("Renderer", "RenderDevice: gladLoaderLoadGL failed -- ensure an OpenGL context is current");
            return false;
        }
        NF_LOG_INFO("Renderer", "RenderDevice: OpenGL function pointers loaded via GLAD");
    }
#else
    if (api == GraphicsAPI::OpenGL) {
        NF_LOG_WARN("Renderer", "RenderDevice: OpenGL requested but NF_HAS_OPENGL not defined; falling back to Null");
        m_API = GraphicsAPI::Null;
    }
#endif

    m_Initialised = true;
    NF_LOG_INFO("Renderer", "RenderDevice initialised");
    return true;
}

void RenderDevice::Shutdown() {
#if defined(NF_HAS_OPENGL) && defined(_WIN32)
    if (m_RenderContext) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(static_cast<HGLRC>(m_RenderContext));
        m_RenderContext = nullptr;
    }
    if (m_DeviceContext && m_NativeWindowHandle) {
        ReleaseDC(static_cast<HWND>(m_NativeWindowHandle),
                  static_cast<HDC>(m_DeviceContext));
        m_DeviceContext = nullptr;
    }
#endif
    m_Initialised = false;
    NF_LOG_INFO("Renderer", "RenderDevice shut down");
}

void RenderDevice::BeginFrame() {
#ifdef NF_HAS_OPENGL
    if (m_API == GraphicsAPI::OpenGL) {
    }
#endif
}

void RenderDevice::EndFrame() {
#if defined(NF_HAS_OPENGL) && defined(_WIN32)
    if (m_API == GraphicsAPI::OpenGL && m_DeviceContext) {
        SwapBuffers(static_cast<HDC>(m_DeviceContext));
    }
#endif
}

void RenderDevice::Resize(int w, int h) noexcept {
    if (w <= 0 || h <= 0) {
        return;
    }
    m_ViewportX = 0;
    m_ViewportY = 0;
    m_ViewportW = w;
    m_ViewportH = h;
#ifdef NF_HAS_OPENGL
    if (m_API == GraphicsAPI::OpenGL) {
        glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
    }
#else
    (void)w; (void)h;
#endif
}

void RenderDevice::SetViewport(int x, int y, int w, int h) noexcept {
    if (w <= 0 || h <= 0) {
        return;
    }
    m_ViewportX = x;
    m_ViewportY = y;
    m_ViewportW = w;
    m_ViewportH = h;
#ifdef NF_HAS_OPENGL
    if (m_API == GraphicsAPI::OpenGL) {
        glViewport(static_cast<GLint>(x), static_cast<GLint>(y),
                   static_cast<GLsizei>(w), static_cast<GLsizei>(h));
    }
#else
    (void)x; (void)y; (void)w; (void)h;
#endif
}

void RenderDevice::EnableScissor(bool enabled) noexcept {
#ifdef NF_HAS_OPENGL
    if (m_API == GraphicsAPI::OpenGL) {
        if (enabled) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
    }
#else
    (void)enabled;
#endif
}

void RenderDevice::SetScissorRect(int x, int y, int w, int h) noexcept {
    if (w <= 0 || h <= 0) {
        return;
    }
#ifdef NF_HAS_OPENGL
    if (m_API == GraphicsAPI::OpenGL) {
        glScissor(static_cast<GLint>(x), static_cast<GLint>(y),
                  static_cast<GLsizei>(w), static_cast<GLsizei>(h));
    }
#else
    (void)x; (void)y; (void)w; (void)h;
#endif
}

void RenderDevice::Clear(float r, float g, float b, float a) {
#ifdef NF_HAS_OPENGL
    if (m_API == GraphicsAPI::OpenGL) {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }
#endif
    // Null API: suppress unused-parameter warnings.
    (void)r; (void)g; (void)b; (void)a;
}

} // namespace NF
