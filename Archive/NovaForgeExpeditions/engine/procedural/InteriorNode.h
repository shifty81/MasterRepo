#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace atlas::procedural {

/// Types of modules that can be placed inside a ship or station.
enum class ModuleType : uint8_t {
    Deck,
    BellyBay,
    RoverBay,
    DroneBay,
    Airlock,
    Engineering,
    Bridge,
    Habitat,
    Storage,
    MedBay,
    Lab,
    WeaponMount,
    ShieldArray,
    PowerPlant,
    FuelTank,
    HangarBay,
    COUNT
};

/// Physical size category for modules.
/// Conversion rules: 2 XS = 1 S, 3 S = 1 M, 5 S = 1 L, 2 L = 1 XL.
enum class ModuleSize : uint8_t {
    XS = 0,
    S,
    M,
    L,
    XL,
    COUNT
};

/// A slot where a module can be placed.
struct ModuleSlot {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    std::vector<ModuleType> allowedTypes;
    ModuleSize size = ModuleSize::S;
    bool occupied = false;
    uint32_t occupantId = 0;
};

/// A module instance placed in a slot.
struct InteriorModule {
    uint32_t id = 0;
    ModuleType type = ModuleType::Deck;
    ModuleSize size = ModuleSize::S;
    uint8_t tier = 1;
    std::string name;
    float health = 100.0f;
    float maxHealth = 100.0f;
    bool powered = true;

    /// True when the module has health remaining and is receiving power.
    bool IsOperational() const { return health > 0.0f && powered; }

    /// Current health as a percentage of maximum [0, 1].
    float HealthPercent() const {
        return (maxHealth > 0.0f) ? health / maxHealth : 0.0f;
    }
};

/// Manages a collection of module slots and placed modules.
class InteriorNode {
public:
    /// Add a slot for module placement; returns its index.
    size_t AddSlot(const ModuleSlot& slot);

    /// Place a module into the given slot.  Returns true on success.
    bool PlaceModule(size_t slotIndex, const InteriorModule& mod);

    /// Remove the module occupying the given slot.
    void RemoveModule(size_t slotIndex);

    /// Get the module placed in a slot (nullptr if empty).
    const InteriorModule* GetModule(size_t slotIndex) const;

    /// Get a slot by index (nullptr if out of range).
    const ModuleSlot* GetSlot(size_t index) const;

    /// Total number of slots.
    size_t SlotCount() const { return m_slots.size(); }

    /// Number of occupied slots.
    size_t OccupiedSlotCount() const;

    /// Find slot indices that allow the given module type.
    std::vector<size_t> FindSlotsByType(ModuleType type) const;

    /// Find slot indices matching the given size.
    std::vector<size_t> FindSlotsBySize(ModuleSize size) const;

    /// Total number of placed modules.
    size_t TotalModuleCount() const;

    /// Get all placed modules of a given type.
    std::vector<InteriorModule> GetModulesByType(ModuleType type) const;

    /// Remove all modules but keep the slot layout.
    void Clear();

    /// True if at least one operational PowerPlant module is present.
    bool HasPower() const;

private:
    std::vector<ModuleSlot> m_slots;
    std::vector<InteriorModule> m_modules;  // parallel to m_slots; only valid when slot is occupied
};

} // namespace atlas::procedural
