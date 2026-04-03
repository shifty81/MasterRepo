#include "Editor/Viewport/EditorViewportNF.h"
#include "Editor/Application/EditorAppIntegration.h"
#include "Editor/Selection/SelectionService.h"

namespace nf
{
    EditorViewport::EditorViewport(EditorApp* app)
        : m_app(app)
    {
    }

    void EditorViewport::SetEditorApp(EditorApp* app)
    {
        m_app = app;
    }

    void EditorViewport::SimulateSelectWorldObject(std::uint64_t objectId, const std::string& label)
    {
        if (m_app != nullptr)
        {
            m_app->OnViewportWorldObjectSelected(objectId, label);
        }
    }

    void EditorViewport::SimulateSelectChunk(std::uint64_t chunkId, const std::string& label)
    {
        if (m_app != nullptr)
        {
            m_app->OnViewportChunkSelected(chunkId, label);
        }
    }

    void EditorViewport::SimulateSelectVoxel(std::uint64_t voxelId, const std::string& label)
    {
        if (m_app != nullptr)
        {
            m_app->OnViewportVoxelSelected(voxelId, label);
        }
    }

    void EditorViewport::SyncSelectionHighlight()
    {
        m_highlightState = {};

        if (m_app == nullptr)
        {
            return;
        }

        const auto& selection = m_app->GetSelectionService().GetSelection();
        m_highlightState.highlightLabel = selection.label;

        switch (selection.kind)
        {
        case SelectionKind::WorldObject:
            m_highlightState.selectedWorldObjectId = selection.id;
            break;
        case SelectionKind::Chunk:
            m_highlightState.selectedChunkId = selection.id;
            break;
        case SelectionKind::Voxel:
            m_highlightState.selectedVoxelId = selection.id;
            break;
        default:
            break;
        }
    }

    const ViewportHighlightState& EditorViewport::GetHighlightState() const noexcept
    {
        return m_highlightState;
    }
}
