#pragma once
#include "Runtime/ECS/ECS.h"
#include "Runtime/Components/Components.h"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace Runtime {

struct ComponentData {
    uint64_t    typeId = 0;
    std::string json;
};

struct PrefabTemplate {
    uint32_t                   id = 0;
    std::string                name;
    std::vector<ComponentData> components;
    std::vector<PrefabTemplate> children;
    std::vector<std::string>   tags;
};

struct PrefabInstance {
    uint32_t               instanceId  = 0;
    uint32_t               templateId  = 0;
    Runtime::ECS::EntityID rootEntity  = 0;
    float                  spawnTime   = 0.f;
};

class PrefabSystem {
public:
    uint32_t RegisterTemplate(PrefabTemplate tmpl);
    PrefabTemplate*       GetTemplate(uint32_t id);
    const PrefabTemplate* GetTemplate(uint32_t id) const;
    PrefabTemplate*       GetTemplateByName(const std::string& name);
    const PrefabTemplate* GetTemplateByName(const std::string& name) const;
    void RemoveTemplate(uint32_t id);

    uint32_t Instantiate(uint32_t templateId, Runtime::ECS::World& world, const float pos[3]);
    bool     Destroy(uint32_t instanceId, Runtime::ECS::World& world);

    const PrefabInstance* GetInstance(uint32_t instanceId) const;
    std::vector<uint32_t> GetInstancesByTemplate(uint32_t templateId) const;

    bool     SaveTemplate(uint32_t id, const std::string& path) const;
    uint32_t LoadTemplate(const std::string& path);

    size_t GetTemplateCount() const;
    size_t GetInstanceCount() const;
    void   Clear();

private:
    uint32_t m_nextTemplateId = 1;
    uint32_t m_nextInstanceId = 1;
    std::unordered_map<uint32_t, PrefabTemplate> m_templates;
    std::unordered_map<uint32_t, PrefabInstance> m_instances;
};

} // namespace Runtime
