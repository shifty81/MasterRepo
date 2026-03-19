#pragma once
#include <string>
#include <cstdint>

namespace Engine::Render {

enum class RendererBackend { OpenGL, Vulkan, DirectX12 };

struct RenderConfig {
    RendererBackend backend   = RendererBackend::OpenGL;
    bool            wireframe = false;
};

struct DrawCall {
    uint32_t    id         = 0;
    std::string meshId;
    std::string materialId;
};

class Renderer {
public:
    explicit Renderer(const RenderConfig& cfg);
    ~Renderer();

    bool Init(int width, int height);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void Submit(const DrawCall& dc);
    void SetClearColor(float r, float g, float b, float a = 1.0f);
    void Clear();

    int  GetDrawCallCount() const;
    void SetViewport(int x, int y, int w, int h);

    RendererBackend GetBackend() const { return m_config.backend; }

private:
    RenderConfig m_config;
    bool         m_initialized  = false;
    int          m_drawCallCount = 0;
};

} // namespace Engine::Render
