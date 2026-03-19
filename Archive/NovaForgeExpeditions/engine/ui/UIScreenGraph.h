#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace atlas::ui {

enum class UIWidgetType : uint8_t {
    Panel,
    Button,
    Text,
    Image,
    List,
    SlotGrid,
    InputField,
    Menu,
    MenuItem,
    Toolbar,
    StatusBar,
    Tooltip,
    Tab,
    ScrollView,
    DockArea,
    Checkbox,
    Slider,
    ProgressBar,
    ComboBox,
    TreeNode,
    Splitter,
    ColorPicker
};

struct UIWidget {
    uint32_t id = 0;
    UIWidgetType type = UIWidgetType::Panel;
    std::string name;
    bool visible = true;
    uint32_t parentId = 0;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    
    // Menu-specific state
    bool isMenuOpen = false;    // For Menu widgets: is dropdown shown?
    bool isHovered = false;     // For MenuItem widgets: is mouse over this item?
    bool isSeparator = false;   // For MenuItem widgets: is this a separator line?
    bool isDisabled = false;    // For MenuItem/Button: grayed-out and non-interactive?
    std::string shortcutLabel;  // For MenuItem: keyboard shortcut display (e.g. "Ctrl+S")
    bool hasSubmenu = false;    // For MenuItem: does this item open a submenu?
    bool isCheckable = false;   // For MenuItem: can this item be checked/unchecked?
    bool isChecked = false;     // For MenuItem: is this item currently checked?
    uint32_t iconId = 0;        // Icon texture/atlas ID (0 = no icon)

    // Slider/ProgressBar state
    float value = 0.0f;          // For Slider: current position (0.0-1.0), For ProgressBar: progress (0.0-1.0)
    float minValue = 0.0f;       // For Slider: minimum value
    float maxValue = 1.0f;       // For Slider: maximum value

    // ComboBox state
    int32_t selectedIndex = -1;  // For ComboBox: currently selected item (-1 = none)
    bool isOpen = false;         // For ComboBox: is dropdown open?

    // TreeNode state
    bool isExpanded = false;     // For TreeNode: is this node expanded?
    int32_t treeDepth = 0;       // For TreeNode: nesting depth level

    // ColorPicker state
    uint8_t colorR = 255;
    uint8_t colorG = 255;
    uint8_t colorB = 255;
    uint8_t colorA = 255;
};

class UIScreen {
public:
    void Init(const std::string& name);
    const std::string& GetName() const;

    uint32_t AddWidget(UIWidgetType type, const std::string& name, float x, float y, float w, float h);
    void RemoveWidget(uint32_t id);
    const UIWidget* GetWidget(uint32_t id) const;
    size_t WidgetCount() const;

    void SetVisible(uint32_t id, bool visible);
    bool IsVisible(uint32_t id) const;

    void SetParent(uint32_t childId, uint32_t parentId);
    std::vector<uint32_t> GetChildren(uint32_t parentId) const;

    // Menu state management
    void SetMenuOpen(uint32_t id, bool open);
    bool IsMenuOpen(uint32_t id) const;
    void SetHovered(uint32_t id, bool hovered);
    bool IsHovered(uint32_t id) const;
    void SetSeparator(uint32_t id, bool isSeparator);
    void SetDisabled(uint32_t id, bool disabled);
    bool IsDisabled(uint32_t id) const;
    void SetShortcutLabel(uint32_t id, const std::string& label);
    void SetHasSubmenu(uint32_t id, bool hasSubmenu);
    void SetCheckable(uint32_t id, bool checkable);
    bool IsCheckable(uint32_t id) const;
    void SetChecked(uint32_t id, bool checked);
    bool IsChecked(uint32_t id) const;
    void SetIconId(uint32_t id, uint32_t iconId);
    uint32_t GetIconId(uint32_t id) const;
    UIWidget* GetWidgetMutable(uint32_t id);

    // Slider/ProgressBar state management
    void SetValue(uint32_t id, float value);
    float GetValue(uint32_t id) const;
    void SetValueRange(uint32_t id, float minVal, float maxVal);
    float GetMinValue(uint32_t id) const;
    float GetMaxValue(uint32_t id) const;

    // ComboBox state management
    void SetSelectedIndex(uint32_t id, int32_t index);
    int32_t GetSelectedIndex(uint32_t id) const;
    void SetComboOpen(uint32_t id, bool open);
    bool IsComboOpen(uint32_t id) const;

    // TreeNode state management
    void SetExpanded(uint32_t id, bool expanded);
    bool IsExpanded(uint32_t id) const;
    void SetTreeDepth(uint32_t id, int32_t depth);
    int32_t GetTreeDepth(uint32_t id) const;

    // ColorPicker state management
    void SetColor(uint32_t id, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void GetColor(uint32_t id, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const;

    /// Proportionally scale all widget positions and sizes from
    /// (oldWidth × oldHeight) to (newWidth × newHeight).  This is
    /// called when the window is resized so the editor layout adapts.
    void ScaleLayout(float oldWidth, float oldHeight,
                     float newWidth, float newHeight);

private:
    std::string m_name;
    std::unordered_map<uint32_t, UIWidget> m_widgets;
    uint32_t m_nextId = 1;
};

} // namespace atlas::ui
