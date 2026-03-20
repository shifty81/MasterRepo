#include "UI/Layouts/Layout.h"
#include <numeric>

namespace UI {

// ──────────────────────────────────────────────────────────────
// StackLayout
// ──────────────────────────────────────────────────────────────

int StackLayout::AddItem(Widget* widget, float grow) {
    LayoutItem item;
    item.widget = widget;
    item.grow   = grow;
    m_items.push_back(item);
    return static_cast<int>(m_items.size()) - 1;
}

void StackLayout::RemoveItem(Widget* widget) {
    auto it = std::find_if(m_items.begin(), m_items.end(),
        [widget](const LayoutItem& i) { return i.widget == widget; });
    if (it != m_items.end()) m_items.erase(it);
}

void StackLayout::Arrange(Rect bounds) {
    if (m_items.empty()) return;

    const bool horiz = (m_direction == LayoutDirection::Horizontal);
    const float pad2 = m_padding * 2.f;
    const float totalSpace = (horiz ? bounds.w : bounds.h) - pad2;
    const float gapTotal   = m_spacing * static_cast<float>(m_items.size() - 1);

    // Base sizes (min or equal share of remaining space after grow/shrink pass)
    float fixedTotal = 0.f;
    float totalGrow  = 0.f;
    for (const auto& item : m_items) {
        fixedTotal += item.minSize;
        totalGrow  += item.grow;
    }

    float flexSpace = totalSpace - fixedTotal - gapTotal;
    if (flexSpace < 0.f) flexSpace = 0.f;

    float cursor = (horiz ? bounds.x : bounds.y) + m_padding;

    for (const auto& item : m_items) {
        float mainSize = item.minSize;
        if (totalGrow > 0.f && item.grow > 0.f)
            mainSize += flexSpace * (item.grow / totalGrow);
        mainSize = std::clamp(mainSize, item.minSize, item.maxSize);

        Rect r;
        if (horiz) {
            r.x = cursor;
            r.y = bounds.y + m_padding;
            r.w = mainSize;
            r.h = (item.align == LayoutAlign::Stretch)
                    ? (bounds.h - pad2)
                    : std::min(item.maxSize, bounds.h - pad2);
        } else {
            r.x = bounds.x + m_padding;
            r.y = cursor;
            r.w = (item.align == LayoutAlign::Stretch)
                    ? (bounds.w - pad2)
                    : std::min(item.maxSize, bounds.w - pad2);
            r.h = mainSize;
        }

        if (item.widget) item.widget->SetBounds(r);
        cursor += mainSize + m_spacing;
    }
}

// ──────────────────────────────────────────────────────────────
// GridLayout
// ──────────────────────────────────────────────────────────────

std::pair<int,int> GridLayout::NextSlot() const {
    int maxRow = -1;
    // Find occupied slots
    std::vector<std::pair<int,int>> occupied;
    for (const auto& e : m_entries)
        occupied.emplace_back(e.col, e.row);

    // Scan reading order for first free slot
    for (int row = 0; row <= static_cast<int>(m_entries.size()); ++row) {
        for (int col = 0; col < m_cols; ++col) {
            auto it = std::find(occupied.begin(), occupied.end(), std::make_pair(col, row));
            if (it == occupied.end()) return {col, row};
        }
    }
    return {0, 0};
}

void GridLayout::AddItem(Widget* widget, int col, int row) {
    GridEntry e;
    e.widget = widget;
    if (col < 0 || row < 0) {
        auto [c, r] = NextSlot();
        e.col = c;
        e.row = r;
    } else {
        e.col = col;
        e.row = row;
    }
    m_entries.push_back(e);
}

void GridLayout::RemoveItem(Widget* widget) {
    auto it = std::find_if(m_entries.begin(), m_entries.end(),
        [widget](const GridEntry& e) { return e.widget == widget; });
    if (it != m_entries.end()) m_entries.erase(it);
}

void GridLayout::Arrange(Rect bounds) {
    for (const auto& e : m_entries) {
        if (!e.widget) continue;
        Rect r;
        r.x = bounds.x + e.col * (m_colWidth  + m_spacing);
        r.y = bounds.y + e.row * (m_rowHeight + m_spacing);
        r.w = m_colWidth;
        r.h = m_rowHeight;
        e.widget->SetBounds(r);
    }
}

// ──────────────────────────────────────────────────────────────
// AnchorLayout
// ──────────────────────────────────────────────────────────────

void AnchorLayout::AddItem(Widget* widget,
                           float anchorX, float anchorY,
                           float offX,    float offY,
                           float w,       float h)
{
    AnchorItem item;
    item.widget  = widget;
    item.anchorX = anchorX;
    item.anchorY = anchorY;
    item.offsetX = offX;
    item.offsetY = offY;
    item.sizeW   = w;
    item.sizeH   = h;
    m_items.push_back(item);
}

void AnchorLayout::RemoveItem(Widget* widget) {
    auto it = std::find_if(m_items.begin(), m_items.end(),
        [widget](const AnchorItem& i) { return i.widget == widget; });
    if (it != m_items.end()) m_items.erase(it);
}

void AnchorLayout::Arrange(Rect bounds) {
    for (const auto& item : m_items) {
        if (!item.widget) continue;
        Rect r;
        r.x = bounds.x + item.anchorX * bounds.w + item.offsetX;
        r.y = bounds.y + item.anchorY * bounds.h + item.offsetY;
        r.w = item.sizeW;
        r.h = item.sizeH;
        item.widget->SetBounds(r);
    }
}

} // namespace UI
