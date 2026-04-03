#include "Editor/Panels/HUDPanel.h"
#include "UI/Rendering/UIRenderer.h"
#include "Game/Interaction/ResourceItem.h"
#include "Game/World/GameWorld.h"
#include <algorithm>
#include <string>

namespace NF::Editor {

void HUDPanel::Update([[maybe_unused]] float dt) {}

void HUDPanel::DrawBar(float x, float y, float w, float barH,
                        float fraction, uint32_t fillColor,
                        const char* label) const
{
    if (!m_Renderer) return;

    constexpr uint32_t kBgColor   = 0x1F2329FF;
    constexpr uint32_t kTextColor = 0xDDE3EAFF;
    constexpr uint32_t kGlowColor = 0xFFFFFF10;
    const float dpi   = m_Renderer->GetDpiScale();
    const float scale = 2.f;

    m_Renderer->DrawRect({x, y, w, barH}, kBgColor);
    m_Renderer->DrawRect({x, y, w, 1.f}, kGlowColor);

    const float f = std::max(0.f, std::min(1.f, fraction));
    if (f > 0.f) {
        m_Renderer->DrawRect({x, y, w * f, barH}, fillColor);
        m_Renderer->DrawRect({x, y, w * f, 1.f}, 0xFFFFFFFF);
    }

    if (label)
        m_Renderer->DrawText(label, x + 4.f * dpi, y + 2.f * dpi, kTextColor, scale);
}

void HUDPanel::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;

    constexpr uint32_t kTextColor   = 0xD5DCE4FF;
    constexpr uint32_t kLabelColor  = 0x93A1B2FF;
    constexpr uint32_t kSepColor    = 0x3F4A57FF;
    constexpr uint32_t kPanelGlow   = 0xFFFFFF08;
    constexpr uint32_t kHealthCol   = 0x4FCB74FF;
    constexpr uint32_t kEnergyCol   = 0x4FA3FFFF;
    constexpr uint32_t kItemCol     = 0xE2B36AFF;
    constexpr uint32_t kReadyCol    = 0x58C27DFF;
    constexpr uint32_t kWarnCol     = 0xE0A84EFF;

    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 18.f * dpi;
    const float barH  = 14.f * dpi;
    const float padX  = 6.f  * dpi;
    const float scale = 2.f;
    float cy = y + 4.f * dpi;

    m_Renderer->DrawRect({x, y, w, 1.f}, kPanelGlow);
    m_Renderer->DrawText("R.I.G. Status", x + padX, cy + 2.f * dpi, kTextColor, scale);
    cy += lineH;
    m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
    cy += 4.f * dpi;

    if (m_GameWorld) {
        const bool ready = m_GameWorld->HasVisibleWorld();
        const std::string worldLine = std::string("World: ")
            + (ready ? "ready" : "not ready")
            + "  chunks " + std::to_string(m_GameWorld->GetLoadedChunkCount());
        m_Renderer->DrawText(worldLine, x + padX, cy + 2.f * dpi, ready ? kReadyCol : kWarnCol, scale);
        cy += lineH;

        const std::string bootLine = std::string("Boot: ") + m_GameWorld->GetBootstrapStatusText();
        m_Renderer->DrawText(bootLine, x + padX, cy + 2.f * dpi, kLabelColor, 1.5f);
        cy += lineH;

        m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
        cy += 4.f * dpi;
    }

    if (!m_Loop) {
        m_Renderer->DrawText("No interaction loop", x + padX, cy + 2.f * dpi, kLabelColor, scale);
        return;
    }

    const NF::Game::RigState& rig = m_Loop->GetRig();

    const std::string nameStr = "Rig: " + rig.GetName();
    m_Renderer->DrawText(nameStr, x + padX, cy + 2.f * dpi, kLabelColor, scale);
    cy += lineH;

    {
        const float fraction = rig.GetHealth() / NF::Game::RigState::kMaxHealth;
        const std::string label = "HP " + std::to_string(static_cast<int>(rig.GetHealth()))
                                + " / " + std::to_string(static_cast<int>(NF::Game::RigState::kMaxHealth));
        DrawBar(x + padX, cy, w - padX * 2.f, barH,
                fraction, kHealthCol, label.c_str());
        cy += barH + 3.f * dpi;
    }

    {
        const float fraction = rig.GetEnergy() / NF::Game::RigState::kMaxEnergy;
        const std::string label = "EN " + std::to_string(static_cast<int>(rig.GetEnergy()))
                                + " / " + std::to_string(static_cast<int>(NF::Game::RigState::kMaxEnergy));
        DrawBar(x + padX, cy, w - padX * 2.f, barH,
                fraction, kEnergyCol, label.c_str());
        cy += barH + 3.f * dpi;
    }

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
