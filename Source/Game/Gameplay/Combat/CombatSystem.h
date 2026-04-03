#pragma once
#include <cstdint>
#include <functional>
#include <vector>

namespace NF::Game::Gameplay {

/// @brief Damage classification for mitigation calculations.
enum class DamageType : uint8_t {
    Physical  = 0,
    Energy    = 1,
    Explosive = 2,
    Radiation = 3,
};

/// @brief Result of a DealDamage call.
enum class DamageResult : uint8_t {
    Hit,         ///< Damage was applied; entity survived.
    Killed,      ///< Damage was applied; entity is now dead.
    AlreadyDead, ///< Entity was already dead; no change.
    NotFound,    ///< Unknown entity id.
};

/// @brief Per-entity combat state.
struct CombatState {
    float    health{100.f};     ///< Current hit-points.
    float    maxHealth{100.f};  ///< Maximum hit-points.
    float    armor{0.f};        ///< Flat damage reduction (physical only).
    bool     alive{true};
};

/// @brief Lightweight combat system — damage model, health, death, and respawn.
///
/// Each entity is registered by caller-supplied ID (typically matches the
/// ECS entity ID).  DealDamage applies mitigation and optionally fires the
/// death callback.  Respawn resets an entity to full health at the same stats.
class CombatSystem {
public:
    CombatSystem() = default;

    // -------------------------------------------------------------------------
    // Entity registration
    // -------------------------------------------------------------------------

    /// @brief Register an entity with the given base stats.
    ///        If the id is already registered the call is ignored.
    void RegisterEntity(uint32_t entityId,
                        float maxHealth = 100.f,
                        float armor     = 0.f);

    /// @brief Remove an entity from tracking.
    bool UnregisterEntity(uint32_t entityId);

    // -------------------------------------------------------------------------
    // Combat actions
    // -------------------------------------------------------------------------

    /// @brief Apply damage to an entity after mitigation.
    ///
    /// Physical damage is reduced by armor (min 0). Energy, Explosive, and
    /// Radiation damage bypass armor entirely.  Calls the death callback
    /// when health first reaches 0.
    DamageResult DealDamage(uint32_t entityId, float amount, DamageType type);

    /// @brief Directly heal an entity.
    /// @return False if entity not found or already dead.
    bool Heal(uint32_t entityId, float amount);

    // -------------------------------------------------------------------------
    // State queries
    // -------------------------------------------------------------------------

    [[nodiscard]] bool IsAlive(uint32_t entityId) const noexcept;

    /// @brief Return a copy of the entity's combat state, or a default-zero
    ///        struct (alive=false) when not found.
    [[nodiscard]] CombatState GetState(uint32_t entityId) const noexcept;

    [[nodiscard]] int EntityCount() const noexcept {
        return static_cast<int>(m_Entities.size());
    }

    // -------------------------------------------------------------------------
    // Respawn
    // -------------------------------------------------------------------------

    /// @brief Respawn a dead entity — restores full health and sets alive=true.
    /// @return False if entity not found or already alive.
    bool Respawn(uint32_t entityId);

    // -------------------------------------------------------------------------
    // Callbacks
    // -------------------------------------------------------------------------

    /// @brief Set a callback fired each time an entity is killed.
    ///        Called with the entity id of the slain entity.
    void SetDeathCallback(std::function<void(uint32_t)> cb) {
        m_DeathCb = std::move(cb);
    }

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------
    void Clear() noexcept;

private:
    struct Entry {
        uint32_t    id{0};
        CombatState state;
    };

    std::vector<Entry> m_Entities;
    std::function<void(uint32_t)> m_DeathCb;

    [[nodiscard]] Entry*       FindEntry(uint32_t id)       noexcept;
    [[nodiscard]] const Entry* FindEntry(uint32_t id) const noexcept;
};

} // namespace NF::Game::Gameplay
