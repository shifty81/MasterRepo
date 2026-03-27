#pragma once
/**
 * @file ItemSystem.h
 * @brief Item definition registry, instance management, stacking, equip, durability.
 *
 * Features:
 *   - ItemDef: id, name, maxStack, weight, rarity, equipSlot, durability
 *   - RegisterItem(def) → bool
 *   - CreateInstance(defId) → instanceId
 *   - DestroyInstance(instanceId)
 *   - GetDefId(instanceId) → uint32_t
 *   - GetStackCount(instanceId) / SetStackCount(instanceId, n) → uint32_t
 *   - SplitStack(instanceId, amount) → newInstanceId
 *   - MergeStack(dstId, srcId) → bool
 *   - Equip(entityId, instanceId, slot) → bool
 *   - Unequip(entityId, slot)
 *   - GetEquipped(entityId, slot) → instanceId (0=none)
 *   - SetDurability(instanceId, d) / GetDurability(instanceId) → float
 *   - AddItemTag(instanceId, tag) / HasItemTag(instanceId, tag) → bool
 *   - SetCustomData(instanceId, key, val) / GetCustomData(instanceId, key) → string
 *   - SetOnEquip(cb) / SetOnUnequip(cb) / SetOnDepleted(cb)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct ItemDef {
    uint32_t    id;
    std::string name;
    uint32_t    maxStack   {1};
    float       weight     {0.f};
    uint32_t    rarity     {0};
    uint32_t    equipSlot  {0};
    float       maxDurability{100.f};
};

class ItemSystem {
public:
    ItemSystem();
    ~ItemSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Registration
    bool RegisterItem(const ItemDef& def);

    // Instance lifecycle
    uint32_t CreateInstance (uint32_t defId);
    void     DestroyInstance(uint32_t instanceId);
    uint32_t GetDefId       (uint32_t instanceId) const;

    // Stacking
    uint32_t GetStackCount (uint32_t instanceId) const;
    void     SetStackCount (uint32_t instanceId, uint32_t n);
    uint32_t SplitStack    (uint32_t instanceId, uint32_t amount);
    bool     MergeStack    (uint32_t dstId, uint32_t srcId);

    // Equipment
    bool     Equip      (uint32_t entityId, uint32_t instanceId, uint32_t slot);
    void     Unequip    (uint32_t entityId, uint32_t slot);
    uint32_t GetEquipped(uint32_t entityId, uint32_t slot) const;

    // Durability
    void  SetDurability(uint32_t instanceId, float d);
    float GetDurability(uint32_t instanceId) const;

    // Tags
    void AddItemTag (uint32_t instanceId, const std::string& tag);
    bool HasItemTag (uint32_t instanceId, const std::string& tag) const;

    // Custom data
    void        SetCustomData(uint32_t instanceId,
                               const std::string& key, const std::string& val);
    std::string GetCustomData(uint32_t instanceId,
                               const std::string& key,
                               const std::string& def = "") const;

    // Callbacks
    void SetOnEquip   (std::function<void(uint32_t entity, uint32_t inst,
                                          uint32_t slot)> cb);
    void SetOnUnequip (std::function<void(uint32_t entity, uint32_t inst,
                                          uint32_t slot)> cb);
    void SetOnDepleted(std::function<void(uint32_t instanceId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
