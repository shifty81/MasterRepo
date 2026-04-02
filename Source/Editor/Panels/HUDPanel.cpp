#include "Editor/Panels/HUDPanel.h"
#include "UI/Rendering/UIRenderer.h"
#include "Game/Interaction/ResourceItem.h"
#include <string>
#include <algorithm>

namespace NF::Editor {

void HUDPanel::Update([[maybe_unused]] float dt) {}

void HUDPanel::DrawBar(float x, float y, float w, float barH,
                        float fraction, uint32_t fillColor,
                        const char* label) const
{
    if (!m_Renderer) return;

    constexpr uint32_t kBgColor   = 0x2A2A2AFF;
    constexpr uint32_t kTextColor = 0xCCCCCCFF;
    const float dpi   = m_Renderer->GetDpiScale();
    const float scale = 2.f;

    // Background track
    m_Renderer->DrawRect({x, y, w, barH}, kBgColor);

    // Fill bar (clamped to [0,1])
    const float f = std::max(0.f, std::min(1.f, fraction));
    if (f > 0.f)
        m_Renderer->DrawRect({x, y, w * f, barH}, fillColor);

    // Label centered in bar
    if (label)
        m_Renderer->DrawText(label, x + 4.f * dpi, y + 2.f * dpi, kTextColor, scale);
}

void HUDPanel::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;

    constexpr uint32_t kTextColor  = 0xB0B0B0FF;
    constexpr uint32_t kLabelColor = 0x808080FF;
    constexpr uint32_t kSepColor   = 0x444444FF;
    constexpr uint32_t kHealthCol  = 0x3BA83BFF; // green
    constexpr uint32_t kEnergyCol  = 0x3B7AB8FF; // blue
    constexpr uint32_t kItemCol    = 0xA87A3BFF; // amber

    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 18.f * dpi;
    const float barH  = 14.f * dpi;
    const float padX  = 6.f  * dpi;
    const float scale = 2.f;
    float cy = y + 4.f * dpi;

    // ---- Title --------------------------------------------------------------
    m_Renderer->DrawText("R.I.G. Status", x + padX, cy + 2.f * dpi, kTextColor, scale);
    cy += lineH;
    m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
    cy += 4.f * dpi;

    if (!m_Loop) {
        m_Renderer->DrawText("No interaction loop", x + padX, cy + 2.f * dpi, kLabelColor, scale);
        return;
    }

    const NF::Game::RigState& rig = m_Loop->GetRig();

    // ---- Rig name -----------------------------------------------------------
    const std::string nameStr = "Rig: " + rig.GetName();
    m_Renderer->DrawText(nameStr, x + padX, cy + 2.f * dpi, kLabelColor, scale);
    cy += lineH;

    // ---- Health bar ---------------------------------------------------------
    {
        const float fraction = rig.GetHealth() / NF::Game::RigState::kMaxHealth;
        const std::string label = "HP " + std::to_string(static_cast<int>(rig.GetHealth()))
                                + " / " + std::to_string(static_cast<int>(NF::Game::RigState::kMaxHealth));
        DrawBar(x + padX, cy, w - padX * 2.f, barH,
                fraction, kHealthCol, label.c_str());
        cy += barH + 3.f * dpi;
    }

    // ---- Energy bar ---------------------------------------------------------
    {
        const float fraction = rig.GetEnergy() / NF::Game::RigState::kMaxEnergy;
        const std::string label = "EN " + std::to_string(static_cast<int>(rig.GetEnergy()))
                                + " / " + std::to_string(static_cast<int>(NF::Game::RigState::kMaxEnergy));
        DrawBar(x + padX, cy, w - padX * 2.f, barH,
                fraction, kEnergyCol, label.c_str());
        cy += barH + 3.f * dpi;
    }

    // ---- Tool slot ----------------------------------------------------------
    {
        static const char* kToolNames[] = {"Mine", "Place", "Repair"};
        const int slot = rig.GetToolSlot();
        const char* toolName = (slot >= 0 && slot < 3) ? kToolNames[slot] : "?";
        const std::string toolStr = std::string("Tool: ") + toolName;
        m_Renderer->DrawText(toolStr, x + padX, cy + 2.f * dpi, kTextColor, scale);
        cy += lineH;
    }

    m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
    cy += 4.f * dpi;

    // ---- Inventory ----------------------------------------------------------
    m_Renderer->DrawText("Inventory", x + padX, cy + 2.f * dpi, kTextColor, scale);
    cy += lineH;

    const NF::Game::Inventory& inv = m_Loop->GetInventory();
    bool anyItem = false;
    for (int i = 0; i < NF::Game::Inventory::kMaxSlots; ++i) {
        if (cy + lineH > y + h) break;

        const NF::Game::ResourceStack& slot = inv.GetSlot(i);
        if (slot.IsEmpty()) continue;

        anyItem = true;
        const std::string line =
            std::string(NF::Game::ResourceTypeName(slot.type))
            + " x" + std::to_string(slot.count);
        m_Renderer->DrawText(line, x + padX, cy + 2.f * dpi, kItemCol, scale);
        cy += lineH;
    }

    if (!anyItem) {
        m_Renderer->DrawText("(empty)", x + padX, cy + 2.f * dpi, kLabelColor, scale);
    }
}

} // namespace NF::Editor
