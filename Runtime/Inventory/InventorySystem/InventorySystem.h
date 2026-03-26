#pragma once
/**
 * @file InventorySystem.h
 * @brief Grid-based inventory with stacking, weight, equipment slots, and persistence.
 *
 * Features:
 *   - Per-entity inventory containers (bag, chest, vendor)
 *   - Grid or list layout with slot capacity
 *   - Item stacking (stackable flag + maxStack)
 *   - Weight limit + carry check
 *   - Equipment slots (head, chest, legs, main-hand, off-hand, …)
 *   - Item transfer between containers
 *   - JSON persistence
 *
 * Typical usage:
 * @code
 *   InventorySystem inv;
 *   inv.Init();
 *   inv.CreateContainer(playerId, 40, 50.f);  // 40 slots, 50 kg limit
 *   ItemDef potion{"health_potion","Health Potion",0.1f,true,99};
 *   inv.AddItem(playerId, potion, 3);
 *   inv.EquipItem(playerId, "sword", EquipSlot::MainHand);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class EquipSlot : uint8_t {
    None=0, Head, Chest, Legs, Feet, MainHand, OffHand,
    Ring1, Ring2, Amulet, Back, COUNT
};

struct ItemDef {
    std::string  id;
    std::string  displayName;
    float        weight{0.f};
    bool         stackable{false};
    uint32_t     maxStack{1};
    EquipSlot    equipSlot{EquipSlot::None};
    std::string  iconPath;
    std::string  description;
};

struct ItemStack {
    ItemDef   def;
    uint32_t  quantity{1};
    uint32_t  slotIndex{0xFFFFFFFFu}; ///< ~0 = unslotted
};

struct ContainerInfo {
    uint32_t entityId{0};
    uint32_t slotCapacity{40};
    float    weightCapacity{50.f};
    float    currentWeight{0.f};
    uint32_t itemCount{0};
    bool     isEquipment{false};
};

class InventorySystem {
public:
    InventorySystem();
    ~InventorySystem();

    void Init();
    void Shutdown();

    // Container
    void CreateContainer(uint32_t entityId, uint32_t slots = 40, float weightCap = 999.f);
    void DestroyContainer(uint32_t entityId);
    bool HasContainer(uint32_t entityId) const;
    ContainerInfo GetContainerInfo(uint32_t entityId) const;

    // Items
    bool     AddItem(uint32_t entityId, const ItemDef& def, uint32_t qty = 1);
    bool     RemoveItem(uint32_t entityId, const std::string& itemId, uint32_t qty = 1);
    bool     HasItem(uint32_t entityId, const std::string& itemId, uint32_t qty = 1) const;
    uint32_t CountItem(uint32_t entityId, const std::string& itemId) const;
    std::vector<ItemStack> GetItems(uint32_t entityId) const;
    ItemStack GetItem(uint32_t entityId, const std::string& itemId) const;

    // Transfer
    bool Transfer(uint32_t fromEntity, uint32_t toEntity,
                  const std::string& itemId, uint32_t qty = 1);

    // Equipment
    bool     EquipItem(uint32_t entityId, const std::string& itemId, EquipSlot slot);
    bool     UnequipItem(uint32_t entityId, EquipSlot slot);
    ItemStack GetEquipped(uint32_t entityId, EquipSlot slot) const;
    bool     IsEquipped(uint32_t entityId, const std::string& itemId) const;

    // Persistence
    bool SaveToFile(uint32_t entityId, const std::string& path) const;
    bool LoadFromFile(uint32_t entityId, const std::string& path);

    // Callbacks
    void OnItemAdded(std::function<void(uint32_t entityId, const ItemStack&)> cb);
    void OnItemRemoved(std::function<void(uint32_t entityId, const std::string& itemId, uint32_t qty)> cb);
    void OnEquipChanged(std::function<void(uint32_t entityId, EquipSlot, const ItemStack&)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
