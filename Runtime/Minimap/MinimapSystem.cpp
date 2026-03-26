#include "Runtime/Minimap/MinimapSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct FogCell { bool revealed{false}; };

struct MinimapSystem::Impl {
    MinimapConfig cfg;
    float         playerPos[3]{};
    float         playerHeading{0.f};
    float         zoom{1.f};
    float         targetZoom{1.f};

    std::unordered_map<uint32_t,MapIcon> icons;
    uint32_t nextIconId{1};

    std::vector<uint8_t> fogMask;  ///< 1 byte per cell: 0=hidden,255=revealed
    std::vector<uint8_t> texture;  ///< RGBA texture data

    struct RegionLabel { std::string label; float pos[3]{}; float radius{50.f}; };
    std::vector<RegionLabel> regions;

    std::function<void(uint32_t)> onIconClicked;

    void AllocBuffers() {
        uint32_t sz=cfg.textureSize;
        fogMask.assign((size_t)sz*sz, 0);
        texture.assign((size_t)sz*sz*4, 0);
    }

    void WorldToTex(const float wp[3], float& tx, float& ty) const {
        float halfExt=cfg.worldExtent;
        tx=((wp[0]+halfExt)/(2.f*halfExt))*(float)(cfg.textureSize-1);
        ty=((wp[2]+halfExt)/(2.f*halfExt))*(float)(cfg.textureSize-1);
    }
};

MinimapSystem::MinimapSystem() : m_impl(new Impl()) {}
MinimapSystem::~MinimapSystem() { delete m_impl; }

void MinimapSystem::Init(const MinimapConfig& cfg) {
    m_impl->cfg  = cfg;
    m_impl->zoom = cfg.defaultZoom;
    m_impl->targetZoom = cfg.defaultZoom;
    m_impl->AllocBuffers();
}

void MinimapSystem::Shutdown() { m_impl->icons.clear(); }

void MinimapSystem::SetPlayerPosition(const float pos[3], float heading) {
    for(int i=0;i<3;++i) m_impl->playerPos[i]=pos[i];
    m_impl->playerHeading=heading;
    if(m_impl->cfg.fogOfWarEnabled) RevealArea(pos, m_impl->cfg.fogVisionRadius);
}

void MinimapSystem::SetZoom(float z) { m_impl->zoom=m_impl->targetZoom=std::max(0.1f,z); }

void MinimapSystem::ZoomTowards(float target, float /*speed*/) { m_impl->targetZoom=std::max(0.1f,target); }

uint32_t MinimapSystem::AddIcon(const std::string& name, const std::string& sprite,
                                 IconLayer layer, const float pos[3]) {
    uint32_t id=m_impl->nextIconId++;
    auto& ic=m_impl->icons[id];
    ic.id=id; ic.name=name; ic.spritePath=sprite; ic.layer=layer;
    for(int i=0;i<3;++i) ic.worldPos[i]=pos[i];
    return id;
}

void MinimapSystem::MoveIcon(uint32_t id, const float pos[3]) {
    auto it=m_impl->icons.find(id); if(it!=m_impl->icons.end()) for(int i=0;i<3;++i) it->second.worldPos[i]=pos[i];
}
void MinimapSystem::SetIconVisible(uint32_t id, bool v) {
    auto it=m_impl->icons.find(id); if(it!=m_impl->icons.end()) it->second.visible=v;
}
void MinimapSystem::SetIconHeading(uint32_t id, float h) {
    auto it=m_impl->icons.find(id); if(it!=m_impl->icons.end()) it->second.heading=h;
}
void MinimapSystem::RemoveIcon(uint32_t id) { m_impl->icons.erase(id); }
MapIcon MinimapSystem::GetIcon(uint32_t id) const {
    auto it=m_impl->icons.find(id); return it!=m_impl->icons.end()?it->second:MapIcon{};
}

void MinimapSystem::RevealArea(const float pos[3], float radius) {
    uint32_t sz=m_impl->cfg.textureSize;
    float tx,ty; m_impl->WorldToTex(pos,tx,ty);
    float cellRadius=radius/(2.f*m_impl->cfg.worldExtent)*(float)sz;
    int r=(int)cellRadius+1;
    int cx=(int)tx, cy=(int)ty;
    for(int dy=-r;dy<=r;++dy) for(int dx=-r;dx<=r;++dx){
        int px=cx+dx, py=cy+dy;
        if(px<0||py<0||(uint32_t)px>=sz||(uint32_t)py>=sz) continue;
        if(dx*dx+dy*dy<=r*r) m_impl->fogMask[(size_t)py*sz+px]=255;
    }
}

void MinimapSystem::SetFogEnabled(bool e) { m_impl->cfg.fogOfWarEnabled=e; }
void MinimapSystem::ResetFog() { std::fill(m_impl->fogMask.begin(),m_impl->fogMask.end(),0); }

void MinimapSystem::AddRegionLabel(const std::string& label, const float pos[3], float r) {
    Impl::RegionLabel rl; rl.label=label; for(int i=0;i<3;++i) rl.pos[i]=pos[i]; rl.radius=r;
    m_impl->regions.push_back(rl);
}
void MinimapSystem::ClearRegionLabels() { m_impl->regions.clear(); }

void MinimapSystem::Update(float dt) {
    // Smooth zoom
    float diff=m_impl->targetZoom-m_impl->zoom;
    if(std::abs(diff)>0.001f) m_impl->zoom+=diff*std::min(1.f,2.f*dt);
}

void MinimapSystem::Render() {
    uint32_t sz=m_impl->cfg.textureSize;
    // Fill background
    uint32_t bg=m_impl->cfg.backgroundColour;
    for(size_t i=0;i<(size_t)sz*sz;++i){
        bool fog=m_impl->cfg.fogOfWarEnabled&&m_impl->fogMask[i]==0;
        uint32_t c=fog?bg:0xFF334433u;
        m_impl->texture[i*4+0]=(c>>16)&0xFF;
        m_impl->texture[i*4+1]=(c>>8 )&0xFF;
        m_impl->texture[i*4+2]=(c    )&0xFF;
        m_impl->texture[i*4+3]=0xFF;
    }
    // Draw icons
    for(auto&[id,ic]:m_impl->icons){
        if(!ic.visible) continue;
        float tx,ty; m_impl->WorldToTex(ic.worldPos,tx,ty);
        int px=(int)tx, py=(int)ty;
        if(px<0||py<0||(uint32_t)px>=sz||(uint32_t)py>=sz) continue;
        size_t idx=((size_t)py*sz+px)*4;
        m_impl->texture[idx+0]=(ic.colour>>16)&0xFF;
        m_impl->texture[idx+1]=(ic.colour>>8 )&0xFF;
        m_impl->texture[idx+2]=(ic.colour    )&0xFF;
        m_impl->texture[idx+3]=0xFF;
    }
}

const uint8_t* MinimapSystem::GetRenderTexture() const { return m_impl->texture.data(); }

void MinimapSystem::WorldToMap(const float wp[3], float& mx, float& my) const {
    m_impl->WorldToTex(wp,mx,my);
}
void MinimapSystem::MapToWorld(float mx, float my, float wp[3]) const {
    uint32_t sz=m_impl->cfg.textureSize; float he=m_impl->cfg.worldExtent;
    wp[0]=mx/(float)(sz-1)*2.f*he-he;
    wp[1]=0.f;
    wp[2]=my/(float)(sz-1)*2.f*he-he;
}

void MinimapSystem::OnIconClicked(std::function<void(uint32_t)> cb) {
    m_impl->onIconClicked=std::move(cb);
}

} // namespace Runtime
