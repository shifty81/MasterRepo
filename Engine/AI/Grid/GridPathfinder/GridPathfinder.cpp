#include "Engine/AI/Grid/GridPathfinder/GridPathfinder.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <unordered_map>
#include <vector>

namespace Engine {

struct Node {
    int32_t x,y; float g,f;
    bool operator>(const Node& o) const { return f>o.f; }
};

struct AsyncReq { GridNode start, goal; std::function<void(const PathResult&)> cb; };

struct GridPathfinder::Impl {
    uint32_t w{0}, h{0};
    float tileSize{1.f};
    std::vector<bool>  passable;
    std::vector<float> cost;
    bool diagonal{true}, smoothing{true};
    std::deque<AsyncReq> asyncQueue;
    float uvTime{0.f};

    bool InBounds(int32_t x, int32_t y) const { return x>=0&&y>=0&&(uint32_t)x<w&&(uint32_t)y<h; }
    uint32_t Idx(int32_t x, int32_t y) const { return (uint32_t)y*w+(uint32_t)x; }

    PathResult AStar(GridNode start, GridNode goal) const {
        PathResult res;
        if(!InBounds(start.x,start.y)||!InBounds(goal.x,goal.y)) return res;
        if(!passable[Idx(start.x,start.y)]||!passable[Idx(goal.x,goal.y)]) return res;
        std::unordered_map<uint32_t,uint32_t> parent;
        std::unordered_map<uint32_t,float> gScore;
        auto h=[&](int32_t x, int32_t y){ return std::abs(x-goal.x)+(float)std::abs(y-goal.y); };
        std::vector<Node> open;
        auto startIdx=Idx(start.x,start.y);
        gScore[startIdx]=0; open.push_back({start.x,start.y,0,h(start.x,start.y)});
        static const int dx4[]={0,1,0,-1,1,1,-1,-1};
        static const int dy4[]={1,0,-1,0,1,-1,-1,1};
        int ndirs=diagonal?8:4;
        while(!open.empty()){
            std::pop_heap(open.begin(),open.end(),std::greater<Node>());
            auto cur=open.back(); open.pop_back();
            uint32_t ci=Idx(cur.x,cur.y);
            if(cur.x==goal.x&&cur.y==goal.y){
                // Reconstruct
                uint32_t gi=Idx(goal.x,goal.y);
                GridNode c{goal.x,goal.y};
                while(gi!=startIdx){
                    res.path.push_back(c);
                    gi=parent[gi];
                    c.x=gi%w; c.y=gi/w;
                }
                res.path.push_back(start);
                std::reverse(res.path.begin(),res.path.end());
                res.found=true; res.totalCost=gScore[Idx(goal.x,goal.y)];
                return res;
            }
            for(int d=0;d<ndirs;d++){
                int nx=cur.x+dx4[d], ny=cur.y+dy4[d];
                if(!InBounds(nx,ny)||!passable[Idx(nx,ny)]) continue;
                float sc=gScore[ci]+cost[Idx(nx,ny)]*(d>=4?1.414f:1.f);
                uint32_t ni=Idx(nx,ny);
                auto it=gScore.find(ni);
                if(it==gScore.end()||sc<it->second){
                    gScore[ni]=sc; parent[ni]=ci;
                    float f=sc+h(nx,ny);
                    open.push_back({nx,ny,sc,f});
                    std::push_heap(open.begin(),open.end(),std::greater<Node>());
                }
            }
        }
        return res;
    }
};

GridPathfinder::GridPathfinder()  : m_impl(new Impl){}
GridPathfinder::~GridPathfinder() { Shutdown(); delete m_impl; }
void GridPathfinder::Shutdown()   { m_impl->passable.clear(); m_impl->cost.clear(); }

void GridPathfinder::Init(uint32_t w, uint32_t h, float tileSize){
    m_impl->w=w; m_impl->h=h; m_impl->tileSize=tileSize;
    m_impl->passable.assign(w*h,true);
    m_impl->cost.assign(w*h,1.f);
}

void  GridPathfinder::SetPassable(int32_t x, int32_t y, bool p){ if(m_impl->InBounds(x,y)) m_impl->passable[m_impl->Idx(x,y)]=p; }
void  GridPathfinder::SetCost    (int32_t x, int32_t y, float c){ if(m_impl->InBounds(x,y)) m_impl->cost[m_impl->Idx(x,y)]=c; }
bool  GridPathfinder::IsPassable (int32_t x, int32_t y) const { return m_impl->InBounds(x,y)&&m_impl->passable[m_impl->Idx(x,y)]; }
float GridPathfinder::GetCost    (int32_t x, int32_t y) const { return m_impl->InBounds(x,y)?m_impl->cost[m_impl->Idx(x,y)]:1e9f; }

void GridPathfinder::FillRect(int32_t x, int32_t y, int32_t w, int32_t h, bool p){
    for(int32_t ry=y;ry<y+h;ry++) for(int32_t rx=x;rx<x+w;rx++) SetPassable(rx,ry,p);
}
void GridPathfinder::Reset(){ m_impl->passable.assign(m_impl->w*m_impl->h,true); m_impl->cost.assign(m_impl->w*m_impl->h,1.f); }

void GridPathfinder::SetDiagonal(bool e){ m_impl->diagonal=e; }
void GridPathfinder::SetSmoothing(bool e){ m_impl->smoothing=e; }

PathResult GridPathfinder::FindPath(GridNode s, GridNode g) const { return m_impl->AStar(s,g); }
PathResult GridPathfinder::ReplanFrom(GridNode s, GridNode g)    { return m_impl->AStar(s,g); }

void GridPathfinder::PathAsync(GridNode s, GridNode g, PathCallback cb){
    m_impl->asyncQueue.push_back({s,g,cb});
}

void GridPathfinder::Tick(float){
    while(!m_impl->asyncQueue.empty()){
        auto req=m_impl->asyncQueue.front(); m_impl->asyncQueue.pop_front();
        auto res=m_impl->AStar(req.start,req.goal);
        if(req.cb) req.cb(res);
    }
}

GridNode GridPathfinder::WorldToGrid(float wx, float wy) const {
    return {(int32_t)(wx/m_impl->tileSize),(int32_t)(wy/m_impl->tileSize)};
}
void GridPathfinder::GridToWorld(int32_t gx, int32_t gy, float& wx, float& wy) const {
    wx=(gx+0.5f)*m_impl->tileSize; wy=(gy+0.5f)*m_impl->tileSize;
}
uint32_t GridPathfinder::Width()  const { return m_impl->w; }
uint32_t GridPathfinder::Height() const { return m_impl->h; }

} // namespace Engine
