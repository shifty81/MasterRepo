#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace NF { class UIRenderer; }

namespace NF::Editor {

enum class SplitAxis { Horizontal, Vertical };

struct DockNode {
    uint32_t    id{0};
    SplitAxis   axis{SplitAxis::Horizontal};
    float       splitRatio{0.5f};
    uint32_t    firstChild{0};
    uint32_t    secondChild{0};
    std::string panelName;
    bool        isLeaf{true};
};

constexpr float kPanelTitleBarHeight = 22.f;
class DockingSystem {
public:
    using DrawFn = std::function<void(float x, float y, float w, float h)>;

    struct PanelRect {
        std::string name;
        float x{0.f};
        float y{0.f};
        float w{0.f};
        float h{0.f};
    };

    DockingSystem() = default;

    void SetUIRenderer(UIRenderer* renderer) noexcept { m_Renderer = renderer; }
    void RegisterPanel(const std::string& name, DrawFn drawFn);
    void SetRootSplit(const std::string& leftPanel,
                      const std::string& rightPanel,
                      float ratio = 0.25f);
    void SplitPanel(const std::string& parentName,
                    const std::string& newName,
                    SplitAxis axis,
                    float ratio = 0.5f);
    void Update(float dt);

    /// Build and cache panel content rects for the current root region.
    void BuildLayout(float x, float y, float totalWidth, float totalHeight);

    /// Traverse the tree, draw panel chrome, and invoke each panel draw callback.
    void Draw(float x, float y, float totalWidth, float totalHeight);

    /// Return the last computed content rect for a panel.
    [[nodiscard]] bool GetPanelRect(const std::string& name,
                                    float& x, float& y, float& w, float& h) const noexcept;

private:
    struct PanelEntry {
        std::string name;
        DrawFn      drawFn;
    };

    std::vector<DockNode>   m_Nodes;
    std::vector<PanelEntry> m_Panels;
    std::vector<PanelRect>  m_LastPanelRects;
    uint32_t                m_NextId{1};
    UIRenderer*             m_Renderer{nullptr};

    uint32_t    AllocNode();
    DockNode*   FindLeaf(const std::string& name);
    DrawFn*     FindDrawFn(const std::string& name);
    void        LayoutNode(const DockNode& node, float x, float y, float w, float h);
    void        DrawNode(const DockNode& node, float x, float y, float w, float h);
    void        CachePanelRect(const std::string& name, float x, float y, float w, float h);
};

} // namespace NF::Editor
