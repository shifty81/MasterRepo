#include "Editor/Panels/ContentBrowser.h"
#include "UI/Rendering/UIRenderer.h"

namespace NF::Editor {

void ContentBrowser::Update([[maybe_unused]] float dt) {}

void ContentBrowser::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;
    (void)w; (void)h;

    static constexpr uint32_t kTextColor  = 0xB0B0B0FF;
    static constexpr uint32_t kLabelColor = 0x808080FF;
    const float scale = 2.f;

    std::string rootLabel = "Root: " + (m_RootPath.empty() ? "(none)" : m_RootPath);
    m_Renderer->DrawText(rootLabel, x + 6.f, y + 4.f, kLabelColor, scale);

    m_Renderer->DrawText("Assets/", x + 6.f, y + 24.f, kTextColor, scale);
    m_Renderer->DrawText("  Definitions/", x + 6.f, y + 42.f, kTextColor, scale);
    m_Renderer->DrawText("  Textures/", x + 6.f, y + 60.f, kTextColor, scale);
    m_Renderer->DrawText("  Models/", x + 6.f, y + 78.f, kTextColor, scale);
}

} // namespace NF::Editor
