#pragma once
#include "Renderer/Materials/Material.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace NF::Editor {

/// @brief Type of value produced or consumed by a node pin.
enum class PinType { Float, Vec3, Color, Texture, Bool };

/// @brief A single input or output pin on a material node.
struct MaterialPin {
    uint32_t id{0};       ///< Globally unique pin identifier.
    std::string name;
    PinType     type{PinType::Float};
    bool        isOutput{false};
};

/// @brief One node in the material node graph.
struct MaterialNode {
    uint32_t              id{0};
    std::string           kind;     ///< e.g. "Multiply", "Texture2D", "Output"
    std::string           label;
    float                 posX{0.f}, posY{0.f};
    std::vector<MaterialPin> pins;
};

/// @brief A connection (edge) between two pins.
struct MaterialLink {
    uint32_t id{0};
    uint32_t fromPin{0};
    uint32_t toPin{0};
};

/// @brief Node-graph material editor panel.
///
/// Maintains a graph of MaterialNode / MaterialLink and can compile
/// the graph into a NF::Material for live preview.
class MaterialEditorPanel {
public:
    MaterialEditorPanel();

    /// @brief Add a new node of the given kind at (x, y).
    /// @return ID of the newly created node.
    uint32_t AddNode(const std::string& kind, float x = 0.f, float y = 0.f);

    /// @brief Remove a node and any links connected to it.
    void RemoveNode(uint32_t nodeId);

    /// @brief Connect two pins.
    /// @return Link ID, or 0 if the connection is invalid.
    uint32_t AddLink(uint32_t fromPinId, uint32_t toPinId);

    /// @brief Disconnect a link.
    void RemoveLink(uint32_t linkId);

    /// @brief Return all nodes.
    [[nodiscard]] const std::vector<MaterialNode>& GetNodes() const noexcept { return m_Nodes; }

    /// @brief Return all links.
    [[nodiscard]] const std::vector<MaterialLink>& GetLinks() const noexcept { return m_Links; }

    /// @brief Attempt to compile the graph into a material.
    /// @return Shared pointer to the compiled material, or nullptr on error.
    [[nodiscard]] std::shared_ptr<Material> Compile() const;

    /// @brief Draw the node-graph panel.
    void Draw();

    /// @brief Returns true while the panel should be visible.
    [[nodiscard]] bool IsOpen() const noexcept { return m_Open; }

    /// @brief Show or hide the panel.
    void SetOpen(bool open) noexcept { m_Open = open; }

private:
    std::vector<MaterialNode> m_Nodes;
    std::vector<MaterialLink> m_Links;
    uint32_t                  m_NextId{1};
    bool                      m_Open{true};

    uint32_t    AllocId() noexcept { return m_NextId++; }
    MaterialNode* FindNode(uint32_t id);

    /// @brief Create the default pins for a node kind.
    std::vector<MaterialPin> MakePins(const std::string& kind);
};

} // namespace NF::Editor
