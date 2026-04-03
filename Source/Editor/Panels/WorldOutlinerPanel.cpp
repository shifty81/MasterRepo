#include "Editor/Panels/WorldOutlinerPanel.h"

#include <utility>

#include "Editor/Application/EditorAppIntegration.h"

namespace nf
{
    WorldOutlinerPanel::WorldOutlinerPanel(EditorApp* app)
        : m_app(app)
    {
    }

    void WorldOutlinerPanel::SetEditorApp(EditorApp* app)
    {
        m_app = app;
    }

    void WorldOutlinerPanel::Clear()
    {
        m_rootNodes.clear();
    }

    void WorldOutlinerPanel::SetRootNodes(std::vector<WorldOutlinerNode> nodes)
    {
        m_rootNodes = std::move(nodes);
    }

    const std::vector<WorldOutlinerNode>& WorldOutlinerPanel::GetRootNodes() const noexcept
    {
        return m_rootNodes;
    }

    bool WorldOutlinerPanel::HasNodes() const noexcept
    {
        return !m_rootNodes.empty();
    }

    void WorldOutlinerPanel::SelectNode(std::uint64_t id)
    {
        SyncSelection(id);

        const auto* node = FindNodeById(m_rootNodes, id);
        if (node == nullptr || m_app == nullptr)
        {
            return;
        }

        m_app->OnWorldOutlinerNodeSelected(node->id, node->label);
    }

    void WorldOutlinerPanel::SyncSelection(std::uint64_t selectedId)
    {
        ApplySelectionRecursive(m_rootNodes, selectedId);
    }

    std::vector<WorldOutlinerNode> WorldOutlinerPanel::BuildDevWorldTreeFromChunkMetadata(
        const std::string& worldName,
        const std::vector<RuntimeChunkMetadata>& chunks)
    {
        WorldOutlinerNode root {};
        root.id = 1;
        root.label = worldName.empty() ? "DevWorld" : worldName;

        WorldOutlinerNode chunksNode {};
        chunksNode.id = 2;
        chunksNode.label = "Chunks";

        for (const auto& chunk : chunks)
        {
            WorldOutlinerNode child {};
            child.id = chunk.id;

            child.label = chunk.label
                + " ["
                + std::to_string(chunk.coordX) + ","
                + std::to_string(chunk.coordY) + ","
                + std::to_string(chunk.coordZ) + "]"
                + (chunk.loaded ? " L" : "")
                + (chunk.dirty ? " D" : "")
                + (chunk.meshed ? " M" : "");

            chunksNode.children.push_back(std::move(child));
        }

        root.children.push_back(std::move(chunksNode));
        return { std::move(root) };
    }

    const WorldOutlinerNode* WorldOutlinerPanel::FindNodeById(
        const std::vector<WorldOutlinerNode>& nodes,
        std::uint64_t id)
    {
        for (const auto& node : nodes)
        {
            if (node.id == id)
            {
                return &node;
            }

            if (!node.children.empty())
            {
                const auto* child = FindNodeById(node.children, id);
                if (child != nullptr)
                {
                    return child;
                }
            }
        }
        return nullptr;
    }

    void WorldOutlinerPanel::ApplySelectionRecursive(
        std::vector<WorldOutlinerNode>& nodes,
        std::uint64_t selectedId)
    {
        for (auto& node : nodes)
        {
            node.selected = (node.id == selectedId);
            if (!node.children.empty())
            {
                ApplySelectionRecursive(node.children, selectedId);
            }
        }
    }
}
