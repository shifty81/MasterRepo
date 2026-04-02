#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Axis along which a dock node is split.
enum class SplitAxis { Horizontal, Vertical };

/// @brief A leaf or split node in the docking tree.
struct DockNode {
    uint32_t    id{0};          ///< Unique node identifier.
    SplitAxis   axis{SplitAxis::Horizontal};
    float       splitRatio{0.5f}; ///< Fraction [0,1] given to the first child.
    uint32_t    firstChild{0};
    uint32_t    secondChild{0};
    std::string panelName;       ///< Non-empty only for leaf nodes.
    bool        isLeaf{true};
};

/// @brief Height in pixels of the panel title bar drawn by the docking system.
constexpr float kPanelTitleBarHeight = 22.f;

/// @brief Manages a tree of dockable panels inside the editor.
///
/// Panels register themselves by name; the system resolves layout
/// at each frame and calls each panel's Draw() callback in the
/// correct region.  When a UIRenderer is provided the system also
/// draws panel chrome (background, border, title bar).
class DockingSystem {
public:
    using DrawFn = std::function<void(float x, float y, float w, float h)>;

    DockingSystem() = default;

    /// @brief Set the UIRenderer used for panel chrome.  May be nullptr.
    void SetUIRenderer(UIRenderer* renderer) noexcept { m_Renderer = renderer; }

    /// @brief Register a named panel with its draw callback.
    /// @param name   Unique panel name.
    /// @param drawFn Called each frame with the panel's pixel region.
    void RegisterPanel(const std::string& name, DrawFn drawFn);

    /// @brief Create a root horizontal split between two panels.
    /// @param leftPanel   Panel name for the left region.
    /// @param rightPanel  Panel name for the right region.
    /// @param ratio       Fraction of total width given to the left panel.
    void SetRootSplit(const std::string& leftPanel,
                      const std::string& rightPanel,
                      float ratio = 0.25f);

    /// @brief Split an existing leaf node into two children.
    /// @param parentName Leaf panel to split.
    /// @param newName    Panel name for the new sibling.
    /// @param axis       Direction of the split.
    /// @param ratio      Fraction of the parent region given to @p parentName.
    void SplitPanel(const std::string& parentName,
                    const std::string& newName,
                    SplitAxis axis,
                    float ratio = 0.5f);

    /// @brief Advance any animated split transitions.
    void Update(float dt);

    /// @brief Traverse the tree and invoke each panel's draw callback.
    /// @param totalWidth  Available width in pixels.
    /// @param totalHeight Available height in pixels.
    void Draw(float totalWidth, float totalHeight);

private:
    struct PanelEntry {
        std::string name;
        DrawFn      drawFn;
    };

    std::vector<DockNode>   m_Nodes;
    std::vector<PanelEntry> m_Panels;
    uint32_t                m_NextId{1};
    UIRenderer*             m_Renderer{nullptr};

    uint32_t    AllocNode();
    DockNode*   FindLeaf(const std::string& name);
    void        DrawNode(const DockNode& node,
                         float x, float y, float w, float h);
    DrawFn*     FindDrawFn(const std::string& name);
};

} // namespace NF::Editor
