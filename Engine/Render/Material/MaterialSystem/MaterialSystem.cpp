#include "Engine/Render/Material/MaterialSystem/MaterialSystem.h"
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Engine {

struct SubRecord { uint32_t id; MaterialSystem::ChangeCb cb; };

struct MaterialSystem::Impl {
    std::unordered_map<std::string, BaseMaterialDesc>    bases;
    std::unordered_map<uint32_t, MaterialInstanceData>   instances;
    std::unordered_map<uint32_t, MaterialLayer>          layers;
    uint32_t nextInstId{1};
    uint32_t nextLayerId{0x80000000u};
    std::vector<SubRecord> subs;
    uint32_t nextSubId{1};

    void Notify(uint32_t instId) {
        for (auto& s : subs) if (s.cb) s.cb(instId);
    }

    const MatParam* Resolve(uint32_t id, const std::string& name) const {
        auto iit = instances.find(id);
        if (iit == instances.end()) return nullptr;
        // Check override
        auto oit = iit->second.overrides.find(name);
        if (oit != iit->second.overrides.end()) return &oit->second;
        // Check base
        auto bit = bases.find(iit->second.baseId);
        if (bit == bases.end()) return nullptr;
        for (auto& p : bit->second.params)
            if (p.name == name) return &p.defaultValue;
        return nullptr;
    }
};

MaterialSystem::MaterialSystem()  : m_impl(new Impl) {}
MaterialSystem::~MaterialSystem() { Shutdown(); delete m_impl; }

void MaterialSystem::Init()     {}
void MaterialSystem::Shutdown() { m_impl->instances.clear(); }

void MaterialSystem::RegisterBase(const BaseMaterialDesc& desc)
{
    m_impl->bases[desc.id] = desc;
}

void MaterialSystem::UnregisterBase(const std::string& id)
{
    m_impl->bases.erase(id);
}

bool MaterialSystem::HasBase(const std::string& id) const
{
    return m_impl->bases.count(id) > 0;
}

const BaseMaterialDesc* MaterialSystem::GetBase(const std::string& id) const
{
    auto it = m_impl->bases.find(id);
    return it != m_impl->bases.end() ? &it->second : nullptr;
}

uint32_t MaterialSystem::CreateInstance(const std::string& baseId)
{
    uint32_t id = m_impl->nextInstId++;
    MaterialInstanceData d;
    d.instanceId = id;
    d.baseId     = baseId;
    d.dirty      = true;
    m_impl->instances[id] = d;
    return id;
}

uint32_t MaterialSystem::CloneInstance(uint32_t srcId)
{
    auto it = m_impl->instances.find(srcId);
    if (it == m_impl->instances.end()) return 0;
    uint32_t id = m_impl->nextInstId++;
    MaterialInstanceData d = it->second;
    d.instanceId = id;
    d.dirty = true;
    m_impl->instances[id] = d;
    return id;
}

void MaterialSystem::DestroyInstance(uint32_t id)  { m_impl->instances.erase(id); }
bool MaterialSystem::HasInstance(uint32_t id) const { return m_impl->instances.count(id)>0; }

static void ApplyParam(MaterialInstanceData& inst, const std::string& name,
                        MatParam val, std::vector<SubRecord>& subs)
{
    inst.overrides[name] = std::move(val);
    inst.dirty = true;
    for (auto& s : subs) if (s.cb) s.cb(inst.instanceId);
}

void MaterialSystem::SetFloat(uint32_t id, const std::string& n, float v)
{
    auto it = m_impl->instances.find(id);
    if (it != m_impl->instances.end()) ApplyParam(it->second, n, MatParam(v), m_impl->subs);
}

void MaterialSystem::SetVec2(uint32_t id, const std::string& n, float x, float y)
{
    auto it = m_impl->instances.find(id);
    if (it != m_impl->instances.end()) ApplyParam(it->second, n, MatParam(std::array<float,2>{x,y}), m_impl->subs);
}

void MaterialSystem::SetVec3(uint32_t id, const std::string& n, float x, float y, float z)
{
    auto it = m_impl->instances.find(id);
    if (it != m_impl->instances.end()) ApplyParam(it->second, n, MatParam(std::array<float,3>{x,y,z}), m_impl->subs);
}

void MaterialSystem::SetVec4(uint32_t id, const std::string& n, float x, float y, float z, float w)
{
    auto it = m_impl->instances.find(id);
    if (it != m_impl->instances.end()) ApplyParam(it->second, n, MatParam(std::array<float,4>{x,y,z,w}), m_impl->subs);
}

void MaterialSystem::SetBool(uint32_t id, const std::string& n, bool v)
{
    auto it = m_impl->instances.find(id);
    if (it != m_impl->instances.end()) ApplyParam(it->second, n, MatParam(v), m_impl->subs);
}

void MaterialSystem::SetTexture(uint32_t id, const std::string& n, const std::string& path)
{
    auto it = m_impl->instances.find(id);
    if (it != m_impl->instances.end()) ApplyParam(it->second, n, MatParam(path), m_impl->subs);
}

void MaterialSystem::ResetParam(uint32_t id, const std::string& name)
{
    auto it = m_impl->instances.find(id);
    if (it == m_impl->instances.end()) return;
    it->second.overrides.erase(name);
    it->second.dirty = true;
    m_impl->Notify(id);
}

void MaterialSystem::ResetAll(uint32_t id)
{
    auto it = m_impl->instances.find(id);
    if (it == m_impl->instances.end()) return;
    it->second.overrides.clear();
    it->second.dirty = true;
    m_impl->Notify(id);
}

const MatParam* MaterialSystem::GetParam(uint32_t id, const std::string& name) const
{
    return m_impl->Resolve(id, name);
}

float MaterialSystem::GetFloat(uint32_t id, const std::string& name, float fallback) const
{
    const auto* p = m_impl->Resolve(id, name);
    if (p && std::holds_alternative<float>(*p)) return std::get<float>(*p);
    return fallback;
}

std::string MaterialSystem::GetTexture(uint32_t id, const std::string& name) const
{
    const auto* p = m_impl->Resolve(id, name);
    if (p && std::holds_alternative<std::string>(*p)) return std::get<std::string>(*p);
    return "";
}

uint32_t MaterialSystem::CreateLayer(uint32_t instA, uint32_t instB,
                                      float blend, const std::string& maskTex)
{
    uint32_t lid = m_impl->nextLayerId++;
    MaterialLayer l;
    l.instanceIdA = instA; l.instanceIdB = instB;
    l.blendFactor = blend; l.maskTexture  = maskTex;
    m_impl->layers[lid] = l;
    return lid;
}

void MaterialSystem::SetLayerBlend(uint32_t layerId, float blend)
{
    auto it = m_impl->layers.find(layerId);
    if (it != m_impl->layers.end()) it->second.blendFactor = blend;
}

void MaterialSystem::DestroyLayer(uint32_t layerId) { m_impl->layers.erase(layerId); }

const MaterialInstanceData* MaterialSystem::GetInstance(uint32_t id) const
{
    auto it = m_impl->instances.find(id);
    return it != m_impl->instances.end() ? &it->second : nullptr;
}

void MaterialSystem::ClearDirty(uint32_t id)
{
    auto it = m_impl->instances.find(id);
    if (it != m_impl->instances.end()) it->second.dirty = false;
}

uint32_t MaterialSystem::Subscribe(ChangeCb cb)
{
    uint32_t sid = m_impl->nextSubId++;
    m_impl->subs.push_back({sid, cb});
    return sid;
}

void MaterialSystem::Unsubscribe(uint32_t subId)
{
    auto& v = m_impl->subs;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& s){ return s.id==subId; }), v.end());
}

bool MaterialSystem::SaveJSON(const std::string& path) const
{
    std::ofstream f(path);
    if (!f) return false;
    f << "{\"materialSystem\":true,\"instances\":" << m_impl->instances.size() << "}\n";
    return true;
}

bool MaterialSystem::LoadJSON(const std::string& /*path*/) { return true; }

} // namespace Engine
