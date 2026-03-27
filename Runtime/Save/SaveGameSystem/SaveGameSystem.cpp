#include "Runtime/Save/SaveGameSystem/SaveGameSystem.h"
#include <algorithm>
#include <ctime>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Runtime {

using SaveValue = std::variant<std::string, int32_t, float, bool, std::vector<uint8_t>>;

struct SaveGameSystem::Impl {
    uint32_t currentSlot{0};
    uint32_t version{1};
    std::unordered_map<std::string, SaveValue> data;
    std::vector<SlotInfo>                      slotIndex;

    std::function<void(uint32_t)>          onSave;
    std::function<void(uint32_t)>          onLoad;
    std::function<void(const std::string&)> onError;

    std::string SlotPath(uint32_t s) const {
        return "/tmp/save_slot_" + std::to_string(s) + ".sav";
    }
};

SaveGameSystem::SaveGameSystem(): m_impl(new Impl){}
SaveGameSystem::~SaveGameSystem(){ Shutdown(); delete m_impl; }
void SaveGameSystem::Init(){}
void SaveGameSystem::Shutdown(){}
void SaveGameSystem::Reset(){
    m_impl->data.clear(); m_impl->currentSlot=0; m_impl->version=1;
}

bool SaveGameSystem::OpenSlot(uint32_t s){ m_impl->currentSlot=s; return true; }

bool SaveGameSystem::Save(){
    std::ofstream f(m_impl->SlotPath(m_impl->currentSlot), std::ios::binary);
    if(!f){ if(m_impl->onError) m_impl->onError("Cannot open save file"); return false; }
    // Write version
    f.write((const char*)&m_impl->version, sizeof(uint32_t));
    // Write count
    uint32_t cnt=(uint32_t)m_impl->data.size();
    f.write((const char*)&cnt, sizeof(uint32_t));
    for(auto& [k,v]:m_impl->data){
        uint32_t kl=(uint32_t)k.size(); f.write((const char*)&kl,4);
        f.write(k.data(),kl);
        uint8_t tag=(uint8_t)v.index(); f.write((const char*)&tag,1);
        switch(tag){
            case 0:{ auto& s=std::get<std::string>(v); uint32_t l=(uint32_t)s.size(); f.write((const char*)&l,4); f.write(s.data(),l); break;}
            case 1:{ auto& i=std::get<int32_t>(v); f.write((const char*)&i,4); break;}
            case 2:{ auto& fl=std::get<float>(v); f.write((const char*)&fl,4); break;}
            case 3:{ uint8_t b=std::get<bool>(v)?1:0; f.write((const char*)&b,1); break;}
            case 4:{ auto& bl=std::get<std::vector<uint8_t>>(v); uint32_t l=(uint32_t)bl.size(); f.write((const char*)&l,4); f.write((const char*)bl.data(),l); break;}
        }
    }
    // Update slot index
    SlotInfo si; si.slotId=m_impl->currentSlot; si.version=m_impl->version;
    si.timestamp=(uint64_t)std::time(nullptr);
    si.name="Slot"+std::to_string(m_impl->currentSlot);
    for(auto& s:m_impl->slotIndex) if(s.slotId==m_impl->currentSlot){ s=si; goto done; }
    m_impl->slotIndex.push_back(si);
done:
    if(m_impl->onSave) m_impl->onSave(m_impl->currentSlot);
    return true;
}

bool SaveGameSystem::Load(uint32_t slotId){
    std::ifstream f(m_impl->SlotPath(slotId), std::ios::binary);
    if(!f){ if(m_impl->onError) m_impl->onError("Save not found"); return false; }
    m_impl->data.clear();
    f.read((char*)&m_impl->version,sizeof(uint32_t));
    uint32_t cnt=0; f.read((char*)&cnt,4);
    for(uint32_t i=0;i<cnt;i++){
        uint32_t kl=0; f.read((char*)&kl,4);
        std::string key(kl,' '); f.read(key.data(),kl);
        uint8_t tag=0; f.read((char*)&tag,1);
        switch(tag){
            case 0:{ uint32_t l=0; f.read((char*)&l,4); std::string s(l,' '); f.read(s.data(),l); m_impl->data[key]=s; break;}
            case 1:{ int32_t v=0; f.read((char*)&v,4); m_impl->data[key]=v; break;}
            case 2:{ float v=0; f.read((char*)&v,4); m_impl->data[key]=v; break;}
            case 3:{ uint8_t b=0; f.read((char*)&b,1); m_impl->data[key]=(bool)(b!=0); break;}
            case 4:{ uint32_t l=0; f.read((char*)&l,4); std::vector<uint8_t> bl(l); f.read((char*)bl.data(),l); m_impl->data[key]=bl; break;}
        }
    }
    m_impl->currentSlot=slotId;
    if(m_impl->onLoad) m_impl->onLoad(slotId);
    return true;
}
bool SaveGameSystem::DeleteSlot(uint32_t s){
    std::remove(m_impl->SlotPath(s).c_str());
    m_impl->slotIndex.erase(std::remove_if(m_impl->slotIndex.begin(),m_impl->slotIndex.end(),
        [s](const SlotInfo& si){return si.slotId==s;}),m_impl->slotIndex.end());
    return true;
}
uint32_t SaveGameSystem::ListSlots(std::vector<SlotInfo>& out){
    out=m_impl->slotIndex; return (uint32_t)out.size();
}

void SaveGameSystem::SetString(const std::string& k,const std::string& v){ m_impl->data[k]=v; }
std::string SaveGameSystem::GetString(const std::string& k,const std::string& d) const {
    auto it=m_impl->data.find(k);
    if(it!=m_impl->data.end()&&std::holds_alternative<std::string>(it->second))
        return std::get<std::string>(it->second);
    return d;
}
void SaveGameSystem::SetInt(const std::string& k,int32_t v){ m_impl->data[k]=v; }
int32_t SaveGameSystem::GetInt(const std::string& k,int32_t d) const {
    auto it=m_impl->data.find(k);
    if(it!=m_impl->data.end()&&std::holds_alternative<int32_t>(it->second))
        return std::get<int32_t>(it->second);
    return d;
}
void SaveGameSystem::SetFloat(const std::string& k,float v){ m_impl->data[k]=v; }
float SaveGameSystem::GetFloat(const std::string& k,float d) const {
    auto it=m_impl->data.find(k);
    if(it!=m_impl->data.end()&&std::holds_alternative<float>(it->second))
        return std::get<float>(it->second);
    return d;
}
void SaveGameSystem::SetBool(const std::string& k,bool v){ m_impl->data[k]=v; }
bool SaveGameSystem::GetBool(const std::string& k,bool d) const {
    auto it=m_impl->data.find(k);
    if(it!=m_impl->data.end()&&std::holds_alternative<bool>(it->second))
        return std::get<bool>(it->second);
    return d;
}
void SaveGameSystem::SetBlob(const std::string& k,const uint8_t* data,uint32_t size){
    m_impl->data[k]=std::vector<uint8_t>(data,data+size);
}
bool SaveGameSystem::GetBlob(const std::string& k,std::vector<uint8_t>& out) const {
    auto it=m_impl->data.find(k);
    if(it!=m_impl->data.end()&&std::holds_alternative<std::vector<uint8_t>>(it->second)){
        out=std::get<std::vector<uint8_t>>(it->second); return true;
    }
    return false;
}
bool SaveGameSystem::HasKey(const std::string& k) const { return m_impl->data.count(k)>0; }
void SaveGameSystem::RemoveKey(const std::string& k){ m_impl->data.erase(k); }
void SaveGameSystem::SetVersion(uint32_t v){ m_impl->version=v; }
uint32_t SaveGameSystem::GetVersion() const { return m_impl->version; }

void SaveGameSystem::SetOnSave (std::function<void(uint32_t)> cb){ m_impl->onSave=cb; }
void SaveGameSystem::SetOnLoad (std::function<void(uint32_t)> cb){ m_impl->onLoad=cb; }
void SaveGameSystem::SetOnError(std::function<void(const std::string&)> cb){ m_impl->onError=cb; }

} // namespace Runtime
