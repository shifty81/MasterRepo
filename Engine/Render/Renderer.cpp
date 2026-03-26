#include "Engine/Render/Renderer.h"
#include "Engine/Core/Logger.h"
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif
#include <GL/gl.h>

namespace Engine::Render {

Renderer::Renderer(const RenderConfig& cfg) : m_config(cfg) {}

Renderer::~Renderer() {
    if (m_initialized)
        Shutdown();
}

bool Renderer::Init(int width, int height) {
    m_width  = width;
    m_height = height;
    Engine::Core::Logger::Info("Renderer::Init (" + std::to_string(width) + "x" + std::to_string(height) + ")");
    m_initialized   = true;
    m_drawCallCount = 0;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    return true;
}

void Renderer::Shutdown() {
    if (!m_initialized)
        return;
    Engine::Core::Logger::Info("Renderer::Shutdown");
    m_initialized = false;
}

void Renderer::BeginFrame() {
    m_drawCallCount = 0;
    glViewport(0, 0, m_width, m_height);
}

void Renderer::EndFrame() {
    glFlush();
}

void Renderer::Submit(const DrawCall& dc) {
    (void)dc;
    ++m_drawCallCount;
}

void Renderer::SetClearColor(float r, float g, float b, float a) {
    m_clearR = r; m_clearG = g; m_clearB = b; m_clearA = a;
}

void Renderer::Clear() {
    glClearColor(m_clearR, m_clearG, m_clearB, m_clearA);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int Renderer::GetDrawCallCount() const {
    return m_drawCallCount;
}

void Renderer::SetViewport(int x, int y, int w, int h) {
    m_width  = w;
    m_height = h;
    glViewport(x, y, w, h);
}

} // namespace Engine::Render
