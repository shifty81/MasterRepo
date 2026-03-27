#include "Runtime/Inventory/Filter/InventoryFilter/InventoryFilter.h"
#include <algorithm>
#include <cctype>
#include <cfloat>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct InventoryFilter::Impl {
    std::vector<FilterItem>    items;
    std::unordered_map<std::string,std::function<bool(const FilterItem&)>> predicates;
    mutable std::vector<uint32_t> lastResult;

    // Built-in filter state
    uint32_t minRarity{0},    maxRarity{UINT32_MAX};
    float    maxWeight{FLT_MAX};
    std::vector<std::string> tagFilter;

    FilterItem* Find(uint32_t id){
        for(auto& i:items) if(i.id==id) return &i;
        return nullptr;
    }

    bool PassBuiltin(const FilterItem& item) const {
        if(item.rarity<minRarity||item.rarity>maxRarity) return false;
        if(item.weight>maxWeight) return false;
        for(auto& req:tagFilter){
            bool found=false;
            for(auto& t:item.tags) if(t==req){found=true;break;}
            if(!found) return false;
        }
        return true;
    }
};

InventoryFilter::InventoryFilter(): m_impl(new Impl){}
InventoryFilter::~InventoryFilter(){ Shutdown(); delete m_impl; }
void InventoryFilter::Init(){}
void InventoryFilter::Shutdown(){ m_impl->items.clear(); m_impl->predicates.clear(); }
void InventoryFilter::Reset(){ Shutdown(); ClearFilters(); }

void InventoryFilter::AddItem(const FilterItem& item){ m_impl->items.push_back(item); }
void InventoryFilter::RemoveItem(uint32_t id){
    m_impl->items.erase(std::remove_if(m_impl->items.begin(),m_impl->items.end(),
        [id](const FilterItem& i){return i.id==id;}),m_impl->items.end());
}
void InventoryFilter::ClearItems(){ m_impl->items.clear(); }

void InventoryFilter::AddPredicate(const std::string& name, std::function<bool(const FilterItem&)> fn){
    m_impl->predicates[name]=fn;
}
void InventoryFilter::RemovePredicate(const std::string& name){ m_impl->predicates.erase(name); }

std::vector<uint32_t> InventoryFilter::Apply(const std::vector<std::string>& names) const {
    std::vector<uint32_t> out;
    for(auto& item:m_impl->items){
        if(!m_impl->PassBuiltin(item)) continue;
        bool ok=true;
        for(auto& n:names){
            auto it=m_impl->predicates.find(n);
            if(it!=m_impl->predicates.end()&&!it->second(item)){ok=false;break;}
        }
        if(ok) out.push_back(item.id);
    }
    m_impl->lastResult=out; return out;
}

std::vector<uint32_t> InventoryFilter::ApplyAll() const {
    std::vector<std::string> names;
    for(auto& [n,_]:m_impl->predicates) names.push_back(n);
    return Apply(names);
}

void InventoryFilter::SetRarityFilter(uint32_t mn, uint32_t mx){ m_impl->minRarity=mn; m_impl->maxRarity=mx; }
void InventoryFilter::SetWeightLimit (float w){ m_impl->maxWeight=w; }
void InventoryFilter::SetTagFilter   (const std::vector<std::string>& tags){ m_impl->tagFilter=tags; }

std::vector<uint32_t> InventoryFilter::Search(const std::string& text) const {
    std::vector<uint32_t> out;
    // Case-insensitive substring match
    std::string lo=text;
    for(auto& c:lo) c=(char)std::tolower((unsigned char)c);
    for(auto& item:m_impl->items){
        std::string name=item.name;
        for(auto& c:name) c=(char)std::tolower((unsigned char)c);
        if(name.find(lo)!=std::string::npos) out.push_back(item.id);
    }
    m_impl->lastResult=out; return out;
}

std::vector<uint32_t> InventoryFilter::Sort(const std::string& field, bool asc) const {
    std::vector<uint32_t> ids;
    for(auto& item:m_impl->items) ids.push_back(item.id);
    std::sort(ids.begin(),ids.end(),[&](uint32_t a,uint32_t b){
        const FilterItem* ia=nullptr, *ib=nullptr;
        for(auto& i:m_impl->items){ if(i.id==a) ia=&i; if(i.id==b) ib=&i; }
        if(!ia||!ib) return false;
        bool less;
        if(field=="name")       less=ia->name<ib->name;
        else if(field=="weight")   less=ia->weight<ib->weight;
        else if(field=="quantity") less=ia->quantity<ib->quantity;
        else                       less=ia->rarity<ib->rarity; // "rarity"
        return asc?less:!less;
    });
    m_impl->lastResult=ids; return ids;
}

const std::vector<uint32_t>& InventoryFilter::GetResult() const { return m_impl->lastResult; }
void InventoryFilter::ClearFilters(){
    m_impl->minRarity=0; m_impl->maxRarity=UINT32_MAX;
    m_impl->maxWeight=FLT_MAX; m_impl->tagFilter.clear();
    m_impl->predicates.clear();
}
uint32_t InventoryFilter::GetItemCount() const { return (uint32_t)m_impl->items.size(); }

} // namespace Runtime
