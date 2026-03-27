#include "PCG/Dungeon/ProceduralDungeon/ProceduralDungeon.h"
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <memory>
#include <vector>

namespace PCG {

struct BSPNode {
    int32_t x{0},y{0},w{0},h{0};
    std::unique_ptr<BSPNode> left, right;
    RoomRect room{};
    bool hasRoom{false};
};

struct ProceduralDungeon::Impl {
    DungeonMap map;
    std::vector<std::pair<int,int>> connectivity;
    uint32_t seed{0};

    uint32_t rng(uint32_t& s){ s^=s<<13; s^=s>>17; s^=s<<5; return s; }

    void PlaceRoom(DungeonMap& m, const RoomRect& r){
        for(int32_t y=r.y;y<r.y+r.h;y++) for(int32_t x=r.x;x<r.x+r.w;x++)
            if(x>0&&y>0&&(uint32_t)x<m.width-1&&(uint32_t)y<m.height-1) m.tiles[y*m.width+x]=DungeonTile::Floor;
    }
    void DrawCorridor(DungeonMap& m, int32_t x1,int32_t y1,int32_t x2,int32_t y2){
        for(int32_t x=std::min(x1,x2);x<=std::max(x1,x2);x++) if((uint32_t)x<m.width&&(uint32_t)y1<m.height) m.tiles[y1*m.width+x]=DungeonTile::Corridor;
        for(int32_t y=std::min(y1,y2);y<=std::max(y1,y2);y++) if((uint32_t)y<m.height&&(uint32_t)x2<m.width) m.tiles[y*m.width+x2]=DungeonTile::Corridor;
    }
    void Split(BSPNode* node, const DungeonParams& p, uint32_t& rng_s, uint32_t depth){
        if(depth>=p.maxDepth||(uint32_t)node->w<p.minRoomSize*2||(uint32_t)node->h<p.minRoomSize*2){
            // Leaf - create room
            uint32_t rw=p.minRoomSize+(rng(rng_s)%(std::max(1u,(uint32_t)node->w-p.minRoomSize)));
            uint32_t rh=p.minRoomSize+(rng(rng_s)%(std::max(1u,(uint32_t)node->h-p.minRoomSize)));
            rw=std::min(rw,p.maxRoomSize); rh=std::min(rh,p.maxRoomSize);
            int32_t rx=node->x+1+(int32_t)(rng(rng_s)%(std::max(1u,(uint32_t)node->w-rw)));
            int32_t ry=node->y+1+(int32_t)(rng(rng_s)%(std::max(1u,(uint32_t)node->h-rh)));
            node->room={rx,ry,(int32_t)rw,(int32_t)rh}; node->hasRoom=true;
            return;
        }
        bool splitH=node->h>node->w;
        int32_t split; float r=0.45f+0.1f*(rng(rng_s)%100)/100.f;
        node->left.reset(new BSPNode); node->right.reset(new BSPNode);
        if(splitH){
            split=(int32_t)(node->h*r);
            *node->left ={node->x,node->y,node->w,split};
            *node->right={node->x,node->y+split,node->w,node->h-split};
        } else {
            split=(int32_t)(node->w*r);
            *node->left ={node->x,node->y,split,node->h};
            *node->right={node->x+split,node->y,node->w-split,node->h};
        }
        Split(node->left.get(),p,rng_s,depth+1);
        Split(node->right.get(),p,rng_s,depth+1);
    }
    void CollectRooms(BSPNode* node, std::vector<RoomRect>& rooms){
        if(!node) return;
        if(node->hasRoom) rooms.push_back(node->room);
        CollectRooms(node->left.get(),rooms);
        CollectRooms(node->right.get(),rooms);
    }
};

ProceduralDungeon::ProceduralDungeon() : m_impl(new Impl){}
ProceduralDungeon::~ProceduralDungeon(){ Shutdown(); delete m_impl; }
void ProceduralDungeon::Init()     {}
void ProceduralDungeon::Shutdown() { m_impl->map.tiles.clear(); m_impl->map.rooms.clear(); }

DungeonMap ProceduralDungeon::Generate(const DungeonParams& p){
    m_impl->map={};
    m_impl->map.width=p.width; m_impl->map.height=p.height;
    m_impl->map.tiles.assign(p.width*p.height,DungeonTile::Wall);
    uint32_t rng_s=p.seed?p.seed:12345u;
    BSPNode root; root.x=0; root.y=0; root.w=(int32_t)p.width; root.h=(int32_t)p.height;
    m_impl->Split(&root,p,rng_s,0);
    m_impl->CollectRooms(&root,m_impl->map.rooms);
    for(auto& r:m_impl->map.rooms) m_impl->PlaceRoom(m_impl->map,r);
    // Connect rooms with corridors
    if(p.addCorridors){
        for(uint32_t i=1;i<(uint32_t)m_impl->map.rooms.size();i++){
            auto& a=m_impl->map.rooms[i-1]; auto& b=m_impl->map.rooms[i];
            int32_t cx1=a.x+a.w/2, cy1=a.y+a.h/2;
            int32_t cx2=b.x+b.w/2, cy2=b.y+b.h/2;
            m_impl->DrawCorridor(m_impl->map,cx1,cy1,cx2,cy2);
            m_impl->connectivity.push_back({(int)i-1,(int)i});
        }
    }
    // Spawns
    for(uint32_t i=0;i<(uint32_t)m_impl->map.rooms.size();i++){
        auto& r=m_impl->map.rooms[i];
        SpawnType t=(i==0)?SpawnType::PlayerStart:(i%3==0?SpawnType::Item:SpawnType::Enemy);
        m_impl->map.spawns.push_back({r.x+r.w/2, r.y+r.h/2, t});
    }
    return m_impl->map;
}

const DungeonMap&            ProceduralDungeon::GetMap()   const { return m_impl->map; }
const std::vector<RoomRect>& ProceduralDungeon::GetRooms() const { return m_impl->map.rooms; }
std::vector<SpawnPoint> ProceduralDungeon::GetSpawnPoints(SpawnType t) const {
    std::vector<SpawnPoint> v; for(auto& s:m_impl->map.spawns) if(s.type==t) v.push_back(s); return v;
}
std::vector<std::pair<int,int>> ProceduralDungeon::GetConnectivity() const { return m_impl->connectivity; }
void ProceduralDungeon::AddCustomRoom(const RoomRect& r){
    m_impl->map.rooms.push_back(r); m_impl->PlaceRoom(m_impl->map,r);
}

} // namespace PCG
