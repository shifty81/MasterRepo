#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Editor/Inspector/PropertyInspectorSystem.h"
#include "Editor/Panels/StatusBarPanel.h"
#include "Editor/Panels/WorldOutlinerPanel.h"
#include "Editor/Selection/SelectionService.h"
#include "Editor/Tools/EditorToolContext.h"

namespace nf
{
    class ContentBrowser;
    class Inspector;
    class EditorViewport;

    class EditorApp
    {
    public:
        EditorApp() = default;

        void SetContentBrowser(ContentBrowser* contentBrowser);
        void SetInspector(Inspector* inspector);
        void SetViewport(EditorViewport* viewport);

        SelectionService& GetSelectionService() noexcept;
        const SelectionService& GetSelectionService() const noexcept;

        PropertyInspectorSystem& GetPropertyInspectorSystem() noexcept;
        const PropertyInspectorSystem& GetPropertyInspectorSystem() const noexcept;

        StatusBarPanel& GetStatusBar() noexcept;
        const StatusBarPanel& GetStatusBar() const noexcept;

        WorldOutlinerPanel& GetWorldOutliner() noexcept;
        const WorldOutlinerPanel& GetWorldOutliner() const noexcept;

        EditorToolContext& GetToolContext() noexcept;
        const EditorToolContext& GetToolContext() const noexcept;

        void ApplySelection(const SelectionHandle& handle);
        void ClearSelection();

        void RebuildInspectorFromSelection();
        void RebuildStatusBar(
            const std::string& activeWorld,
            const std::string& worldReadiness,
            int loadedChunkCount);

        void RebuildWorldOutlinerFromChunks(
            const std::string& worldName,
            const std::vector<RuntimeChunkMetadata>& chunks);

        void OnContentBrowserAssetSelected(
            std::uint64_t assetId,
            const std::string& assetName,
            const std::string& assetPath);

        void OnViewportWorldObjectSelected(
            std::uint64_t objectId,
            const std::string& objectLabel);

        void OnViewportChunkSelected(
            std::uint64_t chunkId,
            const std::string& chunkLabel);

        void OnViewportVoxelSelected(
            std::uint64_t voxelId,
            const std::string& voxelLabel);

        void OnWorldOutlinerNodeSelected(
            std::uint64_t nodeId,
            const std::string& nodeLabel);

        void SyncSelectionToViewportAndOutliner();
        void MarkInspectorEditsDirty();

    private:
        void PushSelectionToInspector();
        void PushSelectionToStatusBar();

    private:
        ContentBrowser* m_contentBrowser = nullptr;
        Inspector* m_inspector = nullptr;
        EditorViewport* m_viewport = nullptr;

        SelectionService m_selectionService;
        PropertyInspectorSystem m_propertyInspectorSystem;
        StatusBarPanel m_statusBar;
        WorldOutlinerPanel m_worldOutliner;
        EditorToolContext m_toolContext;

        std::string m_activeWorld = "DevWorld";
        std::string m_worldReadiness = "Not Ready";
        int m_loadedChunkCount = 0;
    };
}
