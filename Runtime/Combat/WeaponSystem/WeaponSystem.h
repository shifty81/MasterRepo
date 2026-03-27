#pragma once
/**
 * @file WeaponSystem.h
 * @brief Weapon registry, fire modes, ammo/reload lifecycle, projectile dispatch.
 *
 * Features:
 *   - Weapon type registry: ranged, melee, thrown, beam
 *   - Fire modes: semi-auto, burst, full-auto, charge
 *   - Ammo pools per weapon: magazine + reserve, pickup/refill
 *   - Reload state machine: start, in-progress, chamber, cancel
 *   - Spread / recoil pattern (per-shot recoil impulse + recovery)
 *   - Multiple weapons per actor (hot-swap, primary/secondary/melee slots)
 *   - Projectile dispatch callback (no hard physics dependency)
 *   - Heat / overheat model for energy weapons
 *   - JSON definition loading
 *
 * Typical usage:
 * @code
 *   WeaponSystem ws;
 *   ws.Init();
 *   ws.RegisterType({"pistol", WeaponType::Ranged, FireMode::SemiAuto, 12, 60, 0.3f, 200.f});
 *   ws.SetProjectileDispatch([](const ShotEvent& e){ SpawnBullet(e.origin, e.dir); });
 *   uint32_t slot = ws.EquipWeapon(actorId, "pistol");
 *   ws.PullTrigger(actorId);
 *   ws.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class WeaponType : uint8_t { Ranged=0, Melee, Thrown, Beam };
enum class FireMode   : uint8_t { SemiAuto=0, Burst, FullAuto, Charge };

struct WeaponTypeDesc {
    std::string   typeId;
    WeaponType    weaponType{WeaponType::Ranged};
    FireMode      fireMode{FireMode::SemiAuto};
    uint32_t      magazineSize{10};
    uint32_t      reserveMax{100};
    float         fireRate{0.1f};         ///< seconds between shots
    float         damage{25.f};
    float         range{50.f};
    float         reloadTime{2.f};
    uint32_t      burstCount{3};          ///< shots per burst
    float         burstInterval{0.08f};   ///< s between burst shots
    float         chargeTime{1.f};        ///< for Charge mode
    float         heatPerShot{0.f};       ///< 0 = no heat model
    float         heatCoolRate{10.f};
    float         spreadBase{0.01f};      ///< radians
    float         spreadPerShot{0.005f};
    float         spreadRecovery{0.02f};  ///< per second
    std::string   muzzleEffect;
    std::string   impactEffect;
    std::string   shotSound;
    std::string   reloadSound;
};

struct ShotEvent {
    uint32_t    actorId{0};
    std::string weaponTypeId;
    float       origin[3]{};
    float       direction[3]{0,0,1};
    float       damage{0.f};
    float       range{0.f};
    float       spread{0.f};
    std::string muzzleEffect;
    std::string shotSound;
};

struct ReloadEvent {
    uint32_t    actorId{0};
    std::string weaponTypeId;
    uint32_t    newMagazine{0};
    uint32_t    newReserve{0};
};

enum class ReloadPhase : uint8_t { None=0, Started, InProgress, Chambered };

struct WeaponState {
    uint32_t    actorId{0};
    uint32_t    slotId{0};
    std::string typeId;
    uint32_t    magazine{0};
    uint32_t    reserve{0};
    float       heat{0.f};
    float       spread{0.f};
    float       fireCooldown{0.f};
    float       reloadTimer{0.f};
    ReloadPhase reloadPhase{ReloadPhase::None};
    float       chargeLevel{0.f};
    bool        triggerHeld{false};
    uint32_t    burstShotsLeft{0};
    float       burstTimer{0.f};
    bool        overheated{false};
};

class WeaponSystem {
public:
    WeaponSystem();
    ~WeaponSystem();

    void Init();
    void Shutdown();
    void Tick(float dt);

    // Type registry
    void RegisterType(const WeaponTypeDesc& desc);
    void UnregisterType(const std::string& typeId);
    bool HasType(const std::string& typeId) const;

    // Actor weapon management
    uint32_t EquipWeapon(uint32_t actorId, const std::string& typeId,
                          uint32_t slot=0);
    void     UnequipWeapon(uint32_t actorId, uint32_t slot=0);
    void     UnequipAll(uint32_t actorId);
    bool     HasWeapon(uint32_t actorId, uint32_t slot=0) const;

    // Firing
    void     PullTrigger(uint32_t actorId, uint32_t slot=0,
                          const float origin[3]=nullptr,
                          const float direction[3]=nullptr);
    void     ReleaseTrigger(uint32_t actorId, uint32_t slot=0);

    // Reload
    bool     StartReload(uint32_t actorId, uint32_t slot=0);
    void     CancelReload(uint32_t actorId, uint32_t slot=0);
    bool     IsReloading(uint32_t actorId, uint32_t slot=0) const;

    // Ammo
    void     AddAmmo(uint32_t actorId, const std::string& typeId, uint32_t amount);
    uint32_t GetMagazine(uint32_t actorId, uint32_t slot=0) const;
    uint32_t GetReserve(uint32_t actorId, uint32_t slot=0) const;

    // State query
    const WeaponState* GetWeaponState(uint32_t actorId, uint32_t slot=0) const;

    // Callbacks
    using ShotCb   = std::function<void(const ShotEvent&)>;
    using ReloadCb = std::function<void(const ReloadEvent&)>;
    using DryCb    = std::function<void(uint32_t actorId, uint32_t slot)>;
    void SetProjectileDispatch(ShotCb cb);
    void SetOnReloadComplete(ReloadCb cb);
    void SetOnDryFire(DryCb cb);

    // JSON
    bool LoadTypesFromJSON(const std::string& path);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
