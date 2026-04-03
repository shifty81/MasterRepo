#pragma once

#include <cstdint>
#include <string>

namespace nf
{
    class EditorApp;

    struct ViewportHighlightState
    {
        std::uint64_t selectedWorldObjectId = 0;
        std::uint64_t selectedChunkId = 0;
        std::uint64_t selectedVoxelId = 0;
        std::string highlightLabel;
    };

    class EditorViewport
    {
    public:
        explicit EditorViewport(EditorApp* app = nullptr);

        void SetEditorApp(EditorApp* app);

        void SimulateSelectWorldObject(std::uint64_t objectId, const std::string& label);
        void SimulateSelectChunk(std::uint64_t chunkId, const std::string& label);
        void SimulateSelectVoxel(std::uint64_t voxelId, const std::string& label);

        void SyncSelectionHighlight();
        const ViewportHighlightState& GetHighlightState() const noexcept;

    private:
        EditorApp* m_app = nullptr;
        ViewportHighlightState m_highlightState;
    };
}
