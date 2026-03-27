#pragma once
/**
 * @file PrefabSystem.h
 * @brief Prefab instantiation system with nested prefabs and per-instance overrides.
 *
 * Features:
 *   - Register prefab templates from JSON or programmatically
 *   - Instantiate a prefab → returns a list of created entity IDs
 *   - Nested prefabs (prefabs containing prefab references)
 *   - Property overrides per instance (override any field of any component)
 *   - Prefab variant inheritance (child prefab extends parent prefab)
 *   - Live prefab editing: apply changes to all existing instances
 *   - Serialise instance overrides back to JSON
 *   - Pool-backed instantiation for frequently-spawned prefabs
 *
 * Typical usage:
 * @code
 *   PrefabSystem ps;
 *   ps.Init();
 *   ps.RegisterFromJSON("Prefabs/enemy_grunt.json");
 *   PrefabOverrides ov;
 *   ov.Set("Health.maxHp", 150.f);
 *   uint32_t inst = ps.Instantiate("enemy_grunt", {10,0,5}, ov);
 *   ps.Destroy(inst);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Engine {

using OverrideValue = std::variant<bool, int32_t, float, std::string>;

class PrefabOverrides {
public:
    void Set(const std::string& path, OverrideValue value);
    bool Has(const std::string& path) const;
    const OverrideValue* Get(const std::string& path) const;
    const std::unordered_map<std::string,OverrideValue>& All() const;
private:
    std::unordered_map<std::string,OverrideValue> m_values;
};

struct ComponentEntry {
    std::string                         type;      ///< e.g. "HealthComponent"
    std::unordered_map<std::string,OverrideValue> properties;
};

struct PrefabChildRef {
    std::string     prefabId;
    float           localPosition[3]{};
    float           localRotation[4]{0,0,0,1}; ///< quaternion
    PrefabOverrides overrides;
};

struct PrefabTemplate {
    std::string                  id;
    std::string                  parentPrefabId;  ///< "" = root
    std::vector<ComponentEntry>  components;
    std::vector<PrefabChildRef>  children;
    bool                         pooled{false};
    uint32_t                     poolSize{0};
};

struct PrefabInstance {
    uint32_t                     instanceId{0};
    std::string                  prefabId;
    std::vector<uint32_t>        entityIds;   ///< root + all child entity IDs
    PrefabOverrides              overrides;
};

class PrefabSystem {
public:
    PrefabSystem();
    ~PrefabSystem();

    void Init();
    void Shutdown();

    // Registration
    void   Register(const PrefabTemplate& tmpl);
    bool   RegisterFromJSON(const std::string& path);
    void   Unregister(const std::string& prefabId);
    bool   Has(const std::string& prefabId) const;
    const  PrefabTemplate* GetTemplate(const std::string& prefabId) const;

    // Instantiation
    uint32_t   Instantiate(const std::string& prefabId,
                            const float position[3],
                            const PrefabOverrides& overrides = {});
    void       Destroy(uint32_t instanceId);
    void       DestroyAll(const std::string& prefabId);

    // Queries
    const PrefabInstance* GetInstance(uint32_t instanceId) const;
    std::vector<uint32_t> GetInstancesOf(const std::string& prefabId) const;

    // Live editing
    void ApplyTemplateChange(const std::string& prefabId,
                             const std::string& propPath, OverrideValue value);

    // Entity callback (wires into ECS)
    using SpawnEntityCb  = std::function<uint32_t(const std::string& type,
                                                    const float pos[3])>;
    using DestroyEntityCb = std::function<void(uint32_t entityId)>;
    using SetPropertyCb  = std::function<void(uint32_t entityId,
                                              const std::string& compType,
                                              const std::string& propPath,
                                              const OverrideValue& val)>;
    void SetSpawnCallback(SpawnEntityCb cb);
    void SetDestroyCallback(DestroyEntityCb cb);
    void SetPropertyCallback(SetPropertyCb cb);

    // Serialisation
    bool SaveJSON(const std::string& path) const;
    bool LoadJSON(const std::string& path);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
