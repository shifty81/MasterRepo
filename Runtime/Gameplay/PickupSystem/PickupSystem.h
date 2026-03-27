#pragma once
/**
 * @file PickupSystem.h
 * @brief Item pickup system: spawn, proximity collection, respawn timers.
 *
 * Features:
 *   - Register pickup types (health, ammo, key item, powerup, collectible)
 *   - Spawn/despawn pickups at world positions
 *   - Proximity collection (sphere radius) per collector entity
 *   - Magnet mode: pickups within range glide toward collector before collect
 *   - Collection cooldown / per-entity once-only flag
 *   - Respawn timer per pickup (0 = no respawn)
 *   - Team-locked pickups (only collectible by specific team tag)
 *   - Max carry limit per type per entity
 *   - Callback-driven (no direct inventory dependency)
 *   - Spatial grid partitioning for scalable proximity queries
 *
 * Typical usage:
 * @code
 *   PickupSystem ps;
 *   ps.Init();
 *   ps.RegisterType({"health_pack", PickupCategory::Health, 30.f, 10.f, 0.8f});
 *   uint32_t pk = ps.Spawn("health_pack", {10.f, 0.f, 5.f});
 *   ps.SetOnCollected([](const CollectEvent& e){
 *       HealEntity(e.collectorId, e.pickupValue);
 *   });
 *   // per-frame
 *   ps.SetCollectorPosition(playerId, playerPos);
 *   ps.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class PickupCategory : uint8_t {
    Health=0, Ammo, Weapon, Powerup, Key, Currency, Collectible, Custom
};

struct PickupTypeDesc {
    std::string       typeId;
    PickupCategory    category{PickupCategory::Custom};
    float             value{1.f};           ///< health/ammo amount etc.
    float             respawnDelay{0.f};    ///< 0 = no respawn
    float             collectRadius{0.8f};  ///< metres
    float             magnetRadius{0.f};    ///< 0 = no magnet
    float             magnetSpeed{3.f};
    std::string       teamTag;              ///< "" = any team
    uint32_t          maxCarry{0};          ///< 0 = unlimited
    bool              oneShot{false};       ///< entity can only collect once
    std::string       meshId;
    std::string       soundOnCollect;
    std::string       particleOnCollect;
};

struct PickupInstance {
    uint32_t          pickupId{0};
    std::string       typeId;
    float             position[3]{};
    bool              active{true};
    float             respawnTimer{0.f};
    float             magnetPhase{0.f};     ///< progress toward collector [0,1]
    uint32_t          magnetTarget{0};      ///< collector entity being attracted to
};

struct CollectEvent {
    uint32_t          collectorId{0};
    uint32_t          pickupId{0};
    std::string       typeId;
    PickupCategory    category{PickupCategory::Custom};
    float             pickupValue{0.f};
    float             position[3]{};
};

class PickupSystem {
public:
    PickupSystem();
    ~PickupSystem();

    void Init();
    void Shutdown();
    void Tick(float dt);

    // Type registry
    void   RegisterType(const PickupTypeDesc& desc);
    void   UnregisterType(const std::string& typeId);
    bool   HasType(const std::string& typeId) const;

    // Pickup management
    uint32_t  Spawn(const std::string& typeId, const float position[3]);
    void      Despawn(uint32_t pickupId);
    void      DespawnAll(const std::string& typeId="");
    bool      IsActive(uint32_t pickupId) const;
    const     PickupInstance* GetInstance(uint32_t pickupId) const;

    // Collector management
    void   SetCollectorPosition(uint32_t collectorId, const float position[3]);
    void   SetCollectorTeam(uint32_t collectorId, const std::string& teamTag);
    void   RemoveCollector(uint32_t collectorId);

    // Queries
    std::vector<uint32_t> GetPickupsInRadius(const float centre[3],
                                              float radius) const;
    std::vector<uint32_t> GetActivePickups(const std::string& typeId="") const;

    // Events
    using CollectCb  = std::function<void(const CollectEvent&)>;
    using SpawnCb    = std::function<void(uint32_t pickupId, const std::string& typeId)>;
    using DespawnCb  = std::function<void(uint32_t pickupId)>;
    void SetOnCollected(CollectCb cb);
    void SetOnSpawned(SpawnCb cb);
    void SetOnDespawned(DespawnCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
