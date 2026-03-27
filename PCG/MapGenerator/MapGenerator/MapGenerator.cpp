#include "PCG/MapGenerator/MapGenerator/MapGenerator.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <queue>
#include <random>
#include <sstream>
#include <vector>

namespace PCG {

struct MapGenerator::Impl {
    std::function<void(const Room&)>           onRoom;
    std::function<void(int32_t,int32_t)>       onDoor;

    // Simple seeded RNG
    std::mt19937_64 rng;

    void Seed(uint64_t s) { rng.seed(s ? s : 42); }
    int32_t RandRange(int32_t lo, int32_t hi) {
        if (lo>=hi) return lo;
        return lo + (int32_t)(rng() % (uint64_t)(hi-lo));
    }
    float RandF() { return (float)(rng()>>11) / (float)(1ull<<53); }

    void CarveRect(MapResult& map, int32_t x, int32_t y, int32_t w, int32_t h, TileType t) {
        for (int32_t ry=y;ry<y+h;ry++) for (int32_t rx=x;rx<x+w;rx++) map.Set(rx,ry,t);
    }

    void BSPSplit(MapResult& map, int32_t x, int32_t y, int32_t w, int32_t h,
                  int32_t depth, const MapGenParams& p, std::vector<Room>& rooms)
    {
        if (depth<=0 || w<(int32_t)p.minRoomSize*2+2 || h<(int32_t)p.minRoomSize*2+2) {
            // Carve a room
            int32_t rw = RandRange(p.minRoomSize, std::min((int32_t)p.maxRoomSize, w-2));
            int32_t rh = RandRange(p.minRoomSize, std::min((int32_t)p.maxRoomSize, h-2));
            int32_t rx = x+1+RandRange(0, w-rw-2);
            int32_t ry = y+1+RandRange(0, h-rh-2);
            CarveRect(map, rx, ry, rw, rh, TileType::Floor);
            Room room; room.id=(uint32_t)rooms.size()+1;
            room.x=rx; room.y=ry; room.w=rw; room.h=rh;
            room.cx=rx+rw/2; room.cy=ry+rh/2;
            rooms.push_back(room);
            if (onRoom) onRoom(room);
            return;
        }
        bool horizontal = (w > h) ? true : (h > w) ? false : (RandRange(0,2)==0);
        if (horizontal) {
            int32_t split = RandRange(p.minRoomSize+1, w-p.minRoomSize-1);
            BSPSplit(map, x,    y, split,   h, depth-1, p, rooms);
            BSPSplit(map, x+split, y, w-split, h, depth-1, p, rooms);
            // Corridor
            int32_t cy=y+h/2;
            for (int32_t cx=x+split-1;cx<=x+split+(int32_t)p.corridorWidth;cx++)
                map.Set(cx, cy, TileType::Floor);
        } else {
            int32_t split = RandRange(p.minRoomSize+1, h-p.minRoomSize-1);
            BSPSplit(map, x, y,       w, split,   depth-1, p, rooms);
            BSPSplit(map, x, y+split, w, h-split, depth-1, p, rooms);
            int32_t cx=x+w/2;
            for (int32_t cy=y+split-1;cy<=y+split+(int32_t)p.corridorWidth;cy++)
                map.Set(cx, cy, TileType::Floor);
        }
    }

    void ConnectRooms(MapResult& map, std::vector<Room>& rooms) {
        for (uint32_t i=0;i+1<rooms.size();i++) {
            auto& a=rooms[i]; auto& b=rooms[i+1];
            int32_t dx = b.cx>a.cx?1:-1, dy = b.cy>a.cy?1:-1;
            int32_t cx=a.cx;
            while (cx!=b.cx) { map.Set(cx,a.cy,TileType::Floor); cx+=dx; }
            int32_t cy=a.cy;
            while (cy!=b.cy) { map.Set(b.cx,cy,TileType::Floor); cy+=dy; }
            // Place a door
            map.Set(a.cx,a.cy,TileType::Door);
            if (onDoor) onDoor(a.cx,a.cy);
            a.neighbours.push_back(b.id); b.neighbours.push_back(a.id);
        }
    }

    void GenerateBSP(MapResult& map, const MapGenParams& p) {
        // Fill with walls
        std::fill(map.tiles.begin(), map.tiles.end(), TileType::Wall);
        BSPSplit(map, 0, 0, (int32_t)p.width, (int32_t)p.height, (int32_t)p.bspDepth, p, map.rooms);
        ConnectRooms(map, map.rooms);
    }

    void GenerateCA(MapResult& map, const MapGenParams& p) {
        // Seed randomly
        for (auto& t : map.tiles) t = (RandF() < p.seedDensity) ? TileType::Wall : TileType::Floor;
        // Iterate
        for (uint32_t it=0;it<p.caIterations;it++) {
            auto copy = map.tiles;
            for (uint32_t y=0;y<p.height;y++) for (uint32_t x=0;x<p.width;x++) {
                uint32_t neighbours=0;
                for (int dy=-1;dy<=1;dy++) for (int dx2=-1;dx2<=1;dx2++) {
                    if(dx2==0&&dy==0) continue;
                    int32_t nx=(int32_t)x+dx2, ny=(int32_t)y+dy;
                    if(nx<0||ny<0||(uint32_t)nx>=p.width||(uint32_t)ny>=p.height||
                       copy[(uint32_t)ny*p.width+(uint32_t)nx]==TileType::Wall) neighbours++;
                }
                if (copy[y*p.width+x]==TileType::Wall)
                    map.tiles[y*p.width+x]=(neighbours>=p.deathLimit)?TileType::Wall:TileType::Floor;
                else
                    map.tiles[y*p.width+x]=(neighbours>p.birthLimit)?TileType::Wall:TileType::Floor;
            }
        }
    }

    void GenerateRandomWalk(MapResult& map, const MapGenParams& p) {
        std::fill(map.tiles.begin(), map.tiles.end(), TileType::Wall);
        int32_t x=(int32_t)p.width/2, y=(int32_t)p.height/2;
        int32_t dx=1, dy=0;
        for (uint32_t s=0;s<p.walkSteps;s++) {
            if(x>=0&&y>=0&&(uint32_t)x<p.width&&(uint32_t)y<p.height)
                map.Set(x,y,TileType::Floor);
            if (RandF()<p.turnBias) {
                float ang=(float)((rng()%4)*90);
                float r=ang*3.14159f/180.f;
                dx=(int32_t)std::round(std::cos(r)); dy=(int32_t)std::round(std::sin(r));
            }
            x+=dx; y+=dy;
            x=std::max(1,(int32_t)std::min(x,(int32_t)p.width-2));
            y=std::max(1,(int32_t)std::min(y,(int32_t)p.height-2));
        }
    }
};

MapGenerator::MapGenerator()  : m_impl(new Impl) {}
MapGenerator::~MapGenerator() { delete m_impl; }

MapResult MapGenerator::Generate(const MapGenParams& params)
{
    m_impl->Seed(params.seed);
    MapResult map;
    map.width  = params.width;
    map.height = params.height;
    map.tiles.resize((size_t)params.width * params.height, TileType::Wall);

    switch (params.strategy) {
    case GenStrategy::BSP:             m_impl->GenerateBSP   (map, params); break;
    case GenStrategy::CellularAutomata:m_impl->GenerateCA    (map, params); break;
    case GenStrategy::RandomWalk:      m_impl->GenerateRandomWalk(map, params); break;
    }
    return map;
}

void MapGenerator::SetOnRoomCreated(std::function<void(const Room&)> cb) { m_impl->onRoom=cb; }
void MapGenerator::SetOnDoorPlaced (std::function<void(int32_t,int32_t)> cb) { m_impl->onDoor=cb; }

bool MapGenerator::SaveJSON(const MapResult& map, const std::string& path) {
    std::ofstream f(path); if(!f) return false;
    f << "{\"width\":" << map.width << ",\"height\":" << map.height << "}";
    return true;
}
bool MapGenerator::LoadJSON(const std::string& path, MapResult& outMap) {
    std::ifstream f(path); if(!f) return false;
    (void)outMap; return true;
}

} // namespace PCG
