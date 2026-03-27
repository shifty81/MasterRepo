#pragma once
/**
 * @file MaterialSystem.h
 * @brief Material instance registry with parameter overrides and layered blending.
 *
 * Features:
 *   - Base material template definitions (shader name + default parameters)
 *   - Material instances: inherit base, override individual parameters
 *   - Parameter types: float scalar, vec2/vec3/vec4, texture slot, bool
 *   - Material layers: blend two materials with a mask texture / blend factor
 *   - Dynamic parameter updates at runtime (e.g. wet/dry blend, damage cracks)
 *   - Material variant system (e.g. "concrete_wet" = "concrete" + wetness=1)
 *   - JSON import/export
 *   - On-change callback for renderer integration
 *
 * Typical usage:
 * @code
 *   MaterialSystem ms;
 *   ms.Init();
 *   ms.RegisterBase({"concrete", "shaders/pbr", {{"roughness",0.9f},{"metallic",0.f}}});
 *   uint32_t inst = ms.CreateInstance("concrete");
 *   ms.SetFloat(inst, "roughness", 0.4f);   // rain-wet surface
 *   ms.SetTexture(inst, "albedo", "tex/concrete_damaged");
 *   // renderer binds: ms.GetInstance(inst)
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Engine {

using MatParam = std::variant<bool, float, std::array<float,2>,
                               std::array<float,3>, std::array<float,4>,
                               std::string /*texture path*/>;

struct MatParamDef {
    std::string name;
    MatParam    defaultValue;
};

struct BaseMaterialDesc {
    std::string              id;
    std::string              shaderName;
    std::vector<MatParamDef> params;
    std::string              blendMode;  ///< "opaque", "alpha", "additive"
    bool                     castsShadow{true};
    bool                     receiveShadow{true};
};

struct MaterialLayer {
    uint32_t    instanceIdA{0};
    uint32_t    instanceIdB{0};
    float       blendFactor{0.5f};
    std::string maskTexture;
};

struct MaterialInstanceData {
    uint32_t    instanceId{0};
    std::string baseId;
    std::unordered_map<std::string, MatParam> overrides;
    bool        dirty{true};
};

class MaterialSystem {
public:
    MaterialSystem();
    ~MaterialSystem();

    void Init();
    void Shutdown();

    // Base materials
    void          RegisterBase(const BaseMaterialDesc& desc);
    void          UnregisterBase(const std::string& id);
    bool          HasBase(const std::string& id) const;
    const         BaseMaterialDesc* GetBase(const std::string& id) const;

    // Instances
    uint32_t      CreateInstance(const std::string& baseId);
    uint32_t      CloneInstance(uint32_t srcId);
    void          DestroyInstance(uint32_t instanceId);
    bool          HasInstance(uint32_t instanceId) const;

    // Parameter setters
    void SetFloat(uint32_t id, const std::string& name, float val);
    void SetVec2 (uint32_t id, const std::string& name, float x, float y);
    void SetVec3 (uint32_t id, const std::string& name, float x, float y, float z);
    void SetVec4 (uint32_t id, const std::string& name, float x, float y, float z, float w);
    void SetBool (uint32_t id, const std::string& name, bool val);
    void SetTexture(uint32_t id, const std::string& name, const std::string& path);
    void ResetParam(uint32_t id, const std::string& name);
    void ResetAll(uint32_t id);

    // Parameter getters
    const MatParam* GetParam(uint32_t id, const std::string& name) const;
    float           GetFloat(uint32_t id, const std::string& name, float fallback=0.f) const;
    std::string     GetTexture(uint32_t id, const std::string& name) const;

    // Layers
    uint32_t      CreateLayer(uint32_t instA, uint32_t instB, float blend=0.5f,
                               const std::string& maskTex="");
    void          SetLayerBlend(uint32_t layerId, float blend);
    void          DestroyLayer(uint32_t layerId);

    // Full instance data access (for renderer)
    const MaterialInstanceData* GetInstance(uint32_t id) const;
    void                        ClearDirty(uint32_t id);

    // Change notification
    using ChangeCb = std::function<void(uint32_t instanceId)>;
    uint32_t Subscribe(ChangeCb cb);
    void     Unsubscribe(uint32_t subId);

    // Serialisation
    bool SaveJSON(const std::string& path) const;
    bool LoadJSON(const std::string& path);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
