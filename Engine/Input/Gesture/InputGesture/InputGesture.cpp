#include "Engine/Input/Gesture/InputGesture/InputGesture.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct Touch { uint32_t id; float x,y,startX,startY,startTime,prevX,prevY; bool active{false}; };

struct InputGesture::Impl {
    GestureConfig cfg;
    std::unordered_map<uint32_t,Touch> touches;
    std::function<void(const GestureEvent&)> cb;
    float lastTapTime{-1.f};
    float time{0.f};
    bool inProgress{false};

    void Emit(const GestureEvent& e){ if(cb) cb(e); }

    float PinchDist() const {
        if(touches.size()<2) return 0;
        auto it=touches.begin(); auto a=it->second; ++it; auto b=it->second;
        float dx=a.x-b.x, dy=a.y-b.y; return std::sqrt(dx*dx+dy*dy);
    }
    float PinchAngle() const {
        if(touches.size()<2) return 0;
        auto it=touches.begin(); auto a=it->second; ++it; auto b=it->second;
        return std::atan2(b.y-a.y,b.x-a.x);
    }

    float prevPinchDist{0}, prevPinchAngle{0};
};

InputGesture::InputGesture() : m_impl(new Impl){}
InputGesture::~InputGesture(){ Shutdown(); delete m_impl; }
void InputGesture::Init(const GestureConfig& cfg){ m_impl->cfg=cfg; }
void InputGesture::Shutdown(){ m_impl->touches.clear(); }

void InputGesture::BeginTouch(uint32_t id, float x, float y, float t){
    m_impl->inProgress=true;
    Touch& tr=m_impl->touches[id];
    tr={id,x,y,x,y,t,x,y,true};
    if(m_impl->touches.size()==2){
        m_impl->prevPinchDist =m_impl->PinchDist();
        m_impl->prevPinchAngle=m_impl->PinchAngle();
    }
}
void InputGesture::MoveTouch(uint32_t id, float x, float y, float /*t*/){
    auto it=m_impl->touches.find(id); if(it==m_impl->touches.end()) return;
    auto& tr=it->second;
    float dx=x-tr.prevX, dy=y-tr.prevY;
    tr.prevX=tr.x; tr.prevY=tr.y; tr.x=x; tr.y=y;

    if(m_impl->touches.size()==2){
        float d=m_impl->PinchDist();
        if(m_impl->prevPinchDist>0 && std::abs(d-m_impl->prevPinchDist)>m_impl->cfg.pinchMinScale){
            GestureEvent e; e.type=GestureType::Pinch; e.scale=d/m_impl->prevPinchDist; e.touchCount=2; m_impl->Emit(e);
        }
        float a=m_impl->PinchAngle();
        float da=a-m_impl->prevPinchAngle;
        if(std::abs(da)>0.02f){ GestureEvent e; e.type=GestureType::Rotate; e.angle=da; e.touchCount=2; m_impl->Emit(e); }
        m_impl->prevPinchDist=d; m_impl->prevPinchAngle=a;
    } else if(m_impl->touches.size()==1 && (std::abs(dx)>1||std::abs(dy)>1)){
        GestureEvent e; e.type=GestureType::Pan; e.x=x; e.y=y; e.dx=dx; e.dy=dy; m_impl->Emit(e);
    }
}
void InputGesture::EndTouch(uint32_t id, float x, float y, float t){
    auto it=m_impl->touches.find(id); if(it==m_impl->touches.end()) return;
    auto& tr=it->second;
    float dx=x-tr.startX, dy=y-tr.startY;
    float dist=std::sqrt(dx*dx+dy*dy);
    float dur=t-tr.startTime;

    if(m_impl->touches.size()==1){
        if(dur>=m_impl->cfg.longPressMinDuration&&dist<m_impl->cfg.swipeMinDistance){
            GestureEvent e; e.type=GestureType::LongPress; e.x=x; e.y=y; e.duration=dur; m_impl->Emit(e);
        } else if(dist>=m_impl->cfg.swipeMinDistance){
            GestureEvent e;
            float adx=std::abs(dx),ady=std::abs(dy);
            if(adx>ady) e.type=(dx>0)?GestureType::SwipeRight:GestureType::SwipeLeft;
            else        e.type=(dy>0)?GestureType::SwipeDown :GestureType::SwipeUp;
            e.dx=dx; e.dy=dy; m_impl->Emit(e);
        } else if(dur<m_impl->cfg.tapMaxDuration){
            float dt2=t-m_impl->lastTapTime;
            if(m_impl->lastTapTime>0&&dt2<m_impl->cfg.doubleTapMaxInterval){
                GestureEvent e; e.type=GestureType::DoubleTap; e.x=x; e.y=y; m_impl->Emit(e);
                m_impl->lastTapTime=-1;
            } else {
                GestureEvent e; e.type=GestureType::Tap; e.x=x; e.y=y; e.duration=dur; m_impl->Emit(e);
                m_impl->lastTapTime=t;
            }
        }
    }
    m_impl->touches.erase(id);
    if(m_impl->touches.empty()) m_impl->inProgress=false;
}
void InputGesture::CancelGesture(){ m_impl->touches.clear(); m_impl->inProgress=false; }
void InputGesture::SetConfig(const GestureConfig& c){ m_impl->cfg=c; }
const GestureConfig& InputGesture::GetConfig() const { return m_impl->cfg; }
void InputGesture::SetOnGesture(std::function<void(const GestureEvent&)> cb){ m_impl->cb=cb; }
bool InputGesture::IsGestureInProgress() const { return m_impl->inProgress; }
uint32_t InputGesture::ActiveTouchCount() const { return (uint32_t)m_impl->touches.size(); }
void InputGesture::Tick(float dt){ m_impl->time+=dt; }

} // namespace Engine
