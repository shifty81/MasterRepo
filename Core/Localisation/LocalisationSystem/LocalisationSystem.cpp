#include "Core/Localisation/LocalisationSystem/LocalisationSystem.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Core {

struct LocalisationSystem::Impl {
    std::string currentLocale{"en"};
    std::string fallbackLocale{"en"};
    // locale → key → value
    std::unordered_map<std::string, std::unordered_map<std::string,std::string>> table;
    // locale → key → plural forms
    std::unordered_map<std::string, std::unordered_map<std::string,std::vector<std::string>>> plurals;
    // reload paths
    std::unordered_map<std::string,std::pair<std::string,std::string>> reloadPaths; // locale->{type,path}
    std::function<void(const std::string&,const std::string&)> onLocaleChanged;

    const std::string& Lookup(const std::string& locale, const std::string& key) const {
        auto it=table.find(locale);
        if(it!=table.end()){
            auto k=it->second.find(key);
            if(k!=it->second.end()) return k->second;
        }
        // Fallback
        if(locale!=fallbackLocale){
            auto it2=table.find(fallbackLocale);
            if(it2!=table.end()){
                auto k=it2->second.find(key);
                if(k!=it2->second.end()) return k->second;
            }
        }
        static const std::string empty;
        return empty;
    }
};

LocalisationSystem::LocalisationSystem()  : m_impl(new Impl){}
LocalisationSystem::~LocalisationSystem() { Shutdown(); delete m_impl; }

void LocalisationSystem::Init(const std::string& fallback){
    m_impl->fallbackLocale=fallback; m_impl->currentLocale=fallback;
}
void LocalisationSystem::Shutdown(){ m_impl->table.clear(); }

void LocalisationSystem::SetLocale(const std::string& loc){
    auto old=m_impl->currentLocale; m_impl->currentLocale=loc;
    if(m_impl->onLocaleChanged&&old!=loc) m_impl->onLocaleChanged(old,loc);
}
std::string LocalisationSystem::GetLocale()         const { return m_impl->currentLocale; }
std::string LocalisationSystem::GetFallbackLocale() const { return m_impl->fallbackLocale; }
void        LocalisationSystem::SetFallbackLocale(const std::string& l){ m_impl->fallbackLocale=l; }

std::vector<std::string> LocalisationSystem::GetAvailableLocales() const {
    std::vector<std::string> out;
    for(auto& [k,_]:m_impl->table) out.push_back(k);
    return out;
}

bool LocalisationSystem::LoadCSV(const std::string& locale, const std::string& path){
    std::ifstream f(path); if(!f) return false;
    m_impl->reloadPaths[locale]={"csv",path};
    std::string line;
    while(std::getline(f,line)){
        if(line.empty()||line[0]=='#') continue;
        auto comma=line.find(',');
        if(comma==std::string::npos) continue;
        std::string key=line.substr(0,comma);
        std::string val=line.substr(comma+1);
        m_impl->table[locale][key]=val;
    }
    return true;
}

bool LocalisationSystem::LoadJSON(const std::string& locale, const std::string& path){
    std::ifstream f(path); if(!f) return false;
    m_impl->reloadPaths[locale]={"json",path};
    std::string line;
    while(std::getline(f,line)){
        // Minimal: "key":"value" lines
        auto c1=line.find('"'); if(c1==std::string::npos) continue;
        auto c2=line.find('"',c1+1); if(c2==std::string::npos) continue;
        std::string key=line.substr(c1+1,c2-c1-1);
        auto c3=line.find('"',c2+1); if(c3==std::string::npos) continue;
        auto c4=line.find('"',c3+1); if(c4==std::string::npos) continue;
        std::string val=line.substr(c3+1,c4-c3-1);
        m_impl->table[locale][key]=val;
    }
    return true;
}

void LocalisationSystem::Set(const std::string& locale, const std::string& key,
                               const std::string& val){
    m_impl->table[locale][key]=val;
}

std::string LocalisationSystem::GetString(const std::string& key) const {
    auto& s=m_impl->Lookup(m_impl->currentLocale,key);
    return s.empty()?key:s;
}
std::string LocalisationSystem::GetString(const std::string& locale,
                                            const std::string& key) const {
    auto& s=m_impl->Lookup(locale,key);
    return s.empty()?key:s;
}
bool LocalisationSystem::HasKey(const std::string& key) const {
    auto it=m_impl->table.find(m_impl->currentLocale);
    return it!=m_impl->table.end()&&it->second.count(key)>0;
}

void LocalisationSystem::SetPluralKey(const std::string& locale, const std::string& key,
                                        const std::vector<std::string>& forms){
    m_impl->plurals[locale][key]=forms;
}

std::string LocalisationSystem::GetPlural(const std::string& key, int64_t n) const {
    auto lit=m_impl->plurals.find(m_impl->currentLocale);
    if(lit!=m_impl->plurals.end()){
        auto kit=lit->second.find(key);
        if(kit!=lit->second.end()&&!kit->second.empty()){
            uint32_t idx=(n==1)?0:1;
            idx=std::min(idx,(uint32_t)kit->second.size()-1);
            // Replace {n}
            std::string result=kit->second[idx];
            auto pos=result.find("{n}");
            while(pos!=std::string::npos){
                result.replace(pos,3,std::to_string(n));
                pos=result.find("{n}",pos+1);
            }
            return result;
        }
    }
    return GetString(key);
}

std::string LocalisationSystem::FormatString(const std::string& key,
    const std::unordered_map<std::string,std::string>& vars) const
{
    std::string result=GetString(key);
    for(auto& [k,v]:vars){
        std::string token="{"+k+"}";
        auto pos=result.find(token);
        while(pos!=std::string::npos){
            result.replace(pos,token.size(),v);
            pos=result.find(token,pos+v.size());
        }
    }
    return result;
}

bool LocalisationSystem::ReloadLocale(const std::string& locale){
    auto it=m_impl->reloadPaths.find(locale);
    if(it==m_impl->reloadPaths.end()) return false;
    m_impl->table[locale].clear();
    if(it->second.first=="csv") return LoadCSV(locale,it->second.second);
    return LoadJSON(locale,it->second.second);
}

void LocalisationSystem::SetOnLocaleChanged(
    std::function<void(const std::string&,const std::string&)> cb){ m_impl->onLocaleChanged=cb; }

} // namespace Core
