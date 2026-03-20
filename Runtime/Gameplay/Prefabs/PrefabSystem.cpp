#include "Runtime/Gameplay/Prefabs/PrefabSystem.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace Runtime {

uint32_t PrefabSystem::RegisterTemplate(PrefabTemplate tmpl) {
    tmpl.id = m_nextTemplateId++;
    uint32_t id = tmpl.id;
    m_templates[id] = std::move(tmpl);
    return id;
}

PrefabTemplate* PrefabSystem::GetTemplate(uint32_t id) {
    auto it = m_templates.find(id);
    return (it != m_templates.end()) ? &it->second : nullptr;
}
const PrefabTemplate* PrefabSystem::GetTemplate(uint32_t id) const {
    auto it = m_templates.find(id);
    return (it != m_templates.end()) ? &it->second : nullptr;
}

PrefabTemplate* PrefabSystem::GetTemplateByName(const std::string& name) {
    for (auto& [id, tmpl] : m_templates) if (tmpl.name == name) return &tmpl;
    return nullptr;
}
const PrefabTemplate* PrefabSystem::GetTemplateByName(const std::string& name) const {
    for (const auto& [id, tmpl] : m_templates) if (tmpl.name == name) return &tmpl;
    return nullptr;
}

void PrefabSystem::RemoveTemplate(uint32_t id) { m_templates.erase(id); }

uint32_t PrefabSystem::Instantiate(uint32_t templateId, Runtime::ECS::World& world, const float pos[3]) {
    const PrefabTemplate* tmpl = GetTemplate(templateId);
    if (!tmpl) return 0;

    Runtime::ECS::EntityID entity = world.CreateEntity();
    Runtime::Components::Transform t;
    t.position.x = pos[0]; t.position.y = pos[1]; t.position.z = pos[2];
    world.AddComponent(entity, t);

    PrefabInstance inst;
    inst.instanceId = m_nextInstanceId++;
    inst.templateId = templateId;
    inst.rootEntity = entity;
    inst.spawnTime  = 0.f;
    m_instances[inst.instanceId] = inst;
    return inst.instanceId;
}

bool PrefabSystem::Destroy(uint32_t instanceId, Runtime::ECS::World& world) {
    auto it = m_instances.find(instanceId);
    if (it == m_instances.end()) return false;
    world.DestroyEntity(it->second.rootEntity);
    m_instances.erase(it);
    return true;
}

const PrefabInstance* PrefabSystem::GetInstance(uint32_t instanceId) const {
    auto it = m_instances.find(instanceId);
    return (it != m_instances.end()) ? &it->second : nullptr;
}

std::vector<uint32_t> PrefabSystem::GetInstancesByTemplate(uint32_t templateId) const {
    std::vector<uint32_t> result;
    for (const auto& [id, inst] : m_instances)
        if (inst.templateId == templateId) result.push_back(id);
    return result;
}

bool PrefabSystem::SaveTemplate(uint32_t id, const std::string& path) const {
    const PrefabTemplate* tmpl = GetTemplate(id);
    if (!tmpl) return false;
    std::ofstream f(path);
    if (!f) return false;
    f << "prefab " << tmpl->id << " " << tmpl->name << "\n";
    f << "components " << tmpl->components.size() << "\n";
    for (const auto& c : tmpl->components)
        f << "component " << c.typeId << " " << c.json << "\n";
    f << "tags " << tmpl->tags.size() << "\n";
    for (const auto& tag : tmpl->tags) f << "tag " << tag << "\n";
    return true;
}

uint32_t PrefabSystem::LoadTemplate(const std::string& path) {
    std::ifstream f(path);
    if (!f) return 0;
    PrefabTemplate tmpl;
    std::string token;
    f >> token >> tmpl.id >> tmpl.name;
    int count = 0;
    f >> token >> count;
    for (int i = 0; i < count; ++i) {
        ComponentData cd;
        f >> token >> cd.typeId >> cd.json;
        tmpl.components.push_back(cd);
    }
    f >> token >> count;
    for (int i = 0; i < count; ++i) {
        std::string tag;
        f >> token >> tag;
        tmpl.tags.push_back(tag);
    }
    return RegisterTemplate(std::move(tmpl));
}

size_t PrefabSystem::GetTemplateCount() const { return m_templates.size(); }
size_t PrefabSystem::GetInstanceCount() const { return m_instances.size(); }

void PrefabSystem::Clear() {
    m_templates.clear();
    m_instances.clear();
    m_nextTemplateId = 1;
    m_nextInstanceId = 1;
}

} // namespace Runtime
