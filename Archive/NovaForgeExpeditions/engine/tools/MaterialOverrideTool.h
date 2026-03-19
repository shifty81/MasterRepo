#pragma once
#include "ITool.h"
#include <cstdint>
#include <string>
#include <vector>
#include <utility>

namespace atlas::tools {

/// A single material parameter override.
struct MaterialParam {
    std::string name;
    float value = 0.0f;
};

/// In-client overlay tool for live material/shader parameter tweaks.
///
/// Select an entity, then adjust material parameters (color, roughness,
/// metallic, emission, etc.) in real-time without reopening the full
/// material editor panel.
class MaterialOverrideTool : public ITool {
public:
    std::string Name() const override { return "Material Override"; }
    void Activate() override;
    void Deactivate() override;
    void Update(float deltaTime) override;
    bool IsActive() const override { return m_active; }

    // Target entity
    void SetTarget(uint32_t entityId);
    uint32_t Target() const { return m_target; }

    // Override a named parameter
    void SetParam(const std::string& name, float value);

    // Query overrides
    float GetParam(const std::string& name) const;
    bool HasParam(const std::string& name) const;
    const std::vector<MaterialParam>& AllParams() const { return m_params; }
    size_t ParamCount() const { return m_params.size(); }

    // Reset all overrides
    void ResetOverrides();

private:
    bool m_active = false;
    uint32_t m_target = 0;
    std::vector<MaterialParam> m_params;
};

} // namespace atlas::tools
