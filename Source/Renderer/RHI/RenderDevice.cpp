#include "Renderer/RHI/RenderDevice.h"
#include "Core/Logging/Log.h"

#ifdef NF_HAS_OPENGL
#include <GL/gl.h>
#endif

namespace NF {

bool RenderDevice::Init(GraphicsAPI api) {
    m_API = api;
    m_Initialised = true;
    NF_LOG_INFO("Renderer", "RenderDevice initialised");
    return true;
}

void RenderDevice::Shutdown() {
    m_Initialised = false;
    NF_LOG_INFO("Renderer", "RenderDevice shut down");
}

void RenderDevice::BeginFrame() {
#ifdef NF_HAS_OPENGL
    if (m_API == GraphicsAPI::OpenGL) {
        // Platform-specific begin-frame work (e.g. pipeline state reset).
    }
#endif
}

void RenderDevice::EndFrame() {
#ifdef NF_HAS_OPENGL
    if (m_API == GraphicsAPI::OpenGL) {
        // Platform-specific end-frame / buffer swap triggered by the windowing layer.
    }
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
