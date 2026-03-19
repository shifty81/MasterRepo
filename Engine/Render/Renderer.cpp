#include "Engine/Render/Renderer.h"
#include "Engine/Core/Logger.h"

// Stub implementation — real backend will link OpenGL/Vulkan/D3D12.

namespace Engine::Render {

Renderer::Renderer(const RenderConfig& cfg) : m_config(cfg) {}

Renderer::~Renderer() {
    if (m_initialized)
        Shutdown();
}

bool Renderer::Init(int width, int height) {
    (void)width; (void)height;
    Engine::Core::Logger::Info("Renderer::Init — stub (OpenGL not linked)");
    m_initialized   = true;
    m_drawCallCount = 0;
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
}

void Renderer::EndFrame() {
    // No-op until a real graphics backend is linked.
}

void Renderer::Submit(const DrawCall& dc) {
    (void)dc;
    ++m_drawCallCount;
}

void Renderer::SetClearColor(float r, float g, float b, float a) {
    (void)r; (void)g; (void)b; (void)a;
}

void Renderer::Clear() {
    // No-op until a real graphics backend is linked.
}

int Renderer::GetDrawCallCount() const {
    return m_drawCallCount;
}

void Renderer::SetViewport(int x, int y, int w, int h) {
    (void)x; (void)y; (void)w; (void)h;
}

} // namespace Engine::Render
