#include "Engine/Sim/WaterRipple/WaterRipple/WaterRipple.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

struct WaterRipple::Impl {
    uint32_t gw{0}, gh{0};
    float cellSize{0.25f};
    float damping{0.99f};
    float speed{8.f};
    WaterBoundary boundary{WaterBoundary::Absorb};

    std::vector<float> cur, prev, next;

    int Idx(int x, int y) const { return y*(int)gw+x; }
    float SafeGet(const std::vector<float>& buf, int x, int y) const {
        switch(boundary){
            case WaterBoundary::Wrap:
                x=(x+(int)gw)%(int)gw; y=(y+(int)gh)%(int)gh; break;
            case WaterBoundary::Clamp:
                x=std::max(0,std::min((int)gw-1,x)); y=std::max(0,std::min((int)gh-1,y)); break;
            case WaterBoundary::Absorb:
                if(x<0||y<0||(uint32_t)x>=gw||(uint32_t)y>=gh) return 0.f;
                break;
        }
        return buf[Idx(x,y)];
    }

    int WorldToCell(float w) const { return (int)(w/cellSize); }
};

WaterRipple::WaterRipple(): m_impl(new Impl){}
WaterRipple::~WaterRipple(){ Shutdown(); delete m_impl; }

void WaterRipple::Init(uint32_t gw, uint32_t gh, float cs){
    m_impl->gw=gw; m_impl->gh=gh; m_impl->cellSize=cs;
    size_t n=gw*gh;
    m_impl->cur.assign(n,0.f);
    m_impl->prev.assign(n,0.f);
    m_impl->next.assign(n,0.f);
}
void WaterRipple::Shutdown(){ m_impl->cur.clear(); m_impl->prev.clear(); m_impl->next.clear(); }
void WaterRipple::Reset(){ size_t n=m_impl->gw*m_impl->gh; m_impl->cur.assign(n,0); m_impl->prev.assign(n,0); m_impl->next.assign(n,0); }

void WaterRipple::Update(float dt){
    if(!m_impl->gw) return;
    float c=m_impl->speed; float cs=m_impl->cellSize;
    float k=(c*dt/cs); if(k>0.5f) k=0.5f; // stability clamp
    float k2=k*k;
    for(int y=0;y<(int)m_impl->gh;y++) for(int x=0;x<(int)m_impl->gw;x++){
        int i=m_impl->Idx(x,y);
        float lap = m_impl->SafeGet(m_impl->cur,x-1,y) + m_impl->SafeGet(m_impl->cur,x+1,y)
                  + m_impl->SafeGet(m_impl->cur,x,y-1) + m_impl->SafeGet(m_impl->cur,x,y+1)
                  - 4.f*m_impl->cur[i];
        m_impl->next[i] = (2.f*m_impl->cur[i] - m_impl->prev[i] + k2*lap) * m_impl->damping;
    }
    std::swap(m_impl->prev, m_impl->cur);
    std::swap(m_impl->cur,  m_impl->next);
}

void WaterRipple::Splash(float wx, float wy, float force){
    int cx=m_impl->WorldToCell(wx), cy=m_impl->WorldToCell(wy);
    if(cx<0||cy<0||(uint32_t)cx>=m_impl->gw||(uint32_t)cy>=m_impl->gh) return;
    m_impl->cur[m_impl->Idx(cx,cy)] += force;
    m_impl->prev[m_impl->Idx(cx,cy)] += force;
}

float WaterRipple::GetHeight(float wx, float wy) const {
    int cx=m_impl->WorldToCell(wx), cy=m_impl->WorldToCell(wy);
    if(cx<0||cy<0||(uint32_t)cx>=m_impl->gw||(uint32_t)cy>=m_impl->gh) return 0.f;
    return m_impl->cur[m_impl->Idx(cx,cy)];
}
const float* WaterRipple::GetHeightBuffer() const { return m_impl->cur.data(); }

void WaterRipple::GenerateNormals(float* out) const {
    float inv2cs=1.f/(2.f*m_impl->cellSize);
    for(int y=0;y<(int)m_impl->gh;y++) for(int x=0;x<(int)m_impl->gw;x++){
        float hL=m_impl->SafeGet(m_impl->cur,x-1,y);
        float hR=m_impl->SafeGet(m_impl->cur,x+1,y);
        float hD=m_impl->SafeGet(m_impl->cur,x,y-1);
        float hU=m_impl->SafeGet(m_impl->cur,x,y+1);
        float nx=(hL-hR)*inv2cs, ny=1.f, nz=(hD-hU)*inv2cs;
        float len=std::sqrt(nx*nx+ny*ny+nz*nz); if(len<1e-6f)len=1.f;
        int i=m_impl->Idx(x,y);
        out[i*3+0]=nx/len; out[i*3+1]=ny/len; out[i*3+2]=nz/len;
    }
}

void WaterRipple::SetDamping     (float d){ m_impl->damping=d; }
void WaterRipple::SetSpeed       (float c){ m_impl->speed=c; }
void WaterRipple::SetBoundaryMode(WaterBoundary m){ m_impl->boundary=m; }
uint32_t WaterRipple::GridWidth()  const { return m_impl->gw; }
uint32_t WaterRipple::GridHeight() const { return m_impl->gh; }
float    WaterRipple::CellSize()   const { return m_impl->cellSize; }

} // namespace Engine
