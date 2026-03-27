#include "Runtime/Inventory/Item/ItemSystem/ItemSystem.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct ItemInstance {
    uint32_t    instanceId;
    uint32_t    defId;
    uint32_t    stackCount{1};
    float       durability{100.f};
    std::vector<std::string> tags;
    std::unordered_map<std::string,std::string> customData;
};

struct EntityEquip {
    std::unordered_map<uint32_t,uint32_t> slotToInstance; // slot → instanceId
};

struct ItemSystem::Impl {
    std::unordered_map<uint32_t,ItemDef>      defs;
    std::unordered_map<uint32_t,ItemInstance> instances;
    std::unordered_map<uint32_t,EntityEquip>  equip;
    uint32_t nextInstId{1};

    std::function<void(uint32_t,uint32_t,uint32_t)> onEquip, onUnequip;
    std::function<void(uint32_t)>                   onDepleted;

    ItemDef*      FindDef (uint32_t id){ auto it=defs.find(id); return it!=defs.end()?&it->second:nullptr; }
    ItemInstance* FindInst(uint32_t id){ auto it=instances.find(id); return it!=instances.end()?&it->second:nullptr; }
};

ItemSystem::ItemSystem(): m_impl(new Impl){}
ItemSystem::~ItemSystem(){ Shutdown(); delete m_impl; }
void ItemSystem::Init(){}
void ItemSystem::Shutdown(){ m_impl->defs.clear(); m_impl->instances.clear(); m_impl->equip.clear(); }
void ItemSystem::Reset(){ Shutdown(); }

bool ItemSystem::RegisterItem(const ItemDef& def){
    if(m_impl->defs.count(def.id)) return false;
    m_impl->defs[def.id]=def; return true;
}

uint32_t ItemSystem::CreateInstance(uint32_t defId){
    auto* d=m_impl->FindDef(defId); if(!d) return 0;
    uint32_t id=m_impl->nextInstId++;
    ItemInstance inst; inst.instanceId=id; inst.defId=defId;
    inst.stackCount=1; inst.durability=d->maxDurability;
    m_impl->instances[id]=inst;
    return id;
}
void ItemSystem::DestroyInstance(uint32_t id){ m_impl->instances.erase(id); }
uint32_t ItemSystem::GetDefId(uint32_t id) const {
    auto* i=m_impl->FindInst(id); return i?i->defId:0;
}

uint32_t ItemSystem::GetStackCount(uint32_t id) const { auto* i=m_impl->FindInst(id); return i?i->stackCount:0; }
void     ItemSystem::SetStackCount(uint32_t id, uint32_t n){
    auto* i=m_impl->FindInst(id); if(!i) return;
    auto* d=m_impl->FindDef(i->defId);
    i->stackCount=d?std::min(n,d->maxStack):n;
    if(i->stackCount==0){ if(m_impl->onDepleted) m_impl->onDepleted(id); }
}
uint32_t ItemSystem::SplitStack(uint32_t id, uint32_t amount){
    auto* i=m_impl->FindInst(id); if(!i||amount>=i->stackCount) return 0;
    i->stackCount-=amount;
    uint32_t newId=CreateInstance(i->defId);
    auto* ni=m_impl->FindInst(newId); if(ni) ni->stackCount=amount;
    return newId;
}
bool ItemSystem::MergeStack(uint32_t dstId, uint32_t srcId){
    auto* dst=m_impl->FindInst(dstId); auto* src=m_impl->FindInst(srcId);
    if(!dst||!src||dst->defId!=src->defId) return false;
    auto* d=m_impl->FindDef(dst->defId); if(!d) return false;
    uint32_t space=d->maxStack-dst->stackCount;
    uint32_t move=std::min(src->stackCount,space);
    dst->stackCount+=move; src->stackCount-=move;
    if(src->stackCount==0) DestroyInstance(srcId);
    return move>0;
}

bool ItemSystem::Equip(uint32_t entity, uint32_t instId, uint32_t slot){
    if(!m_impl->FindInst(instId)) return false;
    m_impl->equip[entity].slotToInstance[slot]=instId;
    if(m_impl->onEquip) m_impl->onEquip(entity,instId,slot);
    return true;
}
void ItemSystem::Unequip(uint32_t entity, uint32_t slot){
    auto it=m_impl->equip.find(entity); if(it==m_impl->equip.end()) return;
    auto sit=it->second.slotToInstance.find(slot);
    if(sit==it->second.slotToInstance.end()) return;
    uint32_t instId=sit->second;
    it->second.slotToInstance.erase(sit);
    if(m_impl->onUnequip) m_impl->onUnequip(entity,instId,slot);
}
uint32_t ItemSystem::GetEquipped(uint32_t entity, uint32_t slot) const {
    auto it=m_impl->equip.find(entity); if(it==m_impl->equip.end()) return 0;
    auto sit=it->second.slotToInstance.find(slot);
    return sit!=it->second.slotToInstance.end()?sit->second:0;
}

void  ItemSystem::SetDurability(uint32_t id, float d){ auto* i=m_impl->FindInst(id); if(i) i->durability=std::max(0.f,d); }
float ItemSystem::GetDurability(uint32_t id) const { auto* i=m_impl->FindInst(id); return i?i->durability:0; }

void ItemSystem::AddItemTag(uint32_t id, const std::string& tag){
    auto* i=m_impl->FindInst(id); if(!i) return;
    if(!HasItemTag(id,tag)) i->tags.push_back(tag);
}
bool ItemSystem::HasItemTag(uint32_t id, const std::string& tag) const {
    auto* i=m_impl->FindInst(id); if(!i) return false;
    return std::find(i->tags.begin(),i->tags.end(),tag)!=i->tags.end();
}

void ItemSystem::SetCustomData(uint32_t id, const std::string& k, const std::string& v){
    auto* i=m_impl->FindInst(id); if(i) i->customData[k]=v;
}
std::string ItemSystem::GetCustomData(uint32_t id, const std::string& k, const std::string& def) const {
    auto* i=m_impl->FindInst(id); if(!i) return def;
    auto it=i->customData.find(k); return it!=i->customData.end()?it->second:def;
}

void ItemSystem::SetOnEquip   (std::function<void(uint32_t,uint32_t,uint32_t)> cb){ m_impl->onEquip=cb; }
void ItemSystem::SetOnUnequip (std::function<void(uint32_t,uint32_t,uint32_t)> cb){ m_impl->onUnequip=cb; }
void ItemSystem::SetOnDepleted(std::function<void(uint32_t)> cb){ m_impl->onDepleted=cb; }

} // namespace Runtime
