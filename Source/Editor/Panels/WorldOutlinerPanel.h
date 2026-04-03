#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace nf
{
    class EditorApp;

    struct RuntimeChunkMetadata
    {
        std::uint64_t id = 0;
        std::string label;
        int coordX = 0;
        int coordY = 0;
        int coordZ = 0;
        bool loaded = true;
        bool dirty = false;
        bool meshed = false;
    };

    struct WorldOutlinerNode
    {
        std::uint64_t id = 0;
        std::string label;
        bool expanded = true;
        bool selected = false;
        std::vector<WorldOutlinerNode> children;
    };

    class WorldOutlinerPanel
    {
    public:
        explicit WorldOutlinerPanel(EditorApp* app = nullptr);

        void SetEditorApp(EditorApp* app);
        void Clear();
        void SetRootNodes(std::vector<WorldOutlinerNode> nodes);

        const std::vector<WorldOutlinerNode>& GetRootNodes() const noexcept;
        bool HasNodes() const noexcept;

        void SelectNode(std::uint64_t id);
        void SyncSelection(std::uint64_t selectedId);

        static std::vector<WorldOutlinerNode> BuildDevWorldTreeFromChunkMetadata(
            const std::string& worldName,
            const std::vector<RuntimeChunkMetadata>& chunks);

    private:
        static const WorldOutlinerNode* FindNodeById(
            const std::vector<WorldOutlinerNode>& nodes,
            std::uint64_t id);

        static void ApplySelectionRecursive(
            std::vector<WorldOutlinerNode>& nodes,
            std::uint64_t selectedId);

    private:
        EditorApp* m_app = nullptr;
        std::vector<WorldOutlinerNode> m_rootNodes;
    };
}
