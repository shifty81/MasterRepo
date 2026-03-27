#include "Engine/Render/Visibility/VisibilitySystem/VisibilitySystem.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <unordered_map>
#include <vector>

namespace Engine {

struct Sector {
    int32_t id;
    float mn[3]{}, mx[3]{};
    std::vector<int32_t> portals;
};

struct Portal {
    int32_t id, sA, sB;
    float verts[4][3]{};
    bool open{true};
};

struct VisibilitySystem::Impl {
    std::unordered_map<int32_t,Sector> sectors;
    std::unordered_map<int32_t,Portal> portals;
    std::unordered_map<int32_t,bool>   visible;
    bool hasFrustum{false};
    float frustum[6][4]{};

    bool PointInSector(const float p[3], const Sector& s) const {
        return p[0]>=s.mn[0]&&p[0]<=s.mx[0] && p[1]>=s.mn[1]&&p[1]<=s.mx[1] && p[2]>=s.mn[2]&&p[2]<=s.mx[2];
    }
};

VisibilitySystem::VisibilitySystem()  : m_impl(new Impl){}
VisibilitySystem::~VisibilitySystem() { Shutdown(); delete m_impl; }
void VisibilitySystem::Init()     {}
void VisibilitySystem::Shutdown() { m_impl->sectors.clear(); m_impl->portals.clear(); m_impl->visible.clear(); }

void VisibilitySystem::AddSector(int32_t id, const float mn[3], const float mx[3]){
    Sector s; s.id=id; for(int i=0;i<3;i++){s.mn[i]=mn[i]; s.mx[i]=mx[i];}
    m_impl->sectors[id]=s;
}
void VisibilitySystem::RemoveSector(int32_t id){ m_impl->sectors.erase(id); m_impl->visible.erase(id); }

void VisibilitySystem::AddPortal(int32_t id, int32_t sA, int32_t sB, const float verts[4][3]){
    Portal p; p.id=id; p.sA=sA; p.sB=sB; p.open=true;
    for(int i=0;i<4;i++) for(int k=0;k<3;k++) p.verts[i][k]=verts[i][k];
    m_impl->portals[id]=p;
    if(m_impl->sectors.count(sA)) m_impl->sectors[sA].portals.push_back(id);
    if(m_impl->sectors.count(sB)) m_impl->sectors[sB].portals.push_back(id);
}
void VisibilitySystem::RemovePortal(int32_t id){ m_impl->portals.erase(id); }
void VisibilitySystem::OpenPortal (int32_t id){ auto it=m_impl->portals.find(id); if(it!=m_impl->portals.end()) it->second.open=true; }
void VisibilitySystem::ClosePortal(int32_t id){ auto it=m_impl->portals.find(id); if(it!=m_impl->portals.end()) it->second.open=false; }
bool VisibilitySystem::IsPortalOpen(int32_t id) const { auto it=m_impl->portals.find(id); return it!=m_impl->portals.end()&&it->second.open; }

void VisibilitySystem::UpdateVisibility(const float /*viewPos*/[3], int32_t viewSectorId){
    m_impl->visible.clear();
    // BFS flood fill via open portals
    std::deque<int32_t> queue; queue.push_back(viewSectorId);
    m_impl->visible[viewSectorId]=true;
    while(!queue.empty()){
        int32_t cur=queue.front(); queue.pop_front();
        auto sit=m_impl->sectors.find(cur); if(sit==m_impl->sectors.end()) continue;
        for(int32_t pid:sit->second.portals){
            auto pit=m_impl->portals.find(pid); if(pit==m_impl->portals.end()||!pit->second.open) continue;
            int32_t other=(pit->second.sA==cur)?pit->second.sB:pit->second.sA;
            if(!m_impl->visible.count(other)){ m_impl->visible[other]=true; queue.push_back(other); }
        }
    }
}

bool VisibilitySystem::IsVisible(int32_t id) const { return m_impl->visible.count(id)&&m_impl->visible.at(id); }
std::vector<int32_t> VisibilitySystem::GetVisibleSectors() const {
    std::vector<int32_t> out; for(auto& [k,v]:m_impl->visible) if(v) out.push_back(k); return out;
}
int32_t VisibilitySystem::FindSector(const float wp[3]) const {
    for(auto& [id,s]:m_impl->sectors) if(m_impl->PointInSector(wp,s)) return id; return -1;
}
void VisibilitySystem::SetFrustumPlanes(const float planes[6][4]){
    m_impl->hasFrustum=true; for(int i=0;i<6;i++) for(int j=0;j<4;j++) m_impl->frustum[i][j]=planes[i][j];
}
void VisibilitySystem::ClearFrustum(){ m_impl->hasFrustum=false; }
void VisibilitySystem::Reset(){ Shutdown(); Init(); }

} // namespace Engine
