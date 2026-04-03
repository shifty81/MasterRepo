#include "Editor/Panels/DockingSystem.h"
#include "UI/Rendering/UIRenderer.h"
#include "Core/Logging/Log.h"
#include <algorithm>
#include <stdexcept>

namespace NF::Editor {

static constexpr uint32_t kPanelBgColor       = 0x24262BFF;
static constexpr uint32_t kTitleBarBgColor    = 0x2F343DFF;
static constexpr uint32_t kTitleBarAccent     = 0x5E89B8FF;
static constexpr uint32_t kTitleTextColor     = 0xD7DCE3FF;
static constexpr uint32_t kPanelBorderColor   = 0x4B5563FF;
static constexpr uint32_t kPanelContentShade  = 0x1A1C20AA;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

uint32_t DockingSystem::AllocNode()
{
    DockNode node{};
    node.id = m_NextId++;
    m_Nodes.push_back(node);
    return node.id;
}

DockNode* DockingSystem::FindLeaf(const std::string& name)
{
    for (auto& n : m_Nodes)
        if (n.isLeaf && n.panelName == name)
            return &n;
    return nullptr;
}

DockingSystem::DrawFn* DockingSystem::FindDrawFn(const std::string& name)
{
    for (auto& p : m_Panels)
        if (p.name == name)
            return &p.drawFn;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void DockingSystem::RegisterPanel(const std::string& name, DrawFn drawFn)
{
    for (auto& p : m_Panels) {
        if (p.name == name) {
            p.drawFn = std::move(drawFn);
            return;
        }
    }
    m_Panels.push_back({name, std::move(drawFn)});
    Logger::Log(LogLevel::Debug, "DockingSystem",
                "Registered panel: " + name);
}

void DockingSystem::SetRootSplit(const std::string& leftPanel,
                                  const std::string& rightPanel,
                                  float ratio)
{
    m_Nodes.clear();
    m_NextId = 1;

    // Root split node
    DockNode root{};
    root.id         = m_NextId++;
    root.isLeaf     = false;
    root.axis       = SplitAxis::Horizontal;
    root.splitRatio = std::clamp(ratio, 0.05f, 0.95f);
    root.firstChild = m_NextId;      // will be set after

    // Left leaf
    DockNode left{};
    left.id        = m_NextId++;
    left.isLeaf    = true;
    left.panelName = leftPanel;

    // Right leaf
    DockNode right{};
    right.id        = m_NextId++;
    right.isLeaf    = true;
    right.panelName = rightPanel;

    root.firstChild  = left.id;
    root.secondChild = right.id;

    m_Nodes.push_back(root);
    m_Nodes.push_back(left);
    m_Nodes.push_back(right);

    Logger::Log(LogLevel::Info, "DockingSystem",
                "Root split: " + leftPanel + " | " + rightPanel);
}

void DockingSystem::SplitPanel(const std::string& parentName,
                                const std::string& newName,
                                SplitAxis axis,
                                float ratio)
{
    DockNode* leaf = FindLeaf(parentName);
    if (!leaf) {
        Logger::Log(LogLevel::Warning, "DockingSystem",
                    "SplitPanel: panel not found: " + parentName);
        return;
    }

    // Convert leaf into a split
    std::string existingName = leaf->panelName;
    leaf->isLeaf             = false;
    leaf->axis               = axis;
    leaf->splitRatio         = std::clamp(ratio, 0.05f, 0.95f);
    leaf->panelName.clear();

    // First child keeps the original panel name
    DockNode first{};
    first.id        = m_NextId++;
    first.isLeaf    = true;
    first.panelName = existingName;

    // Second child holds the new panel
    DockNode second{};
    second.id        = m_NextId++;
    second.isLeaf    = true;
    second.panelName = newName;

    leaf->firstChild  = first.id;
    leaf->secondChild = second.id;

    m_Nodes.push_back(first);
    m_Nodes.push_back(second);

    Logger::Log(LogLevel::Info, "DockingSystem",
                "Split '" + existingName + "' → '" + newName + "'");
}

void DockingSystem::Update(float /*dt*/) {}

void DockingSystem::CachePanelRect(const std::string& name,
                                    float x, float y, float w, float h)
{
    for (auto& pr : m_LastPanelRects) {
        if (pr.name == name) {
            pr.x = x; pr.y = y; pr.w = w; pr.h = h;
            return;
        }
    }
    m_LastPanelRects.push_back({name, x, y, w, h});
}

void DockingSystem::LayoutNode(const DockNode& node,
                                float x, float y, float w, float h)
{
    if (node.isLeaf) {
        const float dpi       = m_Renderer ? m_Renderer->GetDpiScale() : 1.f;
        const float titleBarH = kPanelTitleBarHeight * dpi;
        const float contentY  = y + titleBarH;
        const float contentH  = h - titleBarH;
        CachePanelRect(node.panelName, x, contentY, w, contentH);
        return;
    }

    DockNode* first  = nullptr;
    DockNode* second = nullptr;
    for (auto& n : m_Nodes) {
        if (n.id == node.firstChild)  first  = &n;
        if (n.id == node.secondChild) second = &n;
    }
    if (!first || !second) return;

    if (node.axis == SplitAxis::Horizontal) {
        float leftW = w * node.splitRatio;
        LayoutNode(*first,  x,         y, leftW,     h);
        LayoutNode(*second, x + leftW, y, w - leftW, h);
    } else {
        float topH = h * node.splitRatio;
        LayoutNode(*first,  x, y,        w, topH);
        LayoutNode(*second, x, y + topH, w, h - topH);
    }
}

void DockingSystem::BuildLayout(float x, float y, float totalWidth, float totalHeight)
{
    m_LastPanelRects.clear();
    if (m_Nodes.empty()) return;
    LayoutNode(m_Nodes[0], x, y, totalWidth, totalHeight);
}

bool DockingSystem::GetPanelRect(const std::string& name,
                                  float& x, float& y, float& w, float& h) const noexcept
{
    for (const auto& pr : m_LastPanelRects) {
        if (pr.name == name) {
            x = pr.x; y = pr.y; w = pr.w; h = pr.h;
            return true;
        }
    }
    return false;
}

void DockingSystem::DrawNode(const DockNode& node,
                              float x, float y, float w, float h)
{
    if (node.isLeaf) {
        // Draw panel chrome via UIRenderer if available
        if (m_Renderer && w > 0.f && h > 0.f) {
            const float dpi       = m_Renderer->GetDpiScale();
            const float titleBarH = kPanelTitleBarHeight * dpi;
            const float contentY  = y + titleBarH;
            const float contentH  = h - titleBarH;

            // Panel background
            m_Renderer->DrawRect({x, y, w, h}, kPanelBgColor);

            // Title bar
            m_Renderer->DrawRect({x, y, w, titleBarH}, kTitleBarBgColor);
            m_Renderer->DrawRect({x, y, w, 2.f * dpi}, kTitleBarAccent);
            if (contentH > 0.f)
                m_Renderer->DrawRect({x, contentY, w, contentH}, kPanelContentShade);

            // Title text — scale 2 at 96 DPI; UIRenderer multiplies by DPI.
            if (!node.panelName.empty()) {
                m_Renderer->DrawText(node.panelName,
                                     x + 6.f * dpi, y + 4.f * dpi,
                                     kTitleTextColor, 2.f);
            }

            // Panel border
            m_Renderer->DrawOutlineRect({x, y, w, h}, kPanelBorderColor);
        }

        // Call the panel's content draw callback with the content area
        // (below the title bar).
        if (DrawFn* fn = FindDrawFn(node.panelName)) {
            const float dpi       = m_Renderer ? m_Renderer->GetDpiScale() : 1.f;
            const float titleBarH = kPanelTitleBarHeight * dpi;
            const float contentY  = y + titleBarH;
            const float contentH  = h - titleBarH;
            if (contentH > 0.f) {
                if (m_Renderer) m_Renderer->PushClipRect({x, contentY, w, contentH});
                (*fn)(x, contentY, w, contentH);
                if (m_Renderer) m_Renderer->PopClipRect();
            }
        }
        return;
    }

    // Find children
    DockNode* first  = nullptr;
    DockNode* second = nullptr;
    for (auto& n : m_Nodes) {
        if (n.id == node.firstChild)  first  = &n;
        if (n.id == node.secondChild) second = &n;
    }
    if (!first || !second) return;

    if (node.axis == SplitAxis::Horizontal) {
        float leftW = w * node.splitRatio;
        DrawNode(*first,  x,          y, leftW,      h);
        DrawNode(*second, x + leftW,  y, w - leftW,  h);
    } else {
        float topH = h * node.splitRatio;
        DrawNode(*first,  x, y,          w, topH);
        DrawNode(*second, x, y + topH,   w, h - topH);
    }
}

void DockingSystem::Draw(float x, float y, float totalWidth, float totalHeight)
{
    if (m_Nodes.empty()) return;
    BuildLayout(x, y, totalWidth, totalHeight);
    DrawNode(m_Nodes[0], x, y, totalWidth, totalHeight);
}

} // namespace NF::Editor
