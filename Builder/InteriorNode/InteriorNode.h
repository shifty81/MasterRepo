#pragma once
/**
 * @file InteriorNode.h
 * @brief Ship/station interior module slot management.
 *
 * Adapted from Nova-Forge-Expeditions (atlas::procedural → Builder namespace).
 *
 * InteriorNode manages a collection of typed module slots and placed modules
 * inside a ship or station hull:
 *
 *   ModuleType: Deck, BellyBay, RoverBay, DroneBay, Airlock, Engineering,
 *               Bridge, Habitat, Storage, MedBay, Lab, WeaponMount,
 *               ShieldArray, PowerPlant, FuelTank, HangarBay.
 *   ModuleSize: XS, S, M, L, XL.
 *   ModuleSlot: position (x/y/z), allowedTypes[], size, occupied, occupantId.
 *   InteriorModule: id, type, size, tier, name, health/maxHealth, powered.
 *
 *   InteriorNode:
 *     - AddSlot(slot) → slotIndex
 *     - PlaceModule(slotIndex, module) / RemoveModule(slotIndex)
 *     - GetModule(slotIndex) / GetSlot(slotIndex)
 *     - FindSlotsByType(type) / FindSlotsBySize(size)
 *     - OccupiedSlotCount() / TotalModuleCount()
 *     - HasPower(): true if at least one operational PowerPlant exists
 *     - Clear(): remove all modules, keep slot layout
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Builder {

// ── Module type ───────────────────────────────────────────────────────────
enum class ModuleType : uint8_t {
    Deck, BellyBay, RoverBay, DroneBay, Airlock, Engineering,
    Bridge, Habitat, Storage, MedBay, Lab, WeaponMount,
    ShieldArray, PowerPlant, FuelTank, HangarBay, COUNT
};

// ── Module size ───────────────────────────────────────────────────────────
enum class ModuleSize : uint8_t { XS = 0, S, M, L, XL, COUNT };

// ── Module slot ───────────────────────────────────────────────────────────
struct ModuleSlot {
    float                    x{0}, y{0}, z{0};
    std::vector<ModuleType>  allowedTypes;
    ModuleSize               size{ModuleSize::S};
    bool                     occupied{false};
    uint32_t                 occupantId{0};
};

// ── Module instance ───────────────────────────────────────────────────────
struct InteriorModule {
    uint32_t    id{0};
    ModuleType  type{ModuleType::Deck};
    ModuleSize  size{ModuleSize::S};
    uint8_t     tier{1};
    std::string name;
    float       health{100.0f};
    float       maxHealth{100.0f};
    bool        powered{true};

    bool  IsOperational() const { return health > 0.0f && powered; }
    float HealthPercent() const { return maxHealth > 0 ? health / maxHealth : 0; }
};

// ── InteriorNode ──────────────────────────────────────────────────────────
class InteriorNode {
public:
    size_t AddSlot(const ModuleSlot& slot);
    bool   PlaceModule(size_t slotIndex, const InteriorModule& mod);
    void   RemoveModule(size_t slotIndex);

    const InteriorModule* GetModule(size_t slotIndex) const;
    const ModuleSlot*     GetSlot(size_t index)       const;

    size_t SlotCount()         const { return m_slots.size(); }
    size_t OccupiedSlotCount() const;
    size_t TotalModuleCount()  const;

    std::vector<size_t> FindSlotsByType(ModuleType type) const;
    std::vector<size_t> FindSlotsBySize(ModuleSize size) const;
    std::vector<InteriorModule> GetModulesByType(ModuleType type) const;

    bool HasPower() const;
    void Clear();

private:
    std::vector<ModuleSlot>     m_slots;
    std::vector<InteriorModule> m_modules; // parallel to m_slots; only valid when occupied
};

} // namespace Builder
