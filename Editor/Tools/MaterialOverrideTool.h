#pragma once
/**
 * @file MaterialOverrideTool.h
 * @brief In-editor overlay tool for live material parameter tweaks.
 *
 * Adapted from Nova-Forge-Expeditions (atlas::tools → Editor namespace).
 *
 * MaterialOverrideTool lets the user select an entity and override individual
 * material parameters (colour, roughness, metallic, emission, etc.) in
 * real-time without opening the full material editor:
 *
 *   MaterialParam: name, float value.
 *
 *   MaterialOverrideTool (implements ITool):
 *     - SetTarget(entityId) / Target()
 *     - SetParam(name, value) / GetParam(name) / HasParam(name)
 *     - AllParams() / ParamCount()
 *     - ResetOverrides()
 */

#include "EditorToolRegistry.h"
#include <cstdint>
#include <string>
#include <vector>

namespace Editor {

// ── Material parameter ────────────────────────────────────────────────────
struct MaterialParam {
    std::string name;
    float       value{0.0f};
};

// ── Tool ──────────────────────────────────────────────────────────────────
class MaterialOverrideTool : public ITool {
public:
    std::string Name()      const override { return "Material Override"; }
    void        Activate()        override { m_active = true; }
    void        Deactivate()      override { m_active = false; }
    void        Update(float)     override {}
    bool        IsActive() const  override { return m_active; }

    void     SetTarget(uint32_t entityId);
    uint32_t Target() const { return m_target; }

    void  SetParam(const std::string& name, float value);
    float GetParam(const std::string& name) const;
    bool  HasParam(const std::string& name) const;

    const std::vector<MaterialParam>& AllParams() const { return m_params; }
    size_t ParamCount() const { return m_params.size(); }

    void ResetOverrides();

private:
    bool                       m_active{false};
    uint32_t                   m_target{0};
    std::vector<MaterialParam> m_params;
};

} // namespace Editor
