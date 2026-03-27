#include "Runtime/UI/UISystem/UISystem.h"
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct Widget {
    UIWidgetDesc desc;
    uint32_t     id{0};
    std::vector<uint32_t> children;
    float  computedX{0}, computedY{0};
    bool   hovered{false};
    bool   pressed{false};

    std::function<void()>               onClickCb;
    std::function<void(float)>          onValueCb;
    std::function<void(std::string)>    onTextCb;
};

struct UISystem::Impl {
    std::vector<Widget>  widgets;
    uint32_t             nextId{1};
    uint32_t             screenW{1280}, screenH{720};
    float                mouseX{0}, mouseY{0};
    bool                 mouseDown[3]{};
    bool                 prevMouseDown[3]{};
    uint32_t             focusedWidget{0};

    Widget* Find(uint32_t id) {
        for (auto& w : widgets) if (w.id==id) return &w;
        return nullptr;
    }
    const Widget* Find(uint32_t id) const {
        for (auto& w : widgets) if (w.id==id) return &w;
        return nullptr;
    }
    uint32_t FindByStringId(const std::string& sid) const {
        for (auto& w : widgets) if (w.desc.id==sid) return w.id;
        return 0;
    }

    bool PointInRect(float x, float y, const Widget& w) const {
        return x>=w.computedX && x<=(w.computedX+w.desc.rect.w) &&
               y>=w.computedY && y<=(w.computedY+w.desc.rect.h);
    }
};

UISystem::UISystem()  : m_impl(new Impl) {}
UISystem::~UISystem() { Shutdown(); delete m_impl; }

void UISystem::Init(uint32_t w, uint32_t h)  { m_impl->screenW=w; m_impl->screenH=h; }
void UISystem::Shutdown()                     { m_impl->widgets.clear(); }
void UISystem::SetScreenSize(uint32_t w, uint32_t h) { m_impl->screenW=w; m_impl->screenH=h; }

uint32_t UISystem::Create(const UIWidgetDesc& desc)
{
    Widget w;
    w.id   = m_impl->nextId++;
    w.desc = desc;
    w.computedX = desc.rect.x;
    w.computedY = desc.rect.y;
    if (desc.parentId) {
        auto* parent = m_impl->Find(desc.parentId);
        if (parent) {
            parent->children.push_back(w.id);
            w.computedX += parent->computedX;
            w.computedY += parent->computedY;
        }
    }
    m_impl->widgets.push_back(w);
    return w.id;
}

uint32_t UISystem::CreatePanel(const std::string& id, UIRect rect, uint32_t parentId)
{
    UIWidgetDesc d; d.id=id; d.type=WidgetType::Panel; d.rect=rect; d.parentId=parentId;
    return Create(d);
}

uint32_t UISystem::CreateLabel(const std::string& id, const std::string& text,
                                UIRect rect, uint32_t parentId)
{
    UIWidgetDesc d; d.id=id; d.type=WidgetType::Label; d.text=text; d.rect=rect; d.parentId=parentId;
    return Create(d);
}

uint32_t UISystem::CreateButton(const std::string& id, const std::string& label,
                                  UIRect rect, uint32_t parentId)
{
    UIWidgetDesc d; d.id=id; d.type=WidgetType::Button; d.text=label; d.rect=rect; d.parentId=parentId;
    return Create(d);
}

uint32_t UISystem::CreateSlider(const std::string& id, UIRect rect,
                                  float minV, float maxV, float val, uint32_t parentId)
{
    UIWidgetDesc d; d.id=id; d.type=WidgetType::Slider; d.rect=rect;
    d.minValue=minV; d.maxValue=maxV; d.value=val; d.parentId=parentId;
    return Create(d);
}

uint32_t UISystem::CreateCheckbox(const std::string& id, const std::string& label,
                                    UIRect rect, bool checked, uint32_t parentId)
{
    UIWidgetDesc d; d.id=id; d.type=WidgetType::Checkbox; d.text=label;
    d.rect=rect; d.value=checked?1.f:0.f; d.parentId=parentId;
    return Create(d);
}

uint32_t UISystem::CreateTextInput(const std::string& id, const std::string& placeholder,
                                     UIRect rect, uint32_t parentId)
{
    UIWidgetDesc d; d.id=id; d.type=WidgetType::TextInput; d.text=placeholder;
    d.rect=rect; d.parentId=parentId;
    return Create(d);
}

void UISystem::Destroy(uint32_t id)
{
    auto& v = m_impl->widgets;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& w){ return w.id==id; }), v.end());
}

void UISystem::DestroyAll() { m_impl->widgets.clear(); }

void UISystem::SetText(uint32_t id, const std::string& text)
{
    auto* w = m_impl->Find(id); if(w) w->desc.text=text;
}
void UISystem::SetVisible(uint32_t id, bool v)
{
    auto* w = m_impl->Find(id); if(w) w->desc.visible=v;
}
void UISystem::SetEnabled(uint32_t id, bool v)
{
    auto* w = m_impl->Find(id); if(w) w->desc.enabled=v;
}
void UISystem::SetValue(uint32_t id, float v)
{
    auto* w = m_impl->Find(id); if(w){ w->desc.value=v; if(w->onValueCb) w->onValueCb(v); }
}
void UISystem::SetRect(uint32_t id, UIRect r)
{
    auto* w = m_impl->Find(id); if(w){ w->desc.rect=r; w->computedX=r.x; w->computedY=r.y; }
}
void UISystem::SetStyle(uint32_t id, const UIStyle& s)
{
    auto* w = m_impl->Find(id); if(w) w->desc.style=s;
}

float UISystem::GetValue(uint32_t id) const
{
    const auto* w = m_impl->Find(id); return w ? w->desc.value : 0.f;
}
std::string UISystem::GetText(uint32_t id) const
{
    const auto* w = m_impl->Find(id); return w ? w->desc.text : "";
}
bool UISystem::IsVisible(uint32_t id) const
{
    const auto* w = m_impl->Find(id); return w && w->desc.visible;
}
uint32_t UISystem::FindById(const std::string& id) const
{
    return m_impl->FindByStringId(id);
}

void UISystem::OnMouseMove(float x, float y)
{
    m_impl->mouseX=x; m_impl->mouseY=y;
    for (auto& w : m_impl->widgets)
        w.hovered = w.desc.visible && m_impl->PointInRect(x, y, w);
}

void UISystem::OnMouseButton(int button, bool pressed)
{
    if (button<0||button>2) return;
    m_impl->prevMouseDown[button] = m_impl->mouseDown[button];
    m_impl->mouseDown[button] = pressed;

    if (!pressed && m_impl->prevMouseDown[button]) {
        // Click
        for (auto it = m_impl->widgets.rbegin(); it!=m_impl->widgets.rend(); ++it) {
            auto& w = *it;
            if (!w.desc.visible || !w.desc.enabled) continue;
            if (w.hovered) {
                if (w.desc.type==WidgetType::Button && w.onClickCb) w.onClickCb();
                if (w.desc.type==WidgetType::Checkbox) {
                    w.desc.value = w.desc.value>0.5f ? 0.f : 1.f;
                    if (w.onValueCb) w.onValueCb(w.desc.value);
                }
                if (w.desc.type==WidgetType::Slider && button==0) {
                    float rel = (m_impl->mouseX - w.computedX) / w.desc.rect.w;
                    float val = w.desc.minValue + rel*(w.desc.maxValue-w.desc.minValue);
                    val = std::max(w.desc.minValue, std::min(w.desc.maxValue, val));
                    w.desc.value = val;
                    if (w.onValueCb) w.onValueCb(val);
                }
                if (w.desc.type==WidgetType::TextInput) m_impl->focusedWidget = w.id;
                break;
            }
        }
    }
}

void UISystem::OnChar(uint32_t cp)
{
    auto* w = m_impl->Find(m_impl->focusedWidget);
    if (!w || w->desc.type!=WidgetType::TextInput) return;
    if (cp==8 && !w->desc.text.empty()) w->desc.text.pop_back();
    else if (cp>=32 && cp<128) w->desc.text += (char)cp;
    if (w->onTextCb) w->onTextCb(w->desc.text);
}

void UISystem::Tick(float /*dt*/) {}

void UISystem::SetOnClick(uint32_t id, std::function<void()> cb)
{ auto* w=m_impl->Find(id); if(w) w->onClickCb=cb; }
void UISystem::SetOnValueChange(uint32_t id, std::function<void(float)> cb)
{ auto* w=m_impl->Find(id); if(w) w->onValueCb=cb; }
void UISystem::SetOnTextChanged(uint32_t id, std::function<void(const std::string&)> cb)
{ auto* w=m_impl->Find(id); if(w) w->onTextCb=cb; }

void UISystem::Render(DrawRectFn drawRect, DrawTextFn drawText)
{
    // Sort by z-order
    std::vector<Widget*> sorted;
    sorted.reserve(m_impl->widgets.size());
    for (auto& w : m_impl->widgets) sorted.push_back(&w);
    std::sort(sorted.begin(),sorted.end(),[](auto* a,auto* b){ return a->desc.zOrder<b->desc.zOrder; });

    for (auto* wp : sorted) {
        if (!wp->desc.visible) continue;
        UIRect r{wp->computedX, wp->computedY, wp->desc.rect.w, wp->desc.rect.h};
        const auto& s = wp->desc.style;
        float col[4]; for(int i=0;i<4;i++) col[i]=s.bgColour[i];
        if (wp->hovered && wp->desc.type==WidgetType::Button) col[0]=std::min(1.f,col[0]*1.2f);
        if (drawRect) drawRect(r, col, s.borderRadius);
        if (!wp->desc.text.empty() && drawText)
            drawText(wp->desc.text, wp->computedX+4, wp->computedY+4,
                     s.fontSize, s.fgColour);
    }
}

bool UISystem::LoadLayout(const std::string& path)
{
    std::ifstream f(path); return f.good();
}

} // namespace Runtime
