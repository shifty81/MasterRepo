#include "Runtime/Analytics/Heatmap/HeatmapSystem/HeatmapSystem.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct HeatmapLayer {
    uint32_t    id{0};
    std::string name;
    HeatmapDesc desc;
    std::vector<float> grid;  // gridW * gridH
    std::mutex  mtx;

    void WorldToCell(const float wp[2], int32_t& cx, int32_t& cy) const {
        float fx = (wp[0]-desc.worldMin[0])/(desc.worldMax[0]-desc.worldMin[0]+1e-9f);
        float fy = (wp[1]-desc.worldMin[1])/(desc.worldMax[1]-desc.worldMin[1]+1e-9f);
        cx = (int32_t)(fx*(float)(desc.gridW-1)+0.5f);
        cy = (int32_t)(fy*(float)(desc.gridH-1)+0.5f);
        cx = std::max(0, std::min(cx,(int32_t)desc.gridW-1));
        cy = std::max(0, std::min(cy,(int32_t)desc.gridH-1));
    }

    float CellToWorldX(uint32_t cx) const {
        return desc.worldMin[0] + (float)cx/(float)(desc.gridW-1+1e-9f) *
               (desc.worldMax[0]-desc.worldMin[0]);
    }
    float CellToWorldY(uint32_t cy) const {
        return desc.worldMin[1] + (float)cy/(float)(desc.gridH-1+1e-9f) *
               (desc.worldMax[1]-desc.worldMin[1]);
    }
};

struct HeatmapSystem::Impl {
    std::vector<HeatmapLayer*> layers;
    uint32_t nextId{1};

    HeatmapLayer* Find(uint32_t id) {
        for(auto* l:layers) if(l->id==id) return l; return nullptr;
    }
    const HeatmapLayer* Find(uint32_t id) const {
        for(auto* l:layers) if(l->id==id) return l; return nullptr;
    }
};

HeatmapSystem::HeatmapSystem()  : m_impl(new Impl) {}
HeatmapSystem::~HeatmapSystem() { Shutdown(); delete m_impl; }

void HeatmapSystem::Init()     {}
void HeatmapSystem::Shutdown() {
    for(auto* l:m_impl->layers) delete l;
    m_impl->layers.clear();
}

uint32_t HeatmapSystem::CreateLayer(const std::string& name, const HeatmapDesc& desc)
{
    auto* l = new HeatmapLayer;
    l->id   = m_impl->nextId++;
    l->name = name;
    l->desc = desc;
    l->grid.assign((size_t)desc.gridW*desc.gridH, 0.f);
    m_impl->layers.push_back(l);
    return l->id;
}

void HeatmapSystem::DestroyLayer(uint32_t id) {
    auto& v=m_impl->layers;
    for(auto it=v.begin();it!=v.end();++it) if((*it)->id==id){delete *it;v.erase(it);return;}
}
bool     HeatmapSystem::HasLayer(uint32_t id) const { return m_impl->Find(id)!=nullptr; }
uint32_t HeatmapSystem::FindLayer(const std::string& n) const {
    for(auto* l:m_impl->layers) if(l->name==n) return l->id; return 0;
}
std::vector<uint32_t> HeatmapSystem::GetAllLayers() const {
    std::vector<uint32_t> out; for(auto* l:m_impl->layers) out.push_back(l->id); return out;
}

void HeatmapSystem::AddSample(uint32_t layerId, const float wp[2], float weight)
{
    auto* l=m_impl->Find(layerId); if(!l) return;
    int32_t cx,cy; l->WorldToCell(wp,cx,cy);
    float sigma=l->desc.splatRadius;
    // Determine cell radius
    float cellW=(l->desc.worldMax[0]-l->desc.worldMin[0])/(float)l->desc.gridW;
    float cellH=(l->desc.worldMax[1]-l->desc.worldMin[1])/(float)l->desc.gridH;
    int32_t rx=(int32_t)(sigma/(cellW+1e-9f))+1;
    int32_t ry=(int32_t)(sigma/(cellH+1e-9f))+1;
    std::lock_guard<std::mutex> lk(l->mtx);
    for(int32_t dy=-ry;dy<=ry;dy++) for(int32_t dx=-rx;dx<=rx;dx++) {
        int32_t nx=cx+dx, ny=cy+dy;
        if(nx<0||ny<0||(uint32_t)nx>=l->desc.gridW||(uint32_t)ny>=l->desc.gridH) continue;
        float wx=(float)dx*cellW, wy=(float)dy*cellH;
        float d2=(wx*wx+wy*wy);
        float s2=sigma*sigma;
        float g=std::exp(-d2/(2.f*s2+1e-9f));
        l->grid[(uint32_t)ny*l->desc.gridW+(uint32_t)nx] += weight*g;
    }
}

void HeatmapSystem::AddSample3D(uint32_t layerId, const float wp3[3], float weight) {
    float wp2[2]={wp3[0],wp3[2]}; AddSample(layerId,wp2,weight);
}

void HeatmapSystem::ResetLayer(uint32_t id) {
    auto* l=m_impl->Find(id); if(l) std::fill(l->grid.begin(),l->grid.end(),0.f);
}

float HeatmapSystem::QueryValue(uint32_t id, const float wp[2]) const {
    const auto* l=m_impl->Find(id); if(!l) return 0.f;
    int32_t cx,cy; l->WorldToCell(wp,cx,cy);
    return l->grid[(uint32_t)cy*l->desc.gridW+(uint32_t)cx];
}
float HeatmapSystem::GetCell(uint32_t id, uint32_t x, uint32_t y) const {
    const auto* l=m_impl->Find(id); if(!l) return 0.f;
    if(x>=l->desc.gridW||y>=l->desc.gridH) return 0.f;
    return l->grid[y*l->desc.gridW+x];
}
float HeatmapSystem::MaxRaw(uint32_t id) const {
    const auto* l=m_impl->Find(id); if(!l||l->grid.empty()) return 0.f;
    return *std::max_element(l->grid.begin(),l->grid.end());
}
void HeatmapSystem::Normalise(uint32_t id) {
    auto* l=m_impl->Find(id); if(!l) return;
    float mx=MaxRaw(id); if(mx<=0.f) return;
    for(auto& v:l->grid) v/=mx;
}
const std::vector<float>& HeatmapSystem::GetGrid(uint32_t id) const {
    static const std::vector<float> empty;
    const auto* l=m_impl->Find(id); return l?l->grid:empty;
}

bool HeatmapSystem::ExportCSV(uint32_t id, const std::string& path) const {
    const auto* l=m_impl->Find(id); if(!l) return false;
    std::ofstream f(path); if(!f) return false;
    for(uint32_t y=0;y<l->desc.gridH;y++) {
        for(uint32_t x=0;x<l->desc.gridW;x++) {
            if(x>0) f<<",";
            f<<l->grid[y*l->desc.gridW+x];
        }
        f<<"\n";
    }
    return true;
}

bool HeatmapSystem::ExportPNG(uint32_t id, const std::string& path, bool heatColour) const {
    // Minimal PPM export (portable; no external PNG deps)
    const auto* l=m_impl->Find(id); if(!l) return false;
    std::ofstream f(path, std::ios::binary); if(!f) return false;
    f << "P6\n" << l->desc.gridW << " " << l->desc.gridH << "\n255\n";
    float mx=MaxRaw(id); if(mx<=0.f) mx=1.f;
    for(uint32_t y=l->desc.gridH;y>0;y--) {
        for(uint32_t x=0;x<l->desc.gridW;x++) {
            float v=l->grid[(y-1)*l->desc.gridW+x]/mx;
            uint8_t r,g,b;
            if(heatColour) {
                // Blue→Cyan→Green→Yellow→Red
                r=(uint8_t)(std::min(1.f,std::max(0.f,v*2.f-1.f))*255.f);
                g=(uint8_t)(std::min(1.f,std::max(0.f,v<0.5f?v*2.f:(1.f-v)*2.f))*255.f);
                b=(uint8_t)(std::min(1.f,std::max(0.f,1.f-v*2.f))*255.f);
            } else {
                r=g=b=(uint8_t)(v*255.f);
            }
            f.put((char)r).put((char)g).put((char)b);
        }
    }
    return true;
}

void HeatmapSystem::Tick(float dt) {
    for(auto* l:m_impl->layers) {
        if(l->desc.decayRate<=0.f) continue;
        float factor=std::exp(-l->desc.decayRate*dt);
        for(auto& v:l->grid) v*=factor;
    }
}

} // namespace Runtime
