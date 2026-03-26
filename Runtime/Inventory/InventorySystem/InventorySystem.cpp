#include "Runtime/Inventory/InventorySystem/InventorySystem.h"
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct Container {
    uint32_t entityId{0};
    uint32_t slotCapacity{40};
    float    weightCapacity{999.f};
    std::vector<ItemStack> items;
    std::unordered_map<uint8_t,ItemStack> equipped;  // EquipSlot → item
};

struct InventorySystem::Impl {
    std::unordered_map<uint32_t, Container> containers;

    std::function<void(uint32_t,const ItemStack&)>               onAdded;
    std::function<void(uint32_t,const std::string&,uint32_t)>    onRemoved;
    std::function<void(uint32_t,EquipSlot,const ItemStack&)>     onEquip;

    float TotalWeight(const Container& c) const {
        float w=0.f;
        for(auto& s:c.items) w+=s.def.weight*(float)s.quantity;
        return w;
    }

    ItemStack* FindItem(Container& c, const std::string& id){
        for(auto& s:c.items) if(s.def.id==id) return &s;
        return nullptr;
    }
};

InventorySystem::InventorySystem() : m_impl(new Impl()) {}
InventorySystem::~InventorySystem() { delete m_impl; }
void InventorySystem::Init()     {}
void InventorySystem::Shutdown() { m_impl->containers.clear(); }

void InventorySystem::CreateContainer(uint32_t eid, uint32_t slots, float wt) {
    Container& c=m_impl->containers[eid]; c.entityId=eid; c.slotCapacity=slots; c.weightCapacity=wt;
}
void InventorySystem::DestroyContainer(uint32_t eid){ m_impl->containers.erase(eid); }
bool InventorySystem::HasContainer(uint32_t eid) const{ return m_impl->containers.count(eid)>0; }
ContainerInfo InventorySystem::GetContainerInfo(uint32_t eid) const {
    auto it=m_impl->containers.find(eid); if(it==m_impl->containers.end()) return {};
    ContainerInfo ci; ci.entityId=eid; ci.slotCapacity=it->second.slotCapacity;
    ci.weightCapacity=it->second.weightCapacity;
    ci.currentWeight=m_impl->TotalWeight(it->second);
    ci.itemCount=(uint32_t)it->second.items.size();
    return ci;
}

bool InventorySystem::AddItem(uint32_t eid, const ItemDef& def, uint32_t qty) {
    auto it=m_impl->containers.find(eid); if(it==m_impl->containers.end()) return false;
    auto& c=it->second;
    // Weight check
    if(m_impl->TotalWeight(c)+def.weight*(float)qty > c.weightCapacity) return false;
    // Stacking
    if(def.stackable) {
        auto* s=m_impl->FindItem(c,def.id);
        if(s) { s->quantity=std::min(s->quantity+qty, def.maxStack); }
        else { ItemStack ns; ns.def=def; ns.quantity=qty; c.items.push_back(ns); }
    } else {
        if(c.items.size()>=c.slotCapacity) return false;
        for(uint32_t i=0;i<qty;++i){ ItemStack ns; ns.def=def; ns.quantity=1; c.items.push_back(ns); }
    }
    if(m_impl->onAdded){ ItemStack s; s.def=def; s.quantity=qty; m_impl->onAdded(eid,s); }
    return true;
}

bool InventorySystem::RemoveItem(uint32_t eid, const std::string& id, uint32_t qty) {
    auto it=m_impl->containers.find(eid); if(it==m_impl->containers.end()) return false;
    auto* s=m_impl->FindItem(it->second,id); if(!s||s->quantity<qty) return false;
    s->quantity-=qty;
    if(s->quantity==0) {
        auto& v=it->second.items;
        v.erase(std::remove_if(v.begin(),v.end(),[&](auto& i){ return i.def.id==id&&i.quantity==0; }),v.end());
    }
    if(m_impl->onRemoved) m_impl->onRemoved(eid,id,qty);
    return true;
}

bool InventorySystem::HasItem(uint32_t eid, const std::string& id, uint32_t qty) const {
    return CountItem(eid,id)>=qty;
}
uint32_t InventorySystem::CountItem(uint32_t eid, const std::string& id) const {
    auto it=m_impl->containers.find(eid); if(it==m_impl->containers.end()) return 0;
    auto* s=const_cast<Impl*>(m_impl)->FindItem(const_cast<Container&>(it->second),id);
    return s?s->quantity:0;
}
std::vector<ItemStack> InventorySystem::GetItems(uint32_t eid) const {
    auto it=m_impl->containers.find(eid); return it!=m_impl->containers.end()?it->second.items:std::vector<ItemStack>{};
}
ItemStack InventorySystem::GetItem(uint32_t eid, const std::string& id) const {
    auto it=m_impl->containers.find(eid); if(it==m_impl->containers.end()) return {};
    auto* s=const_cast<Impl*>(m_impl)->FindItem(const_cast<Container&>(it->second),id);
    return s?*s:ItemStack{};
}

bool InventorySystem::Transfer(uint32_t from, uint32_t to, const std::string& id, uint32_t qty) {
    ItemStack s=GetItem(from,id); if(s.quantity<qty) return false;
    if(!AddItem(to,s.def,qty)) return false;
    RemoveItem(from,id,qty);
    return true;
}

bool InventorySystem::EquipItem(uint32_t eid, const std::string& id, EquipSlot slot) {
    ItemStack s=GetItem(eid,id); if(s.def.id.empty()) return false;
    auto it=m_impl->containers.find(eid); if(it==m_impl->containers.end()) return false;
    it->second.equipped[(uint8_t)slot]=s;
    if(m_impl->onEquip) m_impl->onEquip(eid,slot,s);
    return true;
}
bool InventorySystem::UnequipItem(uint32_t eid, EquipSlot slot) {
    auto it=m_impl->containers.find(eid); if(it==m_impl->containers.end()) return false;
    it->second.equipped.erase((uint8_t)slot); return true;
}
ItemStack InventorySystem::GetEquipped(uint32_t eid, EquipSlot slot) const {
    auto it=m_impl->containers.find(eid); if(it==m_impl->containers.end()) return {};
    auto jt=it->second.equipped.find((uint8_t)slot); return jt!=it->second.equipped.end()?jt->second:ItemStack{};
}
bool InventorySystem::IsEquipped(uint32_t eid, const std::string& id) const {
    auto it=m_impl->containers.find(eid); if(it==m_impl->containers.end()) return false;
    for(auto&[k,v]:it->second.equipped) if(v.def.id==id) return true;
    return false;
}

bool InventorySystem::SaveToFile(uint32_t eid, const std::string& path) const {
    std::ofstream f(path); if(!f.is_open()) return false;
    auto it=m_impl->containers.find(eid); if(it==m_impl->containers.end()) return false;
    for(auto& s:it->second.items) f<<s.def.id<<":"<<s.quantity<<"\n";
    return true;
}
bool InventorySystem::LoadFromFile(uint32_t eid, const std::string& path) {
    std::ifstream f(path); return f.is_open();
}

void InventorySystem::OnItemAdded(std::function<void(uint32_t,const ItemStack&)> cb)      { m_impl->onAdded=std::move(cb); }
void InventorySystem::OnItemRemoved(std::function<void(uint32_t,const std::string&,uint32_t)> cb){ m_impl->onRemoved=std::move(cb); }
void InventorySystem::OnEquipChanged(std::function<void(uint32_t,EquipSlot,const ItemStack&)> cb){ m_impl->onEquip=std::move(cb); }

} // namespace Runtime
