#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

namespace UI {

// ──────────────────────────────────────────────────────────────
// Enumerations
// ──────────────────────────────────────────────────────────────

enum class WidgetType {
    None, Button, Label, TextInput, Checkbox, Slider,
    Dropdown, Image, Panel, ScrollArea, Separator, ProgressBar
};

enum class WidgetEvent {
    None, Clicked, Pressed, Released, Hovered,
    FocusGained, FocusLost, ValueChanged, TextChanged
};

// ──────────────────────────────────────────────────────────────
// Rect
// ──────────────────────────────────────────────────────────────

struct Rect {
    float x = 0.f, y = 0.f, w = 0.f, h = 0.f;

    bool Contains(float px, float py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

// ──────────────────────────────────────────────────────────────
// WidgetStyle
// ──────────────────────────────────────────────────────────────

struct WidgetStyle {
    uint32_t    bgColor      = 0x2B2B2BFF;   // RGBA packed
    uint32_t    fgColor      = 0xFFFFFFFF;
    uint32_t    borderColor  = 0x555555FF;
    float       borderWidth  = 1.0f;
    float       fontSize     = 14.0f;
    std::string fontName     = "default";
    float       padding      = 4.0f;
    float       cornerRadius = 3.0f;
};

// ──────────────────────────────────────────────────────────────
// WidgetState
// ──────────────────────────────────────────────────────────────

struct WidgetState {
    bool hovered = false;
    bool pressed = false;
    bool focused = false;
    bool enabled = true;
    bool visible = true;
};

// ──────────────────────────────────────────────────────────────
// Widget — abstract base class
// ──────────────────────────────────────────────────────────────

class Widget {
public:
    virtual ~Widget() = default;

    virtual WidgetType GetType() const = 0;
    virtual bool OnEvent(WidgetEvent evt, float x, float y) = 0;
    virtual void Tick(float /*dt*/) {}

    // Child management
    void     AddChild(Widget* child);
    void     RemoveChild(uint32_t id);
    Widget*  GetChild(uint32_t id) const;
    const std::vector<std::unique_ptr<Widget>>& Children() const { return m_children; }

    // Bounds
    void        SetBounds(Rect r)          { m_bounds = r; }
    const Rect& GetBounds() const          { return m_bounds; }

    // Visibility / enabled
    void SetVisible(bool v)  { m_state.visible = v; }
    bool IsVisible() const   { return m_state.visible; }
    void SetEnabled(bool e)  { m_state.enabled = e; }
    bool IsEnabled() const   { return m_state.enabled; }

    // Style
    void               SetStyle(WidgetStyle s)  { m_style = std::move(s); }
    const WidgetStyle& GetStyle() const         { return m_style; }

    // Hit testing — depth-first walk returning the deepest visible widget under (x,y)
    Widget* HitTest(float x, float y);

    // Identity
    uint32_t           GetID()   const { return m_id; }
    const std::string& GetName() const { return m_name; }
    void               SetName(const std::string& n) { m_name = n; }

    Widget* Parent() const { return m_parent; }

    static uint32_t NextID();

protected:
    Widget();

    uint32_t    m_id     = 0;
    std::string m_name;
    Rect        m_bounds;
    WidgetStyle m_style;
    WidgetState m_state;
    Widget*     m_parent = nullptr;
    std::vector<std::unique_ptr<Widget>> m_children;
};

// ──────────────────────────────────────────────────────────────
// Button
// ──────────────────────────────────────────────────────────────

class Button : public Widget {
public:
    Button() = default;
    explicit Button(const std::string& label) : m_label(label) {}

    WidgetType GetType() const override { return WidgetType::Button; }
    bool OnEvent(WidgetEvent evt, float x, float y) override;

    void               SetLabel(const std::string& l) { m_label = l; }
    const std::string& GetLabel() const               { return m_label; }

    void SetOnClick(std::function<void()> cb) { m_onClick = std::move(cb); }

private:
    std::string           m_label;
    std::function<void()> m_onClick;
};

// ──────────────────────────────────────────────────────────────
// Label
// ──────────────────────────────────────────────────────────────

class Label : public Widget {
public:
    Label() = default;
    explicit Label(const std::string& text) : m_text(text) {}

    WidgetType GetType() const override { return WidgetType::Label; }
    bool OnEvent(WidgetEvent evt, float x, float y) override;

    void               SetText(const std::string& t) { m_text = t; }
    const std::string& GetText() const               { return m_text; }

private:
    std::string m_text;
};

// ──────────────────────────────────────────────────────────────
// TextInput
// ──────────────────────────────────────────────────────────────

class TextInput : public Widget {
public:
    TextInput() = default;

    WidgetType GetType() const override { return WidgetType::TextInput; }
    bool OnEvent(WidgetEvent evt, float x, float y) override;

    void               SetValue(const std::string& v) { m_value = v; }
    const std::string& GetValue() const               { return m_value; }

    void SetOnTextChanged(std::function<void(const std::string&)> cb) {
        m_onTextChanged = std::move(cb);
    }

private:
    std::string m_value;
    std::function<void(const std::string&)> m_onTextChanged;
};

// ──────────────────────────────────────────────────────────────
// Checkbox
// ──────────────────────────────────────────────────────────────

class Checkbox : public Widget {
public:
    Checkbox() = default;

    WidgetType GetType() const override { return WidgetType::Checkbox; }
    bool OnEvent(WidgetEvent evt, float x, float y) override;

    void SetChecked(bool c) { m_checked = c; }
    bool IsChecked() const  { return m_checked; }

    void SetOnChanged(std::function<void(bool)> cb) { m_onChanged = std::move(cb); }

private:
    bool m_checked = false;
    std::function<void(bool)> m_onChanged;
};

// ──────────────────────────────────────────────────────────────
// Slider
// ──────────────────────────────────────────────────────────────

class Slider : public Widget {
public:
    Slider() = default;
    Slider(float min, float max, float value = 0.f)
        : m_min(min), m_max(max), m_value(value) {}

    WidgetType GetType() const override { return WidgetType::Slider; }
    bool OnEvent(WidgetEvent evt, float x, float y) override;

    void  SetValue(float v)              { m_value = std::clamp(v, m_min, m_max); }
    float GetValue() const               { return m_value; }
    void  SetRange(float mn, float mx)   { m_min = mn; m_max = mx; SetValue(m_value); }
    float GetMin() const                 { return m_min; }
    float GetMax() const                 { return m_max; }

    void SetOnChanged(std::function<void(float)> cb) { m_onChanged = std::move(cb); }

private:
    float m_min = 0.f, m_max = 1.f, m_value = 0.f;
    std::function<void(float)> m_onChanged;
};

// ──────────────────────────────────────────────────────────────
// ProgressBar
// ──────────────────────────────────────────────────────────────

class ProgressBar : public Widget {
public:
    ProgressBar() = default;

    WidgetType GetType() const override { return WidgetType::ProgressBar; }
    bool OnEvent(WidgetEvent evt, float x, float y) override;

    void  SetProgress(float p) { m_value = std::clamp(p, 0.f, 1.f); }
    float GetProgress() const  { return m_value; }

private:
    float m_value = 0.f;
};

} // namespace UI
