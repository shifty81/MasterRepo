#include "Engine/Debug/Overlay/DebugOverlay/DebugOverlay.h"
#include <algorithm>
#include <deque>
#include <unordered_map>
#include <vector>

namespace Engine {

struct WatchEntry {
    std::string name;
    std::function<std::string()> valueFn;
};

struct GraphEntry {
    std::string       name;
    uint32_t          historyLen{120};
    std::deque<float> history;
};

struct DebugOverlay::Impl {
    bool    visible{true};
    std::vector<OverlayPrimitive>           prims;
    std::vector<WatchEntry>                 watches;
    std::unordered_map<std::string,GraphEntry> graphs;

    void EmitGraphPanelPrims(float x, float y, float w, float h);
};

DebugOverlay::DebugOverlay(): m_impl(new Impl){}
DebugOverlay::~DebugOverlay(){ Shutdown(); delete m_impl; }
void DebugOverlay::Init(){}
void DebugOverlay::Shutdown(){ m_impl->prims.clear(); m_impl->watches.clear(); m_impl->graphs.clear(); }
void DebugOverlay::Reset(){ Shutdown(); m_impl->visible=true; }

void DebugOverlay::Tick(float dt){
    for(auto it=m_impl->prims.begin();it!=m_impl->prims.end();){
        if(it->duration<0){ ++it; continue; } // permanent
        it->duration-=dt;
        if(it->duration<=0) it=m_impl->prims.erase(it);
        else ++it;
    }
    // Emit watch primitives (ephemeral, added each frame during Render)
}

static OverlayPrimitive MakePrim(OverlayPrimType t){ OverlayPrimitive p; p.type=t; return p; }

void DebugOverlay::DrawText(float x, float y, const std::string& text,
                             OverlayColor col, float dur){
    auto p=MakePrim(OverlayPrimType::Text);
    p.x=x; p.y=y; p.text=text; p.colour=col; p.duration=dur;
    m_impl->prims.push_back(p);
}
void DebugOverlay::DrawLine(float x0,float y0,float x1,float y1,OverlayColor col,float dur){
    auto p=MakePrim(OverlayPrimType::Line);
    p.x=x0; p.y=y0; p.x1=x1; p.y1=y1; p.colour=col; p.duration=dur;
    m_impl->prims.push_back(p);
}
void DebugOverlay::DrawRect(float x,float y,float w,float h,
                             OverlayColor col,bool filled,float dur){
    auto p=MakePrim(OverlayPrimType::Rect);
    p.x=x; p.y=y; p.w=w; p.h=h; p.colour=col; p.filled=filled; p.duration=dur;
    m_impl->prims.push_back(p);
}
void DebugOverlay::DrawCircle(float cx,float cy,float r,OverlayColor col,uint32_t segs,float dur){
    auto p=MakePrim(OverlayPrimType::Circle);
    p.x=cx; p.y=cy; p.r=r; p.colour=col; p.segments=segs; p.duration=dur;
    m_impl->prims.push_back(p);
}
void DebugOverlay::DrawBar(float x,float y,float w,float h,float val,float maxVal,
                            OverlayColor col,const std::string& label,float dur){
    auto p=MakePrim(OverlayPrimType::Bar);
    p.x=x; p.y=y; p.w=w; p.h=h; p.value=val; p.maxVal=maxVal;
    p.colour=col; p.label=label; p.duration=dur;
    m_impl->prims.push_back(p);
}

void DebugOverlay::AddWatch(const std::string& name, std::function<std::string()> fn){
    for(auto& w:m_impl->watches) if(w.name==name){ w.valueFn=fn; return; }
    WatchEntry we; we.name=name; we.valueFn=fn;
    m_impl->watches.push_back(we);
}
void DebugOverlay::RemoveWatch(const std::string& name){
    m_impl->watches.erase(std::remove_if(m_impl->watches.begin(),m_impl->watches.end(),
        [&name](const WatchEntry& w){return w.name==name;}),m_impl->watches.end());
}

void DebugOverlay::AddGraph(const std::string& name, uint32_t historyLen){
    auto& g=m_impl->graphs[name]; g.name=name; g.historyLen=historyLen;
}
void DebugOverlay::PushGraphValue(const std::string& name, float value){
    auto it=m_impl->graphs.find(name); if(it==m_impl->graphs.end()) return;
    auto& g=it->second;
    g.history.push_back(value);
    while(g.history.size()>g.historyLen) g.history.pop_front();
}
void DebugOverlay::DrawGraphPanel(float x, float y, float w, float h){
    // Adds placeholder primitives (actual rendering done by caller via Render cb)
    auto p=MakePrim(OverlayPrimType::Graph);
    p.x=x; p.y=y; p.w=w; p.h=h; p.duration=0;
    m_impl->prims.push_back(p);
}

void DebugOverlay::Clear(){ m_impl->prims.clear(); }
void DebugOverlay::SetVisible(bool on){ m_impl->visible=on; }
bool DebugOverlay::IsVisible() const { return m_impl->visible; }

void DebugOverlay::Render(std::function<void(const OverlayPrimitive&)> drawCb) const {
    if(!m_impl->visible||!drawCb) return;
    for(auto& p:m_impl->prims) drawCb(p);
    // Watches as Text primitives
    float wy=10;
    for(auto& w:m_impl->watches){
        if(!w.valueFn) continue;
        OverlayPrimitive p=MakePrim(OverlayPrimType::Text);
        p.x=10; p.y=wy; wy+=14;
        p.text=w.name+": "+w.valueFn();
        p.colour={1,1,0,1}; p.duration=0;
        drawCb(p);
    }
}

} // namespace Engine
