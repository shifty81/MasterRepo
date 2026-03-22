#pragma once
/**
 * @file EntityInspectorTool.h
 * @brief In-editor overlay tool for quick entity component inspection.
 *
 * Adapted from Nova-Forge-Expeditions (atlas::tools → Editor namespace).
 *
 * EntityInspectorTool shows a floating overlay with an entity's components
 * and field values when the user hovers or clicks an entity in the viewport:
 *
 *   InspectedComponent: name, fields (field name → value string pairs).
 *
 *   EntityInspectorTool (implements ITool):
 *     - Inspect(entityId): set the entity to inspect.
 *     - AddComponent(name, fields): populate component data.
 *     - Components() / ComponentCount()
 *     - ClearInspection()
 *     - InspectedEntity()
 */

#include "EditorToolRegistry.h"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Editor {

// ── Component summary ─────────────────────────────────────────────────────
struct InspectedComponent {
    std::string name;
    std::vector<std::pair<std::string, std::string>> fields; // name → value
};

// ── Tool ──────────────────────────────────────────────────────────────────
class EntityInspectorTool : public ITool {
public:
    std::string Name()      const override { return "Entity Inspector"; }
    void        Activate()        override { m_active = true; }
    void        Deactivate()      override { m_active = false; }
    void        Update(float)     override {}
    bool        IsActive() const  override { return m_active; }

    void Inspect(uint32_t entityId);
    void AddComponent(const std::string& name,
                      const std::vector<std::pair<std::string, std::string>>& fields);
    void ClearInspection();

    uint32_t                              InspectedEntity()  const { return m_entityId; }
    const std::vector<InspectedComponent>& Components()      const { return m_components; }
    size_t                                 ComponentCount()  const { return m_components.size(); }

private:
    bool                             m_active{false};
    uint32_t                         m_entityId{0};
    std::vector<InspectedComponent>  m_components;
};

} // namespace Editor
