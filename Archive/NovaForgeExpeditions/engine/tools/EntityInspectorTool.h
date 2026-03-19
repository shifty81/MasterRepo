#pragma once
#include "ITool.h"
#include <cstdint>
#include <string>
#include <vector>

namespace atlas::tools {

/// Summary of a single inspected entity's component.
struct InspectedComponent {
    std::string name;
    std::vector<std::pair<std::string, std::string>> fields; // field name → value
};

/// In-client overlay tool for quick entity inspection.
///
/// Hover or click an entity in the viewport to see its components and
/// their field values in a floating overlay — without opening the full
/// ECS Inspector panel.
class EntityInspectorTool : public ITool {
public:
    std::string Name() const override { return "Entity Inspector"; }
    void Activate() override;
    void Deactivate() override;
    void Update(float deltaTime) override;
    bool IsActive() const override { return m_active; }

    // Select entity to inspect
    void Inspect(uint32_t entityId);
    uint32_t InspectedEntity() const { return m_entityId; }

    // Add component data for the inspected entity
    void AddComponent(const std::string& name,
                      const std::vector<std::pair<std::string, std::string>>& fields);

    const std::vector<InspectedComponent>& Components() const { return m_components; }
    size_t ComponentCount() const { return m_components.size(); }

    // Clear inspection
    void ClearInspection();

private:
    bool m_active = false;
    uint32_t m_entityId = 0;
    std::vector<InspectedComponent> m_components;
};

} // namespace atlas::tools
