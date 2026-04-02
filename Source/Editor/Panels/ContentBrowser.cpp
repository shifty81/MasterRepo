#include "Editor/Panels/ContentBrowser.h"
#include "UI/Rendering/UIRenderer.h"

namespace NF::Editor {

void ContentBrowser::Update([[maybe_unused]] float dt) {}

void ContentBrowser::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;
    (void)w; (void)h;

    static constexpr uint32_t kTextColor  = 0xB0B0B0FF;
    static constexpr uint32_t kLabelColor = 0x808080FF;
    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 18.f * dpi;
    const float padX  = 6.f * dpi;
    const float scale = 2.f;

    std::string rootLabel = "Root: " + (m_RootPath.empty() ? "(none)" : m_RootPath);
    m_Renderer->DrawText(rootLabel,      x + padX, y + 4.f * dpi,             kLabelColor, scale);
    m_Renderer->DrawText("Assets/",      x + padX, y + 4.f * dpi +     lineH, kTextColor,  scale);
    m_Renderer->DrawText("  Definitions/", x + padX, y + 4.f * dpi + 2*lineH, kTextColor,  scale);
    m_Renderer->DrawText("  Textures/",  x + padX, y + 4.f * dpi + 3*lineH,   kTextColor,  scale);
    m_Renderer->DrawText("  Models/",    x + padX, y + 4.f * dpi + 4*lineH,   kTextColor,  scale);
}

} // namespace NF::Editor
