#include "Editor/Application/EditorAppIntegration.h"
#include "Editor/Inspector/InspectorPanel.h"
#include "Editor/Viewport/EditorViewportNF.h"

#include <utility>

namespace nf
{
    namespace
    {
        PropertyWidgetHint WidgetHintForType(PropertyType type, bool readOnly)
        {
            if (readOnly)
            {
                return PropertyWidgetHint::ReadOnlyLabel;
            }

            switch (type)
            {
            case PropertyType::Bool: return PropertyWidgetHint::Checkbox;
            case PropertyType::Int:
            case PropertyType::Float: return PropertyWidgetHint::NumericField;
            case PropertyType::String: return PropertyWidgetHint::TextField;
            case PropertyType::Vec3: return PropertyWidgetHint::Vec3Field;
            default: return PropertyWidgetHint::Default;
            }
        }
    }

    void EditorApp::SetContentBrowser(ContentBrowser* contentBrowser)
    {
        m_contentBrowser = contentBrowser;
    }

    void EditorApp::SetInspector(Inspector* inspector)
    {
        m_inspector = inspector;
    }

    void EditorApp::SetViewport(EditorViewport* viewport)
    {
        m_viewport = viewport;
    }

    SelectionService& EditorApp::GetSelectionService() noexcept
    {
        return m_selectionService;
    }

    const SelectionService& EditorApp::GetSelectionService() const noexcept
    {
        return m_selectionService;
    }

    PropertyInspectorSystem& EditorApp::GetPropertyInspectorSystem() noexcept
    {
        return m_propertyInspectorSystem;
    }

    const PropertyInspectorSystem& EditorApp::GetPropertyInspectorSystem() const noexcept
    {
        return m_propertyInspectorSystem;
    }

    StatusBarPanel& EditorApp::GetStatusBar() noexcept
    {
        return m_statusBar;
    }

    const StatusBarPanel& EditorApp::GetStatusBar() const noexcept
    {
        return m_statusBar;
    }

    WorldOutlinerPanel& EditorApp::GetWorldOutliner() noexcept
    {
        return m_worldOutliner;
    }

    const WorldOutlinerPanel& EditorApp::GetWorldOutliner() const noexcept
    {
        return m_worldOutliner;
    }

    EditorToolContext& EditorApp::GetToolContext() noexcept
    {
        return m_toolContext;
    }

    const EditorToolContext& EditorApp::GetToolContext() const noexcept
    {
        return m_toolContext;
    }

    void EditorApp::ApplySelection(const SelectionHandle& handle)
    {
        m_selectionService.Select(handle);
        PushSelectionToInspector();
        PushSelectionToStatusBar();
        SyncSelectionToViewportAndOutliner();
    }

    void EditorApp::ClearSelection()
    {
        m_selectionService.Clear();
        PushSelectionToInspector();
        PushSelectionToStatusBar();
        SyncSelectionToViewportAndOutliner();
    }

    void EditorApp::RebuildInspectorFromSelection()
    {
        PushSelectionToInspector();
    }

    void EditorApp::RebuildStatusBar(
        const std::string& activeWorld,
        const std::string& worldReadiness,
        int loadedChunkCount)
    {
        m_activeWorld = activeWorld;
        m_worldReadiness = worldReadiness;
        m_loadedChunkCount = loadedChunkCount;
        PushSelectionToStatusBar();
    }

    void EditorApp::RebuildWorldOutlinerFromChunks(
        const std::string& worldName,
        const std::vector<RuntimeChunkMetadata>& chunks)
    {
        m_activeWorld = worldName;
        m_loadedChunkCount = static_cast<int>(chunks.size());
        m_worldOutliner.SetRootNodes(
            WorldOutlinerPanel::BuildDevWorldTreeFromChunkMetadata(worldName, chunks));
        SyncSelectionToViewportAndOutliner();
    }

    void EditorApp::OnContentBrowserAssetSelected(
        std::uint64_t assetId,
        const std::string& assetName,
        const std::string& assetPath)
    {
        SelectionHandle handle {};
        handle.kind = SelectionKind::Asset;
        handle.id = assetId;
        handle.label = assetName.empty() ? assetPath : assetName;
        ApplySelection(handle);

        PropertySet propertySet {};
        propertySet.title = "Asset";
        propertySet.entries.push_back({"Name", PropertyType::String, PropertyWidgetHint::ReadOnlyLabel, assetName, {}, true, false});
        propertySet.entries.push_back({"Path", PropertyType::String, PropertyWidgetHint::ReadOnlyLabel, assetPath, {}, true, false});
        propertySet.entries.push_back({"AssetId", PropertyType::Int, PropertyWidgetHint::ReadOnlyLabel, static_cast<int>(assetId), {}, true, false});
        m_propertyInspectorSystem.SetPropertySet(std::move(propertySet));
    }

    void EditorApp::OnViewportWorldObjectSelected(
        std::uint64_t objectId,
        const std::string& objectLabel)
    {
        SelectionHandle handle {};
        handle.kind = SelectionKind::WorldObject;
        handle.id = objectId;
        handle.label = objectLabel;
        ApplySelection(handle);
    }

    void EditorApp::OnViewportChunkSelected(
        std::uint64_t chunkId,
        const std::string& chunkLabel)
    {
        SelectionHandle handle {};
        handle.kind = SelectionKind::Chunk;
        handle.id = chunkId;
        handle.label = chunkLabel;
        ApplySelection(handle);
    }

    void EditorApp::OnViewportVoxelSelected(
        std::uint64_t voxelId,
        const std::string& voxelLabel)
    {
        SelectionHandle handle {};
        handle.kind = SelectionKind::Voxel;
        handle.id = voxelId;
        handle.label = voxelLabel;
        ApplySelection(handle);
    }

    void EditorApp::OnWorldOutlinerNodeSelected(
        std::uint64_t nodeId,
        const std::string& nodeLabel)
    {
        SelectionHandle handle {};
        handle.kind = SelectionKind::WorldObject;
        handle.id = nodeId;
        handle.label = nodeLabel;
        ApplySelection(handle);
    }

    void EditorApp::SyncSelectionToViewportAndOutliner()
    {
        if (m_viewport != nullptr)
        {
            m_viewport->SyncSelectionHighlight();
        }

        if (m_selectionService.HasSelection())
        {
            m_worldOutliner.SyncSelection(m_selectionService.GetSelection().id);
        }
        else
        {
            m_worldOutliner.SyncSelection(0);
        }

        m_selectionService.ClearSelectionChanged();
    }

    void EditorApp::MarkInspectorEditsDirty()
    {
        if (m_propertyInspectorSystem.IsDirty())
        {
            m_toolContext.worldDirty = true;
            PushSelectionToStatusBar();
        }
    }

    void EditorApp::PushSelectionToInspector()
    {
        if (!m_selectionService.HasSelection())
        {
            PropertySet propertySet {};
            propertySet.title = "Inspector";
            propertySet.entries.push_back({
                "State",
                PropertyType::String,
                PropertyWidgetHint::ReadOnlyLabel,
                std::string("No Selection"),
                {},
                true,
                false
            });
            m_propertyInspectorSystem.SetPropertySet(std::move(propertySet));

            if (m_inspector != nullptr)
            {
                m_inspector->SetPropertyInspectorSystem(&m_propertyInspectorSystem);
            }
            return;
        }

        const auto& selection = m_selectionService.GetSelection();

        PropertySet propertySet {};
        propertySet.title = "Selection";

        propertySet.entries.push_back({
            "Label",
            PropertyType::String,
            WidgetHintForType(PropertyType::String, false),
            selection.label,
            {},
            false,
            false
        });

        propertySet.entries.push_back({
            "Id",
            PropertyType::Int,
            WidgetHintForType(PropertyType::Int, true),
            static_cast<int>(selection.id),
            {},
            true,
            false
        });

        switch (selection.kind)
        {
        case SelectionKind::Asset:
            propertySet.entries.push_back({"Kind", PropertyType::String, PropertyWidgetHint::ReadOnlyLabel, std::string("Asset"), {}, true, false});
            break;
        case SelectionKind::WorldObject:
            propertySet.entries.push_back({"Kind", PropertyType::String, PropertyWidgetHint::ReadOnlyLabel, std::string("WorldObject"), {}, true, false});
            break;
        case SelectionKind::Chunk:
            propertySet.entries.push_back({"Kind", PropertyType::String, PropertyWidgetHint::ReadOnlyLabel, std::string("Chunk"), {}, true, false});
            propertySet.entries.push_back({"Visible", PropertyType::Bool, PropertyWidgetHint::Checkbox, true, {}, false, false});
            break;
        case SelectionKind::Voxel:
            propertySet.entries.push_back({"Kind", PropertyType::String, PropertyWidgetHint::ReadOnlyLabel, std::string("Voxel"), {}, true, false});
            propertySet.entries.push_back({"Solid", PropertyType::Bool, PropertyWidgetHint::Checkbox, true, {}, false, false});
            break;
        default:
            propertySet.entries.push_back({"Kind", PropertyType::String, PropertyWidgetHint::ReadOnlyLabel, std::string("None"), {}, true, false});
            break;
        }

        m_propertyInspectorSystem.SetPropertySet(std::move(propertySet));

        if (m_inspector != nullptr)
        {
            m_inspector->SetPropertyInspectorSystem(&m_propertyInspectorSystem);
        }
    }

    void EditorApp::PushSelectionToStatusBar()
    {
        StatusBarState state {};
        state.activeWorld = m_activeWorld;
        state.worldReadiness = m_worldReadiness;
        state.loadedChunkCount = m_loadedChunkCount;
        state.dirty = m_toolContext.worldDirty || m_propertyInspectorSystem.IsDirty();
        state.selectionSummary = m_selectionService.GetSelectionLabel();

        switch (m_toolContext.activeMode)
        {
        case EditorToolMode::Select: state.activeTool = "Select"; break;
        case EditorToolMode::VoxelInspect: state.activeTool = "VoxelInspect"; break;
        case EditorToolMode::VoxelAdd: state.activeTool = "VoxelAdd"; break;
        case EditorToolMode::VoxelRemove: state.activeTool = "VoxelRemove"; break;
        default: state.activeTool = "Unknown"; break;
        }

        m_statusBar.SetState(std::move(state));
    }
}
