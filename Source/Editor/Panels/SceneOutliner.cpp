#include "Editor/Panels/SceneOutliner.h"
#include "UI/Rendering/UIRenderer.h"

namespace NF::Editor {

void SceneOutliner::Update([[maybe_unused]] float dt) {}

void SceneOutliner::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;

    static constexpr uint32_t kTextColor   = 0xB0B0B0FF;
    static constexpr uint32_t kLabelColor  = 0x808080FF;
    const float lineH = 18.f;
    const float scale = 2.f;
    float cy = y + 4.f;

    if (!m_World) {
        m_Renderer->DrawText("No world loaded", x + 6.f, cy, kLabelColor, scale);
        return;
    }

    // List entities from the ECS world
    const auto& entities = m_World->GetLiveEntities();
    if (entities.empty()) {
        m_Renderer->DrawText("(empty world)", x + 6.f, cy, kLabelColor, scale);
        return;
    }

    for (EntityId e : entities) {
        if (cy + lineH > y + h) break; // clip to panel
        std::string label = "Entity " + std::to_string(e);
        m_Renderer->DrawText(label, x + 6.f, cy, kTextColor, scale);
        cy += lineH;
    }
}

} // namespace NF::Editor
