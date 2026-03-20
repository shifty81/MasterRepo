#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <utility>

namespace Editor {

struct InspectedComponent {
    std::string name;
    std::vector<std::pair<std::string, std::string>> fields;
};

// ──────────────────────────────────────────────────────────────
// EntityInspectorTool
// ──────────────────────────────────────────────────────────────

class EntityInspectorTool {
public:
    void Activate()               { m_active = true; }
    void Deactivate()             { m_active = false; }
    bool IsActive() const         { return m_active; }
    void Update(float /*dt*/)     {}

    void     Inspect(uint32_t entityId);
    uint32_t InspectedEntity() const { return m_entityId; }
    void AddComponent(const std::string& name,
                      const std::vector<std::pair<std::string,std::string>>& fields);
    const std::vector<InspectedComponent>& Components() const { return m_components; }
    size_t ComponentCount() const { return m_components.size(); }
    void ClearInspection();

private:
    bool     m_active   = false;
    uint32_t m_entityId = 0;
    std::vector<InspectedComponent> m_components;
};

// ──────────────────────────────────────────────────────────────
// MaterialOverrideTool
// ──────────────────────────────────────────────────────────────

struct MaterialParam { std::string name; float value = 0.0f; };

class MaterialOverrideTool {
public:
    void Activate()           { m_active = true; }
    void Deactivate()         { m_active = false; }
    bool IsActive() const     { return m_active; }
    void Update(float /*dt*/) {}

    void     SetTarget(uint32_t entityId) { m_target = entityId; }
    uint32_t Target() const               { return m_target; }
    void  SetParam(const std::string& name, float value);
    float GetParam(const std::string& name) const;
    bool  HasParam(const std::string& name) const;
    const std::vector<MaterialParam>& AllParams() const { return m_params; }
    size_t ParamCount() const { return m_params.size(); }
    void ResetOverrides();

private:
    bool     m_active = false;
    uint32_t m_target = 0;
    std::vector<MaterialParam> m_params;
};

} // namespace Editor
