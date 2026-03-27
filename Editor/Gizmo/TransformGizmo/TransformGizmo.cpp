#include "Editor/Gizmo/TransformGizmo/TransformGizmo.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Editor {

static void NormalizeQ(float q[4]){ float l=std::sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3])+1e-9f; for(int i=0;i<4;i++) q[i]/=l; }

struct TransformGizmo::Impl {
    GizmoMode  mode {GizmoMode::Translate};
    GizmoSpace space{GizmoSpace::World};
    float pos[3]{}, rot[4]{0,0,0,1}, scale[3]{1,1,1};
    float snapTrans{0.f}, snapRot{0.f}, snapScale{0.f};
    bool  snapEnabled{false};
    bool  dragging{false};
    GizmoAxis dragAxis{GizmoAxis::None};
    float dragStart[2]{};
    float gizmoScale{1.f};
    uint32_t screenW{1920}, screenH{1080};
    GizmoDelta lastDelta;
    std::vector<GizmoSegment> drawData;

    void BuildDrawData(){
        drawData.clear();
        static const float colours[3][4]={{1,0,0,1},{0,1,0,1},{0,0,1,1}};
        static const float dirs[3][3]={{1,0,0},{0,1,0},{0,0,1}};
        for(int a=0;a<3;a++){
            GizmoSegment seg;
            for(int i=0;i<3;i++){seg.start[i]=pos[i]; seg.end[i]=pos[i]+dirs[a][i]*gizmoScale;}
            for(int i=0;i<4;i++) seg.colour[i]=colours[a][i];
            seg.thickness=2.f; drawData.push_back(seg);
        }
    }
};

TransformGizmo::TransformGizmo()  : m_impl(new Impl){ m_impl->BuildDrawData(); }
TransformGizmo::~TransformGizmo() { Shutdown(); delete m_impl; }
void TransformGizmo::Init()     { m_impl->BuildDrawData(); }
void TransformGizmo::Shutdown() {}

void       TransformGizmo::SetMode (GizmoMode  m){ m_impl->mode=m; m_impl->BuildDrawData(); }
GizmoMode  TransformGizmo::GetMode ()  const { return m_impl->mode; }
void       TransformGizmo::SetSpace(GizmoSpace s){ m_impl->space=s; }
GizmoSpace TransformGizmo::GetSpace()  const { return m_impl->space; }

void TransformGizmo::SetTransform(const float p[3], const float q[4], const float s[3]){
    if(p) for(int i=0;i<3;i++) m_impl->pos[i]=p[i];
    if(q) for(int i=0;i<4;i++) m_impl->rot[i]=q[i];
    if(s) for(int i=0;i<3;i++) m_impl->scale[i]=s[i];
    m_impl->BuildDrawData();
}
void TransformGizmo::GetPosition(float out[3]) const { for(int i=0;i<3;i++) out[i]=m_impl->pos[i]; }
void TransformGizmo::GetRotation(float out[4]) const { for(int i=0;i<4;i++) out[i]=m_impl->rot[i]; }
void TransformGizmo::GetScale   (float out[3]) const { for(int i=0;i<3;i++) out[i]=m_impl->scale[i]; }

void TransformGizmo::SetSnapTranslate(float g){ m_impl->snapTrans=g; }
void TransformGizmo::SetSnapRotate   (float d){ m_impl->snapRot=d; }
void TransformGizmo::SetSnapScale    (float s){ m_impl->snapScale=s; }
void TransformGizmo::SetSnapEnabled  (bool e){ m_impl->snapEnabled=e; }

GizmoAxis TransformGizmo::GetHoveredAxis(const float*, const float*) const {
    return GizmoAxis::None; // placeholder
}

void TransformGizmo::BeginDrag(GizmoAxis axis, const float screenPos[2]){
    m_impl->dragging=true; m_impl->dragAxis=axis;
    m_impl->dragStart[0]=screenPos[0]; m_impl->dragStart[1]=screenPos[1];
}

void TransformGizmo::Drag(const float screenPos[2], const float*){
    if(!m_impl->dragging) return;
    float dx=(screenPos[0]-m_impl->dragStart[0])/(float)m_impl->screenW;
    float dy=(screenPos[1]-m_impl->dragStart[1])/(float)m_impl->screenH;
    float delta=dx*m_impl->gizmoScale*2.f;
    if(m_impl->mode==GizmoMode::Translate){
        int ax=(m_impl->dragAxis==GizmoAxis::X)?0:(m_impl->dragAxis==GizmoAxis::Y)?1:2;
        if(m_impl->snapEnabled&&m_impl->snapTrans>0.f)
            delta=std::round(delta/m_impl->snapTrans)*m_impl->snapTrans;
        m_impl->pos[ax]+=delta;
        m_impl->lastDelta.translate[ax]+=delta;
    } else if(m_impl->mode==GizmoMode::Scale){
        for(int i=0;i<3;i++) m_impl->scale[i]+=dy;
    }
    m_impl->dragStart[0]=screenPos[0]; m_impl->dragStart[1]=screenPos[1];
    m_impl->BuildDrawData();
}

GizmoDelta TransformGizmo::EndDrag(){
    m_impl->dragging=false; m_impl->lastDelta.changed=true;
    GizmoDelta d=m_impl->lastDelta;
    m_impl->lastDelta={};
    return d;
}
bool TransformGizmo::IsDragging() const { return m_impl->dragging; }

bool TransformGizmo::ManipulateTransform(const float*, const float*,
                                           const float mousePos[2], bool mouseDown){
    if(!mouseDown&&m_impl->dragging){ EndDrag(); return true; }
    return false;
}

const std::vector<GizmoSegment>& TransformGizmo::GetDrawData() const { return m_impl->drawData; }
void TransformGizmo::SetScreenSize(uint32_t w, uint32_t h){ m_impl->screenW=w; m_impl->screenH=h; }
void TransformGizmo::SetGizmoScale(float s){ m_impl->gizmoScale=s; m_impl->BuildDrawData(); }

} // namespace Editor
