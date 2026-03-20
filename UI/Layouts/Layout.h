#pragma once
#include <vector>
#include <cfloat>
#include <algorithm>
#include "UI/Widgets/Widget.h"

namespace UI {

// ──────────────────────────────────────────────────────────────
// Enumerations
// ──────────────────────────────────────────────────────────────

enum class LayoutDirection { Horizontal, Vertical };
enum class LayoutAlign     { Start, Center, End, Stretch };

// ──────────────────────────────────────────────────────────────
// LayoutItem — used by StackLayout
// ──────────────────────────────────────────────────────────────

struct LayoutItem {
    Widget*     widget  = nullptr;
    float       grow    = 0.f;
    float       shrink  = 1.f;
    float       minSize = 0.f;
    float       maxSize = FLT_MAX;
    LayoutAlign align   = LayoutAlign::Stretch;
};

// ──────────────────────────────────────────────────────────────
// StackLayout — single-axis flex-like layout
// ──────────────────────────────────────────────────────────────

class StackLayout {
public:
    explicit StackLayout(LayoutDirection dir = LayoutDirection::Vertical,
                         float spacing = 4.f, float padding = 0.f)
        : m_direction(dir), m_spacing(spacing), m_padding(padding) {}

    // Add a widget; returns item index
    int  AddItem(Widget* widget, float grow = 0.f);
    void RemoveItem(Widget* widget);

    // Compute and set bounds on every widget within the given container rect
    void Arrange(Rect bounds);

    const std::vector<LayoutItem>& GetItems() const { return m_items; }

    void SetDirection(LayoutDirection d) { m_direction = d; }
    void SetSpacing(float s)             { m_spacing   = s; }
    void SetPadding(float p)             { m_padding   = p; }

private:
    LayoutDirection         m_direction;
    float                   m_spacing;
    float                   m_padding;
    std::vector<LayoutItem> m_items;
};

// ──────────────────────────────────────────────────────────────
// GridLayout
// ──────────────────────────────────────────────────────────────

struct GridEntry {
    Widget* widget = nullptr;
    int     col    = 0;
    int     row    = 0;
};

class GridLayout {
public:
    explicit GridLayout(int cols = 2, float colWidth = 120.f,
                        float rowHeight = 28.f, float spacing = 4.f)
        : m_cols(cols), m_colWidth(colWidth),
          m_rowHeight(rowHeight), m_spacing(spacing) {}

    // col/row = -1 → auto-place in reading order
    void AddItem(Widget* widget, int col = -1, int row = -1);
    void RemoveItem(Widget* widget);

    void Arrange(Rect bounds);

    const std::vector<GridEntry>& GetItems() const { return m_entries; }

    int   Cols()      const { return m_cols; }
    float ColWidth()  const { return m_colWidth; }
    float RowHeight() const { return m_rowHeight; }

    void SetCols(int c)          { m_cols      = c; }
    void SetColWidth(float w)    { m_colWidth  = w; }
    void SetRowHeight(float h)   { m_rowHeight = h; }
    void SetSpacing(float s)     { m_spacing   = s; }

private:
    int                    m_cols;
    float                  m_colWidth;
    float                  m_rowHeight;
    float                  m_spacing;
    std::vector<GridEntry> m_entries;

    // Returns the next free (col, row) pair in reading order
    std::pair<int,int> NextSlot() const;
};

// ──────────────────────────────────────────────────────────────
// AnchorLayout — absolute / anchored positioning
// ──────────────────────────────────────────────────────────────

struct AnchorItem {
    Widget* widget  = nullptr;
    float   anchorX = 0.f;  // 0=left edge, 1=right edge of parent
    float   anchorY = 0.f;  // 0=top  edge, 1=bottom edge of parent
    float   offsetX = 0.f;
    float   offsetY = 0.f;
    float   sizeW   = 100.f;
    float   sizeH   = 24.f;
};

class AnchorLayout {
public:
    void AddItem(Widget* widget,
                 float anchorX, float anchorY,
                 float offX,    float offY,
                 float w,       float h);
    void RemoveItem(Widget* widget);

    // Position each widget relative to bounds using anchor + offset
    void Arrange(Rect bounds);

    const std::vector<AnchorItem>& GetItems() const { return m_items; }

private:
    std::vector<AnchorItem> m_items;
};

} // namespace UI
