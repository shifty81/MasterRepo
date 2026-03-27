#include "Core/Config/ConfigSystem/ConfigSystem/ConfigSystem.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Core {

struct CbEntry { uint32_t id{0}; std::function<void()> fn; };

struct ConfigSystem::Impl {
    // Storage: section → key → value string
    std::unordered_map<std::string,
        std::unordered_map<std::string, std::string>> data;
    std::unordered_map<std::string, bool> readOnly;  ///< "Section.key" → ro

    std::vector<CbEntry> callbacks;
    uint32_t nextCbId{1};
    std::string lastFile;
    bool lastWasJson{false};

    static std::string FlatKey(const std::string& sec, const std::string& key) {
        return sec + "." + key;
    }

    bool IsReadOnly(const std::string& sec, const std::string& key) const {
        auto it=readOnly.find(FlatKey(sec,key)); return it!=readOnly.end()&&it->second;
    }

    void FireCallbacks(const std::string& flatKey) {
        for (auto& cb : callbacks) if (cb.fn) cb.fn(); (void)flatKey;
    }

    bool ParseIni(std::istream& in) {
        std::string line, section;
        while (std::getline(in, line)) {
            // Strip comments
            if (!line.empty()&&line[0]==';') continue;
            if (!line.empty()&&line[0]=='#') continue;
            if (line.empty()) continue;
            if (line.front()=='[' && line.back()==']') {
                section = line.substr(1, line.size()-2);
            } else {
                auto eq = line.find('=');
                if (eq==std::string::npos) continue;
                std::string k = line.substr(0, eq);
                std::string v = line.substr(eq+1);
                // Trim whitespace
                while (!k.empty()&&k.back()==' ') k.pop_back();
                while (!v.empty()&&v.front()==' ') v=v.substr(1);
                if (!section.empty()) data[section][k]=v;
            }
        }
        return true;
    }
};

ConfigSystem::ConfigSystem()  : m_impl(new Impl) {}
ConfigSystem::~ConfigSystem() { Shutdown(); delete m_impl; }

void ConfigSystem::Init()     {}
void ConfigSystem::Shutdown() { m_impl->data.clear(); }

bool ConfigSystem::LoadIni(const std::string& path)
{
    std::ifstream f(path);
    if (!f) return false;
    m_impl->lastFile=path; m_impl->lastWasJson=false;
    return m_impl->ParseIni(f);
}

bool ConfigSystem::LoadJSON(const std::string& path)
{
    // Minimal JSON parser (flat object only, not nested)
    std::ifstream f(path); if (!f) return false;
    m_impl->lastFile=path; m_impl->lastWasJson=true;
    // Best-effort parse of {"Section":{"key":"value",...},...}
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    // Simplified: parse INI from JSON-like key=value lines (fallback)
    std::istringstream ss(content);
    return m_impl->ParseIni(ss);
}

bool ConfigSystem::SaveIni(const std::string& path) const
{
    std::ofstream f(path); if (!f) return false;
    for (auto& [sec, keys] : m_impl->data) {
        f << "[" << sec << "]\n";
        for (auto& [k,v] : keys) f << k << "=" << v << "\n";
        f << "\n";
    }
    return true;
}

bool ConfigSystem::SaveJSON(const std::string& path) const
{
    std::ofstream f(path); if (!f) return false;
    f << "{\n";
    bool firstSec=true;
    for (auto& [sec,keys] : m_impl->data) {
        if(!firstSec) f<<",\n"; firstSec=false;
        f << "  \"" << sec << "\": {\n";
        bool firstKey=true;
        for (auto& [k,v] : keys) {
            if(!firstKey) f<<",\n"; firstKey=false;
            f << "    \"" << k << "\": \"" << v << "\"";
        }
        f << "\n  }";
    }
    f << "\n}\n";
    return true;
}

void ConfigSystem::Reload()
{
    if (m_impl->lastFile.empty()) return;
    m_impl->data.clear();
    if (m_impl->lastWasJson) LoadJSON(m_impl->lastFile);
    else                     LoadIni (m_impl->lastFile);
}

// Helpers
static bool StrToBool(const std::string& v) {
    return v=="true"||v=="1"||v=="yes"||v=="on";
}
static int32_t StrToInt(const std::string& v) {
    try { return std::stoi(v); } catch(...) { return 0; }
}
static float StrToFloat(const std::string& v) {
    try { return std::stof(v); } catch(...) { return 0.f; }
}

bool ConfigSystem::GetBool(const std::string& sec, const std::string& key, bool def) const {
    auto sit=m_impl->data.find(sec); if(sit==m_impl->data.end()) return def;
    auto kit=sit->second.find(key); if(kit==sit->second.end()) return def;
    return StrToBool(kit->second);
}
int32_t ConfigSystem::GetInt(const std::string& sec, const std::string& key, int32_t def) const {
    auto sit=m_impl->data.find(sec); if(sit==m_impl->data.end()) return def;
    auto kit=sit->second.find(key); if(kit==sit->second.end()) return def;
    return StrToInt(kit->second);
}
float ConfigSystem::GetFloat(const std::string& sec, const std::string& key, float def) const {
    auto sit=m_impl->data.find(sec); if(sit==m_impl->data.end()) return def;
    auto kit=sit->second.find(key); if(kit==sit->second.end()) return def;
    return StrToFloat(kit->second);
}
std::string ConfigSystem::GetString(const std::string& sec, const std::string& key,
                                      const std::string& def) const {
    auto sit=m_impl->data.find(sec); if(sit==m_impl->data.end()) return def;
    auto kit=sit->second.find(key); if(kit==sit->second.end()) return def;
    return kit->second;
}

void ConfigSystem::Set(const std::string& sec, const std::string& key, bool v) {
    if (m_impl->IsReadOnly(sec,key)) return;
    m_impl->data[sec][key] = v?"true":"false";
    m_impl->FireCallbacks(Impl::FlatKey(sec,key));
}
void ConfigSystem::Set(const std::string& sec, const std::string& key, int32_t v) {
    if (m_impl->IsReadOnly(sec,key)) return;
    m_impl->data[sec][key] = std::to_string(v);
    m_impl->FireCallbacks(Impl::FlatKey(sec,key));
}
void ConfigSystem::Set(const std::string& sec, const std::string& key, float v) {
    if (m_impl->IsReadOnly(sec,key)) return;
    m_impl->data[sec][key] = std::to_string(v);
    m_impl->FireCallbacks(Impl::FlatKey(sec,key));
}
void ConfigSystem::Set(const std::string& sec, const std::string& key, const std::string& v) {
    if (m_impl->IsReadOnly(sec,key)) return;
    m_impl->data[sec][key] = v;
    m_impl->FireCallbacks(Impl::FlatKey(sec,key));
}

bool ConfigSystem::HasKey(const std::string& sec, const std::string& key) const {
    auto sit=m_impl->data.find(sec); if(sit==m_impl->data.end()) return false;
    return sit->second.count(key)>0;
}
bool ConfigSystem::HasSection(const std::string& sec) const { return m_impl->data.count(sec)>0; }

void ConfigSystem::Remove(const std::string& sec, const std::string& key) {
    auto sit=m_impl->data.find(sec); if(sit!=m_impl->data.end()) sit->second.erase(key);
}
void ConfigSystem::RemoveSection(const std::string& sec) { m_impl->data.erase(sec); }

void ConfigSystem::SetReadOnly(const std::string& sec, const std::string& key, bool ro) {
    m_impl->readOnly[Impl::FlatKey(sec,key)]=ro;
}

std::vector<std::string> ConfigSystem::GetSections() const {
    std::vector<std::string> out;
    for (auto& [s,_] : m_impl->data) out.push_back(s);
    return out;
}
std::vector<std::string> ConfigSystem::GetKeys(const std::string& sec) const {
    std::vector<std::string> out;
    auto sit=m_impl->data.find(sec); if(sit==m_impl->data.end()) return out;
    for (auto& [k,_] : sit->second) out.push_back(k);
    return out;
}

std::unordered_map<std::string,std::string> ConfigSystem::ExportFlat() const {
    std::unordered_map<std::string,std::string> out;
    for (auto& [sec,keys] : m_impl->data)
        for (auto& [k,v] : keys) out[Impl::FlatKey(sec,k)]=v;
    return out;
}

void ConfigSystem::ParseCommandLine(int argc, char** argv)
{
    for (int i=1;i<argc;i++) {
        std::string arg(argv[i]);
        if (arg.substr(0,2)!="--") continue;
        arg=arg.substr(2);
        auto eq=arg.find('='); if(eq==std::string::npos) continue;
        auto dotpos=arg.find('.');
        if (dotpos==std::string::npos||dotpos>eq) continue;
        std::string sec=arg.substr(0,dotpos);
        std::string key=arg.substr(dotpos+1,eq-dotpos-1);
        std::string val=arg.substr(eq+1);
        m_impl->data[sec][key]=val;
    }
}

uint32_t ConfigSystem::OnChange(const std::string& /*flatKey*/, std::function<void()> cb) {
    uint32_t id=m_impl->nextCbId++;
    m_impl->callbacks.push_back({id,cb});
    return id;
}
void ConfigSystem::RemoveCb(uint32_t id) {
    auto& v=m_impl->callbacks;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& e){return e.id==id;}),v.end());
}

} // namespace Core
