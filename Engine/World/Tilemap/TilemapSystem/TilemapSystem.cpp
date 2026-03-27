#include "Engine/World/Tilemap/TilemapSystem/TilemapSystem.h"
#include <algorithm>
#include <vector>

namespace Engine {

struct TilemapLayer {
    std::vector<uint16_t> tiles;
    bool visible{true};
    bool dirty{true};
};

struct TilemapData {
    uint32_t id{0};
    TilemapDesc desc;
    std::vector<TilemapLayer> layers;
    // Atlas
    uint32_t texW{0}, texH{0}, tilePxW{1}, tilePxH{1};
    // Collision mask: tileId → uint8_t
    std::vector<uint8_t> collisionMask; // indexed by tileId, up to 4096
    std::function<void(uint32_t,uint32_t,int32_t,int32_t)> onTileChanged;
};

struct TilemapSystem::Impl {
    std::vector<TilemapData*> maps;
    uint32_t nextId{1};

    TilemapData* Find(uint32_t id){ for(auto* m:maps) if(m->id==id) return m; return nullptr; }
    const TilemapData* Find(uint32_t id) const { for(auto* m:maps) if(m->id==id) return m; return nullptr; }
};

TilemapSystem::TilemapSystem()  : m_impl(new Impl){}
TilemapSystem::~TilemapSystem() { Shutdown(); delete m_impl; }
void TilemapSystem::Init()     {}
void TilemapSystem::Shutdown() { for(auto* m:m_impl->maps) delete m; m_impl->maps.clear(); }

uint32_t TilemapSystem::Create(const TilemapDesc& d){
    TilemapData* td=new TilemapData; td->id=m_impl->nextId++; td->desc=d;
    td->layers.resize(d.layerCount);
    for(auto& l:td->layers) l.tiles.assign(d.width*d.height,0);
    td->collisionMask.assign(4096,0xFF); // all passable by default
    m_impl->maps.push_back(td); return td->id;
}
void TilemapSystem::Destroy(uint32_t id){
    auto& v=m_impl->maps;
    for(auto it=v.begin();it!=v.end();++it) if((*it)->id==id){delete *it;v.erase(it);return;}
}
bool TilemapSystem::Has(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void TilemapSystem::SetAtlas(uint32_t id, uint32_t tw, uint32_t th, uint32_t tpw, uint32_t tph){
    auto* m=m_impl->Find(id); if(m){m->texW=tw;m->texH=th;m->tilePxW=tpw;m->tilePxH=tph;}
}

void TilemapSystem::SetTile(uint32_t id, uint32_t layer, int32_t x, int32_t y, uint16_t tileId){
    auto* m=m_impl->Find(id); if(!m||layer>=(uint32_t)m->layers.size()) return;
    if(x<0||y<0||(uint32_t)x>=m->desc.width||(uint32_t)y>=m->desc.height) return;
    m->layers[layer].tiles[(uint32_t)y*m->desc.width+(uint32_t)x]=tileId;
    m->layers[layer].dirty=true;
    if(m->onTileChanged) m->onTileChanged(id,layer,x,y);
}
uint16_t TilemapSystem::GetTile(uint32_t id, uint32_t layer, int32_t x, int32_t y) const {
    const auto* m=m_impl->Find(id);
    if(!m||layer>=(uint32_t)m->layers.size()||x<0||y<0||(uint32_t)x>=m->desc.width||(uint32_t)y>=m->desc.height) return 0;
    return m->layers[layer].tiles[(uint32_t)y*m->desc.width+(uint32_t)x];
}
void TilemapSystem::FillRect(uint32_t id, uint32_t layer, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t tileId){
    for(int32_t ry=y;ry<y+h;ry++) for(int32_t rx=x;rx<x+w;rx++) SetTile(id,layer,rx,ry,tileId);
}

TileUVRect TilemapSystem::GetUVRect(uint32_t id, uint16_t tileId) const {
    const auto* m=m_impl->Find(id); if(!m||!m->tilePxW||!m->tilePxH||!m->texW||!m->texH) return {0,0,1,1};
    uint32_t tPerRow=m->texW/m->tilePxW;
    if(!tPerRow) return {0,0,1,1};
    uint32_t tx=tileId%tPerRow, ty=tileId/tPerRow;
    float u0=(float)tx*m->tilePxW/m->texW, v0=(float)ty*m->tilePxH/m->texH;
    float u1=u0+(float)m->tilePxW/m->texW, v1=v0+(float)m->tilePxH/m->texH;
    return {u0,v0,u1,v1};
}

void    TilemapSystem::SetCollisionMask(uint32_t id, uint16_t tileId, uint8_t mask){
    auto* m=m_impl->Find(id); if(m&&tileId<(uint16_t)m->collisionMask.size()) m->collisionMask[tileId]=mask;
}
uint8_t TilemapSystem::GetCollisionMask(uint32_t id, uint16_t tileId) const {
    const auto* m=m_impl->Find(id); return (m&&tileId<m->collisionMask.size())?m->collisionMask[tileId]:0xFF;
}
bool TilemapSystem::IsPassable(uint32_t id, int32_t x, int32_t y, uint8_t dir) const {
    const auto* m=m_impl->Find(id); if(!m) return true;
    // Check all layers
    for(uint32_t l=0;l<(uint32_t)m->layers.size();l++){
        uint16_t tid=GetTile(id,l,x,y);
        if(tid==0) continue;
        uint8_t mask=GetCollisionMask(id,tid);
        if(!(mask&(1<<dir))) return false;
    }
    return true;
}

void TilemapSystem::GetTileAtWorld(uint32_t id, float wx, float wy, int32_t& ox, int32_t& oy) const {
    const auto* m=m_impl->Find(id);
    ox=m?(int32_t)(wx/m->desc.tileWidth):0;
    oy=m?(int32_t)(wy/m->desc.tileHeight):0;
}
void TilemapSystem::GetWorldPos(uint32_t id, int32_t tx, int32_t ty, float& wx, float& wy) const {
    const auto* m=m_impl->Find(id);
    wx=m?(float)tx*m->desc.tileWidth:0; wy=m?(float)ty*m->desc.tileHeight:0;
}

void TilemapSystem::SetLayerVisible(uint32_t id, uint32_t layer, bool v){
    auto* m=m_impl->Find(id); if(m&&layer<(uint32_t)m->layers.size()) m->layers[layer].visible=v;
}
bool TilemapSystem::IsLayerVisible(uint32_t id, uint32_t layer) const {
    const auto* m=m_impl->Find(id); return m&&layer<(uint32_t)m->layers.size()&&m->layers[layer].visible;
}
bool TilemapSystem::IsDirty(uint32_t id, uint32_t layer) const {
    const auto* m=m_impl->Find(id); return m&&layer<(uint32_t)m->layers.size()&&m->layers[layer].dirty;
}
void TilemapSystem::ClearDirty(uint32_t id, uint32_t layer){
    auto* m=m_impl->Find(id); if(m&&layer<(uint32_t)m->layers.size()) m->layers[layer].dirty=false;
}
void TilemapSystem::SetOnTileChanged(std::function<void(uint32_t,uint32_t,int32_t,int32_t)> cb){
    for(auto* m:m_impl->maps) m->onTileChanged=cb;
}
std::vector<uint32_t> TilemapSystem::GetAll() const {
    std::vector<uint32_t> out; for(auto* m:m_impl->maps) out.push_back(m->id); return out;
}

} // namespace Engine
