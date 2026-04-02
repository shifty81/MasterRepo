#include "Editor/Panels/VoxelInspector.h"
#include "UI/Rendering/UIRenderer.h"
#include "Game/Voxel/ChunkCoord.h"
#include <string>
#include <vector>

namespace NF::Editor {

void VoxelInspector::Update([[maybe_unused]] float dt) {}

void VoxelInspector::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;

    static constexpr uint32_t kTextColor    = 0xB0B0B0FF;
    static constexpr uint32_t kLabelColor   = 0x808080FF;
    static constexpr uint32_t kHoverColor   = 0x3F3F50FF;
    static constexpr uint32_t kSelectColor  = 0x264F78FF;
    static constexpr uint32_t kToggleOnCol  = 0x3B8A3BFF; // green = on
    static constexpr uint32_t kToggleOffCol = 0x8A3B3BFF; // red   = off
    static constexpr uint32_t kSepColor     = 0x444444FF;

    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 18.f * dpi;
    const float padX  = 6.f  * dpi;
    const float scale = 2.f;
    float cy = y + 4.f * dpi;

    // ---- Toggle row ---------------------------------------------------------
    {
        const bool hovered = m_Input &&
            m_Input->mouseX >= x        && m_Input->mouseX < x + w &&
            m_Input->mouseY >= cy       && m_Input->mouseY < cy + lineH;

        if (hovered) m_Renderer->DrawRect({x, cy, w, lineH}, kHoverColor);
        if (hovered && m_Input->leftJustPressed) m_OverlayVisible = !m_OverlayVisible;

        const uint32_t toggleCol = m_OverlayVisible ? kToggleOnCol : kToggleOffCol;
        const std::string toggleLabel =
            std::string("Voxel Overlay: ") + (m_OverlayVisible ? "ON" : "OFF");
        m_Renderer->DrawText(toggleLabel, x + padX, cy + 2.f * dpi, toggleCol, scale);
        cy += lineH;
    }

    // Separator
    m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
    cy += 4.f * dpi;

    // ---- Stats row ----------------------------------------------------------
    if (m_ChunkMap) {
        const std::string statsStr =
            NF::Game::VoxelDebugOverlay::BuildStatsString(*m_ChunkMap);
        m_Renderer->DrawText(statsStr, x + padX, cy + 2.f * dpi, kLabelColor, scale);
        cy += lineH;
    } else {
        m_Renderer->DrawText("No chunk map", x + padX, cy + 2.f * dpi, kLabelColor, scale);
        cy += lineH;
    }

    // Separator
    m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
    cy += 4.f * dpi;

    // ---- Chunk list ---------------------------------------------------------
    if (!m_ChunkMap) return;

    const std::vector<NF::Game::ChunkCoord> coords = m_ChunkMap->GetLoadedCoords();
    if (coords.empty()) {
        m_Renderer->DrawText("No chunks loaded", x + padX, cy + 2.f * dpi, kLabelColor, scale);
        return;
    }

    m_Renderer->DrawText("Loaded Chunks:", x + padX, cy + 2.f * dpi, kLabelColor, scale);
    cy += lineH;

    for (int i = 0; i < static_cast<int>(coords.size()); ++i) {
        if (cy + lineH > y + h) break;

        const bool hovered = m_Input &&
            m_Input->mouseX >= x        && m_Input->mouseX < x + w &&
            m_Input->mouseY >= cy       && m_Input->mouseY < cy + lineH;
        const bool selected = (i == m_SelectedChunkIdx);

        if (selected) {
            m_Renderer->DrawRect({x, cy, w, lineH}, kSelectColor);
        } else if (hovered) {
            m_Renderer->DrawRect({x, cy, w, lineH}, kHoverColor);
        }

        if (hovered && m_Input->leftJustPressed)
            m_SelectedChunkIdx = i;

        const auto& cc = coords[static_cast<size_t>(i)];
        std::string label = "(" + std::to_string(cc.X) + ","
                          + std::to_string(cc.Y) + ","
                          + std::to_string(cc.Z) + ")";

        const NF::Game::Chunk* chunk = m_ChunkMap->GetChunk(cc);
        if (chunk) {
            label += "  " + std::to_string(chunk->CountSolid()) + " solid";
            if (chunk->IsDirty()) label += "*";
        }

        const uint32_t rowColor = selected ? 0xFFFFFFFF : kTextColor;
        m_Renderer->DrawText(label, x + padX, cy + 2.f * dpi, rowColor, scale);
        cy += lineH;
    }

    // ---- Selected chunk breakdown -------------------------------------------
    if (m_SelectedChunkIdx >= 0 &&
        m_SelectedChunkIdx < static_cast<int>(coords.size()))
    {
        const auto& cc    = coords[static_cast<size_t>(m_SelectedChunkIdx)];
        const NF::Game::Chunk* chunk = m_ChunkMap->GetChunk(cc);
        if (!chunk) return;

        cy += 2.f * dpi;
        m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
        cy += 4.f * dpi;

        // Count each voxel type.
        static const NF::Game::VoxelType kTypesToShow[] = {
            NF::Game::VoxelType::Stone,
            NF::Game::VoxelType::Ore,
            NF::Game::VoxelType::Dirt,
            NF::Game::VoxelType::Rock,
            NF::Game::VoxelType::Metal,
            NF::Game::VoxelType::Ice,
            NF::Game::VoxelType::Organic,
        };

        const auto& raw = chunk->GetRawData();
        for (auto type : kTypesToShow) {
            if (cy + lineH > y + h) break;

            const NF::Game::VoxelId id = static_cast<NF::Game::VoxelId>(type);
            int32_t count = 0;
            for (auto v : raw) if (v == id) ++count;
            if (count == 0) continue;

            const std::string line =
                std::string(NF::Game::GetVoxelTypeInfo(id).name)
                + ": " + std::to_string(count);
            m_Renderer->DrawText(line, x + padX, cy + 2.f * dpi, kTextColor, scale);
            cy += lineH;
        }
    }
}

} // namespace NF::Editor
