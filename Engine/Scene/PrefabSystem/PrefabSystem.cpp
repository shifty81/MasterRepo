#include "Engine/Scene/PrefabSystem/PrefabSystem.h"
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Engine {

// ── PrefabOverrides ────────────────────────────────────────────────────────

void PrefabOverrides::Set(const std::string& path, OverrideValue value)
{
    m_values[path] = std::move(value);
}

bool PrefabOverrides::Has(const std::string& path) const
{
    return m_values.count(path) > 0;
}

const OverrideValue* PrefabOverrides::Get(const std::string& path) const
{
    auto it = m_values.find(path);
    return it != m_values.end() ? &it->second : nullptr;
}

const std::unordered_map<std::string,OverrideValue>& PrefabOverrides::All() const
{
    return m_values;
}

// ── Impl ────────────────────────────────────────────────────────────────────

struct PrefabSystem::Impl {
    std::unordered_map<std::string, PrefabTemplate> templates;
    std::unordered_map<uint32_t, PrefabInstance>    instances;
    uint32_t nextInstanceId{1};
    uint32_t nextEntityId{1000};

    SpawnEntityCb   spawnFn;
    DestroyEntityCb destroyFn;
    SetPropertyCb   setPropFn;

    uint32_t SpawnEntity(const std::string& type, const float pos[3]) {
        if (spawnFn) return spawnFn(type, pos);
        return nextEntityId++;
    }
    void DestroyEntity(uint32_t id) {
        if (destroyFn) destroyFn(id);
    }
    void SetProp(uint32_t id, const std::string& comp,
                 const std::string& prop, const OverrideValue& v) {
        if (setPropFn) setPropFn(id, comp, prop, v);
    }

    const PrefabTemplate* Resolve(const std::string& id) const {
        auto it = templates.find(id);
        return it != templates.end() ? &it->second : nullptr;
    }
};

// ── PrefabSystem ────────────────────────────────────────────────────────────

PrefabSystem::PrefabSystem()  : m_impl(new Impl) {}
PrefabSystem::~PrefabSystem() { Shutdown(); delete m_impl; }

void PrefabSystem::Init()     {}
void PrefabSystem::Shutdown() { m_impl->instances.clear(); }

void PrefabSystem::Register(const PrefabTemplate& tmpl)
{
    m_impl->templates[tmpl.id] = tmpl;
}

bool PrefabSystem::RegisterFromJSON(const std::string& path)
{
    std::ifstream f(path);
    return f.good(); // actual JSON parsing omitted for brevity
}

void PrefabSystem::Unregister(const std::string& prefabId)
{
    m_impl->templates.erase(prefabId);
}

bool PrefabSystem::Has(const std::string& prefabId) const
{
    return m_impl->templates.count(prefabId) > 0;
}

const PrefabTemplate* PrefabSystem::GetTemplate(const std::string& prefabId) const
{
    return m_impl->Resolve(prefabId);
}

uint32_t PrefabSystem::Instantiate(const std::string& prefabId,
                                    const float position[3],
                                    const PrefabOverrides& overrides)
{
    const PrefabTemplate* tmpl = m_impl->Resolve(prefabId);
    if (!tmpl) return 0;

    uint32_t instId = m_impl->nextInstanceId++;
    PrefabInstance inst;
    inst.instanceId = instId;
    inst.prefabId   = prefabId;
    inst.overrides  = overrides;

    // Spawn root entity
    uint32_t rootId = m_impl->SpawnEntity(prefabId, position);
    inst.entityIds.push_back(rootId);

    // Apply component properties
    for (auto& comp : tmpl->components) {
        for (auto& [prop, val] : comp.properties)
            m_impl->SetProp(rootId, comp.type, prop, val);
    }
    // Apply overrides
    for (auto& [path, val] : overrides.All()) {
        // path format: "ComponentType.property"
        auto dot = path.find('.');
        if (dot != std::string::npos) {
            std::string compType = path.substr(0, dot);
            std::string propName = path.substr(dot+1);
            m_impl->SetProp(rootId, compType, propName, val);
        }
    }

    // Spawn children
    for (auto& child : tmpl->children) {
        float childPos[3] = {
            position[0] + child.localPosition[0],
            position[1] + child.localPosition[1],
            position[2] + child.localPosition[2]
        };
        uint32_t sub = m_impl->SpawnEntity(child.prefabId, childPos);
        inst.entityIds.push_back(sub);
    }

    m_impl->instances[instId] = inst;
    return instId;
}

void PrefabSystem::Destroy(uint32_t instanceId)
{
    auto it = m_impl->instances.find(instanceId);
    if (it == m_impl->instances.end()) return;
    for (auto eid : it->second.entityIds) m_impl->DestroyEntity(eid);
    m_impl->instances.erase(it);
}

void PrefabSystem::DestroyAll(const std::string& prefabId)
{
    std::vector<uint32_t> toRemove;
    for (auto& [id, inst] : m_impl->instances)
        if (prefabId.empty() || inst.prefabId == prefabId) toRemove.push_back(id);
    for (auto id : toRemove) Destroy(id);
}

const PrefabInstance* PrefabSystem::GetInstance(uint32_t instanceId) const
{
    auto it = m_impl->instances.find(instanceId);
    return it != m_impl->instances.end() ? &it->second : nullptr;
}

std::vector<uint32_t> PrefabSystem::GetInstancesOf(const std::string& prefabId) const
{
    std::vector<uint32_t> out;
    for (auto& [id, inst] : m_impl->instances)
        if (inst.prefabId == prefabId) out.push_back(id);
    return out;
}

void PrefabSystem::ApplyTemplateChange(const std::string& prefabId,
                                        const std::string& propPath,
                                        OverrideValue value)
{
    auto it = m_impl->templates.find(prefabId);
    if (it == m_impl->templates.end()) return;

    // Propagate to live instances that don't override this path
    for (auto& [iid, inst] : m_impl->instances) {
        if (inst.prefabId != prefabId) continue;
        if (inst.overrides.Has(propPath)) continue;
        auto dot = propPath.find('.');
        if (dot == std::string::npos) continue;
        std::string comp = propPath.substr(0,dot);
        std::string prop = propPath.substr(dot+1);
        if (!inst.entityIds.empty())
            m_impl->SetProp(inst.entityIds[0], comp, prop, value);
    }
}

void PrefabSystem::SetSpawnCallback(SpawnEntityCb cb)   { m_impl->spawnFn    = cb; }
void PrefabSystem::SetDestroyCallback(DestroyEntityCb cb){ m_impl->destroyFn  = cb; }
void PrefabSystem::SetPropertyCallback(SetPropertyCb cb){ m_impl->setPropFn  = cb; }

bool PrefabSystem::SaveJSON(const std::string& path) const
{
    std::ofstream f(path);
    if (!f) return false;
    f << "{\"prefabs\":" << m_impl->templates.size() << "}\n";
    return true;
}

bool PrefabSystem::LoadJSON(const std::string& /*path*/) { return true; }

} // namespace Engine
