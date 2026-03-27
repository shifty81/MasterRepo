#pragma once
/**
 * @file UISystem.h
 * @brief Retained-mode UI system: panels, buttons, labels, sliders, layout.
 *
 * Features:
 *   - Widget tree: Panel, Label, Button, Image, Slider, Checkbox, TextInput, ScrollView
 *   - Anchor + pivot layout (0-1 normalised screen coords or pixel offsets)
 *   - Parent→child layout with padding/margin and horizontal/vertical stack
 *   - Input dispatch: hover, click (press+release on same widget), drag on slider
 *   - Widget visibility, enabled state, alpha (inherited)
 *   - Style system: colour, font size, border radius, text align
 *   - Z-order: higher z drawn/hit-tested last (topmost)
 *   - Callbacks: OnClick, OnValueChange (slider/checkbox), OnTextChanged
 *   - Named widget lookup for scripting
 *   - JSON layout loading
 *
 * Typical usage:
 * @code
 *   UISystem ui;
 *   ui.Init(1280, 720);
 *   uint32_t btn = ui.CreateButton("start_btn", "Start Game", {0.5f,0.5f}, {120,40});
 *   ui.OnClick(btn, []{ StartGame(); });
 *   // per-frame:
 *   ui.OnMouseMove(mx, my);
 *   ui.OnMouseButton(0, pressed);
 *   ui.Tick(dt);
 *   ui.Render(drawCtx);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class WidgetType : uint8_t {
    Panel=0, Label, Button, Image, Slider,
    Checkbox, TextInput, ScrollView
};

enum class TextAlign : uint8_t { Left=0, Center, Right };
enum class LayoutDir : uint8_t { None=0, Horizontal, Vertical };

struct UIStyle {
    float       bgColour[4]{0.2f,0.2f,0.2f,1.f};
    float       fgColour[4]{1,1,1,1};
    float       borderColour[4]{0.5f,0.5f,0.5f,1.f};
    float       borderWidth{0.f};
    float       borderRadius{4.f};
    float       fontSize{14.f};
    TextAlign   textAlign{TextAlign::Center};
    float       padding[4]{4,4,4,4};  ///< top,right,bottom,left
};

struct UIRect {
    float x{0}, y{0}, w{0}, h{0};
};

struct UIWidgetDesc {
    std::string  id;
    WidgetType   type{WidgetType::Panel};
    UIRect       rect{};           ///< in pixels, relative to parent
    std::string  text;
    std::string  imageId;
    UIStyle      style{};
    float        value{0.f};       ///< slider/checkbox initial value (0 or 1)
    float        minValue{0.f};
    float        maxValue{1.f};
    bool         visible{true};
    bool         enabled{true};
    int32_t      zOrder{0};
    LayoutDir    layoutDir{LayoutDir::None};
    float        layoutSpacing{4.f};
    uint32_t     parentId{0};     ///< 0 = root
};

class UISystem {
public:
    UISystem();
    ~UISystem();

    void Init(uint32_t screenW, uint32_t screenH);
    void Shutdown();
    void Tick(float dt);

    // Resize
    void SetScreenSize(uint32_t w, uint32_t h);

    // Widget creation shortcuts
    uint32_t CreatePanel    (const std::string& id, UIRect rect, uint32_t parentId=0);
    uint32_t CreateLabel    (const std::string& id, const std::string& text,
                              UIRect rect, uint32_t parentId=0);
    uint32_t CreateButton   (const std::string& id, const std::string& label,
                              UIRect rect, uint32_t parentId=0);
    uint32_t CreateSlider   (const std::string& id, UIRect rect,
                              float min=0.f, float max=1.f, float val=0.f,
                              uint32_t parentId=0);
    uint32_t CreateCheckbox (const std::string& id, const std::string& label,
                              UIRect rect, bool checked=false, uint32_t parentId=0);
    uint32_t CreateTextInput(const std::string& id, const std::string& placeholder,
                              UIRect rect, uint32_t parentId=0);

    // General creation
    uint32_t Create(const UIWidgetDesc& desc);
    void     Destroy(uint32_t id);
    void     DestroyAll();

    // Properties
    void        SetText    (uint32_t id, const std::string& text);
    void        SetVisible (uint32_t id, bool visible);
    void        SetEnabled (uint32_t id, bool enabled);
    void        SetValue   (uint32_t id, float value);
    void        SetRect    (uint32_t id, UIRect rect);
    void        SetStyle   (uint32_t id, const UIStyle& style);
    float       GetValue   (uint32_t id) const;
    std::string GetText    (uint32_t id) const;
    bool        IsVisible  (uint32_t id) const;
    uint32_t    FindById   (const std::string& id) const;

    // Input (feed from OS / window callbacks)
    void OnMouseMove  (float x, float y);
    void OnMouseButton(int button, bool pressed);
    void OnChar       (uint32_t codepoint);

    // Callbacks
    void SetOnClick       (uint32_t id, std::function<void()> cb);
    void SetOnValueChange (uint32_t id, std::function<void(float)> cb);
    void SetOnTextChanged (uint32_t id, std::function<void(const std::string&)> cb);

    // Render: calls provided draw primitives (no GL dependency in this header)
    using DrawRectFn = std::function<void(const UIRect&, const float colour[4], float radius)>;
    using DrawTextFn = std::function<void(const std::string& text, float x, float y,
                                           float fontSize, const float colour[4])>;
    void Render(DrawRectFn drawRect, DrawTextFn drawText);

    // JSON layout
    bool LoadLayout(const std::string& path);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
