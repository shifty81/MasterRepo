#include "UI/Widgets/Widget.h"
#include <atomic>

namespace UI {

// ──────────────────────────────────────────────────────────────
// Widget base
// ──────────────────────────────────────────────────────────────

static std::atomic<uint32_t> s_nextWidgetID{1};

/*static*/ uint32_t Widget::NextID() {
    return s_nextWidgetID.fetch_add(1, std::memory_order_relaxed);
}

Widget::Widget() : m_id(NextID()) {}

void Widget::AddChild(Widget* child) {
    if (!child) return;
    child->m_parent = this;
    // Transfer ownership — caller must pass a raw pointer obtained via release()
    // or we wrap a non-owning view; convention: AddChild takes OWNERSHIP via unique_ptr
    m_children.emplace_back(child);
}

void Widget::RemoveChild(uint32_t id) {
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [id](const std::unique_ptr<Widget>& w) { return w->m_id == id; });
    if (it != m_children.end()) {
        (*it)->m_parent = nullptr;
        m_children.erase(it);
    }
}

Widget* Widget::GetChild(uint32_t id) const {
    for (const auto& c : m_children)
        if (c->m_id == id) return c.get();
    return nullptr;
}

Widget* Widget::HitTest(float x, float y) {
    if (!m_state.visible || !m_bounds.Contains(x, y))
        return nullptr;
    // Depth-first: deepest child wins
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        Widget* hit = (*it)->HitTest(x, y);
        if (hit) return hit;
    }
    return this;
}

// ──────────────────────────────────────────────────────────────
// Button
// ──────────────────────────────────────────────────────────────

bool Button::OnEvent(WidgetEvent evt, float x, float y) {
    if (!m_state.enabled) return false;
    switch (evt) {
        case WidgetEvent::Hovered:
            m_state.hovered = true;
            return true;
        case WidgetEvent::Pressed:
            m_state.pressed = true;
            return true;
        case WidgetEvent::Released:
            m_state.pressed = false;
            return true;
        case WidgetEvent::Clicked:
            if (m_bounds.Contains(x, y) && m_onClick)
                m_onClick();
            return true;
        case WidgetEvent::FocusGained:
            m_state.focused = true;
            return true;
        case WidgetEvent::FocusLost:
            m_state.focused = false;
            return true;
        default:
            return false;
    }
}

// ──────────────────────────────────────────────────────────────
// Label
// ──────────────────────────────────────────────────────────────

bool Label::OnEvent(WidgetEvent /*evt*/, float /*x*/, float /*y*/) {
    return false; // labels are passive
}

// ──────────────────────────────────────────────────────────────
// TextInput
// ──────────────────────────────────────────────────────────────

bool TextInput::OnEvent(WidgetEvent evt, float x, float y) {
    if (!m_state.enabled) return false;
    switch (evt) {
        case WidgetEvent::FocusGained:
            m_state.focused = true;
            return true;
        case WidgetEvent::FocusLost:
            m_state.focused = false;
            return true;
        case WidgetEvent::TextChanged:
            if (m_onTextChanged)
                m_onTextChanged(m_value);
            return true;
        case WidgetEvent::Clicked:
            m_state.focused = true;
            return true;
        default:
            return false;
    }
}

// ──────────────────────────────────────────────────────────────
// Checkbox
// ──────────────────────────────────────────────────────────────

bool Checkbox::OnEvent(WidgetEvent evt, float x, float y) {
    if (!m_state.enabled) return false;
    if (evt == WidgetEvent::Clicked && m_bounds.Contains(x, y)) {
        m_checked = !m_checked;
        if (m_onChanged) m_onChanged(m_checked);
        return true;
    }
    return false;
}

// ──────────────────────────────────────────────────────────────
// Slider
// ──────────────────────────────────────────────────────────────

bool Slider::OnEvent(WidgetEvent evt, float x, float /*y*/) {
    if (!m_state.enabled) return false;
    if ((evt == WidgetEvent::Pressed || evt == WidgetEvent::ValueChanged)
        && m_bounds.w > 0.f)
    {
        float t = std::clamp((x - m_bounds.x) / m_bounds.w, 0.f, 1.f);
        float newVal = m_min + t * (m_max - m_min);
        if (newVal != m_value) {
            m_value = newVal;
            if (m_onChanged) m_onChanged(m_value);
        }
        m_state.pressed = (evt == WidgetEvent::Pressed);
        return true;
    }
    if (evt == WidgetEvent::Released) {
        m_state.pressed = false;
        return true;
    }
    return false;
}

// ──────────────────────────────────────────────────────────────
// ProgressBar
// ──────────────────────────────────────────────────────────────

bool ProgressBar::OnEvent(WidgetEvent /*evt*/, float /*x*/, float /*y*/) {
    return false; // read-only display widget
}

} // namespace UI
