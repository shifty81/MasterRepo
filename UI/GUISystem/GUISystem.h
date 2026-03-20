#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace UI {

// ──────────────────────────────────────────────────────────────
// Anchor / alignment flags
// ──────────────────────────────────────────────────────────────

enum class GUIAnchor { TopLeft, Top, TopRight, Left, Center, Right, BottomLeft, Bottom, BottomRight };

// ──────────────────────────────────────────────────────────────
// Rect — normalized or pixel space
// ──────────────────────────────────────────────────────────────

struct GUIRect {
    float x = 0.f, y = 0.f;
    float w = 100.f, h = 30.f;
};

// ──────────────────────────────────────────────────────────────
// Base in-game GUI element
// ──────────────────────────────────────────────────────────────

struct GUIElement {
    std::string id;
    GUIRect     rect;
    GUIAnchor   anchor  = GUIAnchor::TopLeft;
    bool        visible = true;
    bool        enabled = true;
    float       alpha   = 1.f;
    int         layer   = 0;    // draw order

    virtual ~GUIElement() = default;
    virtual std::string Type() const { return "GUIElement"; }
};

// ──────────────────────────────────────────────────────────────
// Concrete element types
// ──────────────────────────────────────────────────────────────

struct GUILabel : GUIElement {
    std::string text;
    float       fontSize = 14.f;
    std::string fontPath;
    uint32_t    color    = 0xFFFFFFFF;
    std::string Type() const override { return "GUILabel"; }
};

struct GUIButton : GUIElement {
    std::string              label;
    std::function<void()>    onClick;
    uint32_t                 normalColor  = 0xFF555555;
    uint32_t                 hoverColor   = 0xFF888888;
    uint32_t                 pressColor   = 0xFF333333;
    std::string Type() const override { return "GUIButton"; }
};

struct GUIImage : GUIElement {
    std::string texturePath;
    bool        tiled     = false;
    float       uvScaleX  = 1.f;
    float       uvScaleY  = 1.f;
    std::string Type() const override { return "GUIImage"; }
};

struct GUIProgressBar : GUIElement {
    float   value     = 0.f;  // 0.0 – 1.0
    uint32_t fillColor    = 0xFF00AA00;
    uint32_t bgColor      = 0xFF333333;
    std::string Type() const override { return "GUIProgressBar"; }
};

struct GUIPanel : GUIElement {
    uint32_t                                     bgColor = 0xAA222222;
    std::vector<std::shared_ptr<GUIElement>>     children;
    std::string Type() const override { return "GUIPanel"; }
};

// ──────────────────────────────────────────────────────────────
// GUIScreen — a named collection of elements (e.g. HUD, PauseMenu)
// ──────────────────────────────────────────────────────────────

struct GUIScreen {
    std::string                                  name;
    bool                                         active = true;
    std::vector<std::shared_ptr<GUIElement>>     elements;
};

// ──────────────────────────────────────────────────────────────
// GUISystem — runtime in-game GUI manager
// ──────────────────────────────────────────────────────────────

class GUISystem {
public:
    // Lifecycle
    void Initialise(uint32_t screenW, uint32_t screenH);
    void Shutdown();
    void Update(float deltaTime);
    void Render();

    // Screen management
    GUIScreen* CreateScreen(const std::string& name);
    GUIScreen* GetScreen(const std::string& name);
    void       DestroyScreen(const std::string& name);
    void       SetActiveScreen(const std::string& name, bool active);

    // Element factory helpers
    std::shared_ptr<GUILabel>       AddLabel(const std::string& screenName,
                                              const std::string& id,
                                              const GUIRect& rect,
                                              const std::string& text);

    std::shared_ptr<GUIButton>      AddButton(const std::string& screenName,
                                               const std::string& id,
                                               const GUIRect& rect,
                                               const std::string& label,
                                               std::function<void()> onClick = nullptr);

    std::shared_ptr<GUIImage>       AddImage(const std::string& screenName,
                                              const std::string& id,
                                              const GUIRect& rect,
                                              const std::string& texturePath);

    std::shared_ptr<GUIProgressBar> AddProgressBar(const std::string& screenName,
                                                    const std::string& id,
                                                    const GUIRect& rect,
                                                    float value = 0.f);

    // Input routing
    void OnMouseMove(float x, float y);
    void OnMousePress(float x, float y, int button);
    void OnMouseRelease(float x, float y, int button);

    // Serialisation (save/load screen layouts)
    bool SaveScreenToJSON(const std::string& screenName, const std::string& filePath) const;
    bool LoadScreenFromJSON(const std::string& filePath);

    // Singleton
    static GUISystem& Get();

private:
    GUIElement* HitTest(GUIScreen& screen, float x, float y);

    std::unordered_map<std::string, GUIScreen> m_screens;
    uint32_t m_screenW = 1280;
    uint32_t m_screenH = 720;
    float    m_mouseX  = 0.f;
    float    m_mouseY  = 0.f;
};

} // namespace UI
