#include "UI/GUISystem/GUISystem.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace UI {

// ── Singleton ──────────────────────────────────────────────────

GUISystem& GUISystem::Get() {
    static GUISystem instance;
    return instance;
}

// ── Lifecycle ──────────────────────────────────────────────────

void GUISystem::Initialise(uint32_t screenW, uint32_t screenH) {
    m_screenW = screenW;
    m_screenH = screenH;
}

void GUISystem::Shutdown() {
    m_screens.clear();
}

void GUISystem::Update(float deltaTime) {
    (void)deltaTime;
    // Future: run animations, update data-bindings
}

void GUISystem::Render() {
    // Iterate screens in layer order and render each element
    for (auto& [name, screen] : m_screens) {
        if (!screen.active) continue;
        // Render elements sorted by layer
        std::vector<GUIElement*> elems;
        elems.reserve(screen.elements.size());
        for (auto& e : screen.elements) {
            if (e && e->visible) elems.push_back(e.get());
        }
        std::stable_sort(elems.begin(), elems.end(),
                         [](const GUIElement* a, const GUIElement* b){
                             return a->layer < b->layer;
                         });
        for (auto* e : elems) {
            // Rendering dispatched to platform renderer (stub)
            (void)e;
        }
    }
}

// ── Screen management ──────────────────────────────────────────

GUIScreen* GUISystem::CreateScreen(const std::string& name) {
    m_screens[name] = GUIScreen{name, true, {}};
    return &m_screens[name];
}

GUIScreen* GUISystem::GetScreen(const std::string& name) {
    auto it = m_screens.find(name);
    return (it != m_screens.end()) ? &it->second : nullptr;
}

void GUISystem::DestroyScreen(const std::string& name) {
    m_screens.erase(name);
}

void GUISystem::SetActiveScreen(const std::string& name, bool active) {
    auto* s = GetScreen(name);
    if (s) s->active = active;
}

// ── Element factory ────────────────────────────────────────────

std::shared_ptr<GUILabel> GUISystem::AddLabel(const std::string& screenName,
                                               const std::string& id,
                                               const GUIRect& rect,
                                               const std::string& text) {
    auto* screen = GetScreen(screenName);
    if (!screen) screen = CreateScreen(screenName);
    auto elem    = std::make_shared<GUILabel>();
    elem->id     = id;
    elem->rect   = rect;
    elem->text   = text;
    screen->elements.push_back(elem);
    return elem;
}

std::shared_ptr<GUIButton> GUISystem::AddButton(const std::string& screenName,
                                                 const std::string& id,
                                                 const GUIRect& rect,
                                                 const std::string& label,
                                                 std::function<void()> onClick) {
    auto* screen    = GetScreen(screenName);
    if (!screen) screen = CreateScreen(screenName);
    auto elem       = std::make_shared<GUIButton>();
    elem->id        = id;
    elem->rect      = rect;
    elem->label     = label;
    elem->onClick   = std::move(onClick);
    screen->elements.push_back(elem);
    return elem;
}

std::shared_ptr<GUIImage> GUISystem::AddImage(const std::string& screenName,
                                               const std::string& id,
                                               const GUIRect& rect,
                                               const std::string& texturePath) {
    auto* screen      = GetScreen(screenName);
    if (!screen) screen = CreateScreen(screenName);
    auto elem         = std::make_shared<GUIImage>();
    elem->id          = id;
    elem->rect        = rect;
    elem->texturePath = texturePath;
    screen->elements.push_back(elem);
    return elem;
}

std::shared_ptr<GUIProgressBar> GUISystem::AddProgressBar(const std::string& screenName,
                                                           const std::string& id,
                                                           const GUIRect& rect,
                                                           float value) {
    auto* screen  = GetScreen(screenName);
    if (!screen) screen = CreateScreen(screenName);
    auto elem     = std::make_shared<GUIProgressBar>();
    elem->id      = id;
    elem->rect    = rect;
    elem->value   = value;
    screen->elements.push_back(elem);
    return elem;
}

// ── Input routing ──────────────────────────────────────────────

void GUISystem::OnMouseMove(float x, float y) {
    m_mouseX = x;
    m_mouseY = y;
}

void GUISystem::OnMousePress(float x, float y, int button) {
    for (auto& [name, screen] : m_screens) {
        if (!screen.active) continue;
        GUIElement* hit = HitTest(screen, x, y);
        if (hit && button == 0) {
            // Try to fire onClick for buttons
            auto* btn = dynamic_cast<GUIButton*>(hit);
            if (btn && btn->enabled && btn->onClick) {
                btn->onClick();
            }
        }
    }
}

void GUISystem::OnMouseRelease(float /*x*/, float /*y*/, int /*button*/) {
    // Future: drag-end, scroll, etc.
}

// ── Serialisation ──────────────────────────────────────────────

bool GUISystem::SaveScreenToJSON(const std::string& screenName,
                                  const std::string& filePath) const {
    auto it = m_screens.find(screenName);
    if (it == m_screens.end()) return false;
    std::ofstream f(filePath);
    if (!f.is_open()) return false;
    f << "{\n  \"screen\": \"" << screenName << "\",\n";
    f << "  \"elements\": [\n";
    bool first = true;
    for (auto& e : it->second.elements) {
        if (!e) continue;
        if (!first) f << ",\n";
        first = false;
        f << "    {\"id\":\"" << e->id << "\","
          << "\"type\":\"" << e->Type() << "\","
          << "\"x\":" << e->rect.x << ","
          << "\"y\":" << e->rect.y << ","
          << "\"w\":" << e->rect.w << ","
          << "\"h\":" << e->rect.h << "}";
    }
    f << "\n  ]\n}\n";
    return true;
}

bool GUISystem::LoadScreenFromJSON(const std::string& filePath) {
    std::ifstream f(filePath);
    if (!f.is_open()) return false;
    // Minimal JSON parsing — production use should use a proper JSON lib
    std::string line;
    std::string screenName;
    while (std::getline(f, line)) {
        if (line.find("\"screen\"") != std::string::npos) {
            auto pos1 = line.find(':', 0);
            auto pos2 = line.find('"', pos1 + 1);
            auto pos3 = line.find('"', pos2 + 1);
            if (pos2 != std::string::npos && pos3 != std::string::npos)
                screenName = line.substr(pos2 + 1, pos3 - pos2 - 1);
        }
    }
    if (!screenName.empty()) CreateScreen(screenName);
    return !screenName.empty();
}

// ── Private helpers ────────────────────────────────────────────

GUIElement* GUISystem::HitTest(GUIScreen& screen, float x, float y) {
    // Reverse iterate so top-most elements are tested first
    for (auto it = screen.elements.rbegin(); it != screen.elements.rend(); ++it) {
        auto& e = *it;
        if (!e || !e->visible || !e->enabled) continue;
        if (x >= e->rect.x && x <= e->rect.x + e->rect.w &&
            y >= e->rect.y && y <= e->rect.y + e->rect.h) {
            return e.get();
        }
    }
    return nullptr;
}

} // namespace UI
