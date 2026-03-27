#include "Runtime/Inventory/InventoryGrid/InventoryGrid/InventoryGrid.h"
#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace Runtime {

struct InventoryGrid::Impl {
    uint32_t cols{0}, rows{0};
    std::vector<uint32_t> cells; // cell → itemId (0=empty)
    std::unordered_map<uint32_t,GridItem> registry;
    std::vector<PlacedItem> placed;

    uint32_t& Cell(int c, int r){ return cells[(uint32_t)r*cols+(uint32_t)c]; }
    uint32_t  Cell(int c, int r) const { return cells[(uint32_t)r*cols+(uint32_t)c]; }
    bool InBounds(int c, int r) const { return c>=0&&r>=0&&(uint32_t)c<cols&&(uint32_t)r<rows; }

    bool CanFit(const GridItem& item, int col, int row, bool rot) const {
        uint8_t ew=rot?item.h:item.w, eh=rot?item.w:item.h;
        for(int dy=0;dy<eh;dy++) for(int dx=0;dx<ew;dx++){
            if(!InBounds(col+dx,row+dy)) return false;
            uint32_t existing=Cell(col+dx,row+dy);
            if(existing!=0 && existing!=item.id) return false;
            // stackable: same id allowed
        }
        return true;
    }

    void FillCells(uint32_t id, int col, int row, uint8_t ew, uint8_t eh){
        for(int dy=0;dy<eh;dy++) for(int dx=0;dx<ew;dx++) Cell(col+dx,row+dy)=id;
    }
    void ClearCells(uint32_t id, int col, int row, uint8_t ew, uint8_t eh){
        for(int dy=0;dy<eh;dy++) for(int dx=0;dx<ew;dx++)
            if(Cell(col+dx,row+dy)==id) Cell(col+dx,row+dy)=0;
    }
};

InventoryGrid::InventoryGrid(): m_impl(new Impl){}
InventoryGrid::~InventoryGrid(){ Shutdown(); delete m_impl; }
void InventoryGrid::Init(uint32_t c, uint32_t r){ m_impl->cols=c; m_impl->rows=r; m_impl->cells.assign(c*r,0); }
void InventoryGrid::Shutdown(){ m_impl->cells.clear(); m_impl->placed.clear(); }
void InventoryGrid::Clear(){ std::fill(m_impl->cells.begin(),m_impl->cells.end(),0); m_impl->placed.clear(); }
void InventoryGrid::RegisterItem  (const GridItem& i){ m_impl->registry[i.id]=i; }
void InventoryGrid::UnregisterItem(uint32_t id){ m_impl->registry.erase(id); }

bool InventoryGrid::CanPlace(uint32_t id, int col, int row, bool rot) const {
    auto it=m_impl->registry.find(id);
    if(it==m_impl->registry.end()) return false;
    return m_impl->CanFit(it->second,col,row,rot);
}

bool InventoryGrid::TryPlace(uint32_t id, int col, int row, bool rot){
    auto it=m_impl->registry.find(id);
    if(it==m_impl->registry.end()) return false;
    const GridItem& item=it->second;
    if(item.stackable){
        // Try to add to existing stack
        for(auto& p:m_impl->placed) if(p.itemId==id){
            auto& reg=m_impl->registry[id];
            if(reg.stackCount<reg.stackMax){ reg.stackCount++; return true; }
        }
    }
    if(!m_impl->CanFit(item,col,row,rot)) return false;
    uint8_t ew=rot?item.h:item.w, eh=rot?item.w:item.h;
    m_impl->FillCells(id,col,row,ew,eh);
    m_impl->placed.push_back({id,col,row,rot});
    return true;
}

bool InventoryGrid::Remove(uint32_t id){
    for(auto it=m_impl->placed.begin();it!=m_impl->placed.end();++it){
        if(it->itemId==id){
            auto ri=m_impl->registry.find(id);
            if(ri==m_impl->registry.end()) return false;
            uint8_t ew=it->rotated?ri->second.h:ri->second.w;
            uint8_t eh=it->rotated?ri->second.w:ri->second.h;
            m_impl->ClearCells(id,it->col,it->row,ew,eh);
            m_impl->placed.erase(it);
            return true;
        }
    }
    return false;
}

std::optional<GridPos> InventoryGrid::FindFit(uint32_t id, bool tryRotated) const {
    auto it=m_impl->registry.find(id);
    if(it==m_impl->registry.end()) return std::nullopt;
    for(int r=0;r<(int)m_impl->rows;r++) for(int c=0;c<(int)m_impl->cols;c++){
        if(m_impl->CanFit(it->second,c,r,false)) return GridPos{c,r};
        if(tryRotated && m_impl->CanFit(it->second,c,r,true)) return GridPos{c,r};
    }
    return std::nullopt;
}

uint32_t InventoryGrid::AddStack(uint32_t id, uint32_t count){
    auto it=m_impl->registry.find(id);
    if(it==m_impl->registry.end()||!it->second.stackable) return count;
    uint32_t space=it->second.stackMax-it->second.stackCount;
    uint32_t add=std::min(space,count);
    it->second.stackCount+=add;
    return count-add;
}
uint32_t InventoryGrid::RemoveStack(uint32_t id, uint32_t count){
    auto it=m_impl->registry.find(id);
    if(it==m_impl->registry.end()) return 0;
    uint32_t rem=std::min(it->second.stackCount,count);
    it->second.stackCount-=rem;
    return rem;
}

uint32_t InventoryGrid::GetItemAt(int c, int r) const {
    if(!m_impl->InBounds(c,r)) return 0;
    return m_impl->Cell(c,r);
}
const std::vector<PlacedItem>& InventoryGrid::GetPlacedItems() const { return m_impl->placed; }
uint32_t InventoryGrid::Cols() const { return m_impl->cols; }
uint32_t InventoryGrid::Rows() const { return m_impl->rows; }

std::string InventoryGrid::SaveToJSON() const {
    std::ostringstream os;
    os<<"{\"cols\":"<<m_impl->cols<<",\"rows\":"<<m_impl->rows<<",\"items\":[";
    bool first=true;
    for(auto& p:m_impl->placed){
        if(!first) os<<",";
        first=false;
        os<<"{\"id\":"<<p.itemId<<",\"col\":"<<p.col<<",\"row\":"<<p.row<<",\"rot\":"<<(p.rotated?"true":"false")<<"}";
    }
    os<<"]}";
    return os.str();
}
bool InventoryGrid::LoadFromJSON(const std::string& /*json*/){ return true; /* stub */ }

} // namespace Runtime
