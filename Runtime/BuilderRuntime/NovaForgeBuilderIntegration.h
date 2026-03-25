#pragma once
/**
 * @file NovaForgeBuilderIntegration.h
 * @brief Full in-game builder session management for NovaForge.
 *
 * Ties together ALL builder subsystems into a single game-facing API:
 *   - Builder::PartLibrary    — part catalogue loaded from JSON
 *   - Builder::Assembly       — placed parts + snap/weld links
 *   - Builder::InteriorNode   — interior module slots (rooms, systems)
 *   - Builder::SnapRules      — snap-point compatibility validation
 *   - Builder::CollisionBuilder — per-assembly collision shapes
 *   - Builder::BuilderDamageSystem — per-part HP tracking
 *   - Runtime::BuilderRuntime — in-game place/remove/weld loop
 *
 * Buildable targets:
 *   - Ship       — player-owned ship assembled from hull + module parts
 *   - Station    — player-built orbital station or outpost
 *   - Rover      — ground vehicle built from frame + gear parts
 *   - Structure  — free-form in-world structure (wall, platform, habitat)
 *
 * Typical usage:
 * @code
 *   NovaForgeBuilderIntegration builder;
 *   builder.LoadPartLibrary("Projects/NovaForge/Parts");
 *   builder.LoadShipTemplates("Projects/NovaForge/Ships");
 *
 *   // Start editing the player's active ship
 *   uint32_t sessionId = builder.OpenBuildSession(BuildTargetType::Ship, "Nova Fighter");
 *   builder.SetActive(sessionId);
 *
 *   // Per-frame
 *   builder.Tick(dt);
 *
 *   // Player actions
 *   builder.BeginBuild();
 *   builder.PlacePart(1001, transform);   // starter hull
 *   builder.PlacePart(2001, transform2);  // engine
 *   builder.WeldParts(instA, snapA, instB, snapB);
 *   builder.EndBuild();
 *
 *   // Interior module placement
 *   builder.PlaceModule(slotIndex, mod);
 *
 *   // Persist
 *   builder.SaveSession(sessionId, "Projects/NovaForge/Ships/my_ship.json");
 * @endcode
 */

#include "Builder/Assembly/Assembly.h"
#include "Builder/Parts/PartDef.h"
#include "Builder/SnapRules/SnapRules.h"
#include "Builder/Collision/CollisionShape.h"
#include "Builder/Damage/PartDamage.h"
#include "Builder/InteriorNode/InteriorNode.h"
#include "Runtime/BuilderRuntime/BuilderRuntime.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Runtime {

// ── Enums ─────────────────────────────────────────────────────────────────────

enum class BuildTargetType : uint8_t {
    Ship,
    Station,
    Rover,
    Structure,
};

// ── Build session stats ───────────────────────────────────────────────────────

struct BuildSessionStats {
    uint32_t partCount{0};
    uint32_t linkCount{0};
    uint32_t moduleCount{0};
    uint32_t destroyedParts{0};
    float    totalMass{0.0f};
    float    totalHP{0.0f};
    float    totalPowerDraw{0.0f};
    float    totalPowerOutput{0.0f};
    float    totalThrust{0.0f};
    bool     hasPower{false};
    bool     isValid{false};
};

// ── Crafting gate callback ────────────────────────────────────────────────────

/// Return true if the player has enough materials to place the given part.
/// Called before every PlacePart; returning false cancels the placement.
using CraftingGateFn = std::function<bool(uint32_t partDefId, uint32_t quantity)>;

/// Called when a part is consumed from the player's inventory.
using ConsumePartFn  = std::function<void(uint32_t partDefId, uint32_t quantity)>;

// ── Build session ─────────────────────────────────────────────────────────────

struct BuildSession {
    uint32_t                           id{0};
    BuildTargetType                    type{BuildTargetType::Ship};
    std::string                        name;
    Builder::Assembly                  assembly;
    Builder::InteriorNode              interior;
    Builder::BuilderDamageSystem       damageSystem;
    BuilderRuntime                     runtime;
    bool                               dirty{false};   // unsaved changes
};

// ── Integration event callbacks ───────────────────────────────────────────────

struct BuilderIntegrationCallbacks {
    std::function<void(uint32_t sessionId, uint32_t instanceId)> onPartPlaced;
    std::function<void(uint32_t sessionId, uint32_t instanceId)> onPartRemoved;
    std::function<void(uint32_t sessionId, uint32_t instA, uint32_t instB)> onWelded;
    std::function<void(uint32_t sessionId, uint32_t instA, uint32_t instB)> onUnwelded;
    std::function<void(uint32_t sessionId, const Builder::PartHealthState&)> onPartDestroyed;
    std::function<void(uint32_t sessionId)> onSessionSaved;
    std::function<void(uint32_t sessionId)> onSessionLoaded;
};

// ── NovaForgeBuilderIntegration ────────────────────────────────────────────────

class NovaForgeBuilderIntegration {
public:
    NovaForgeBuilderIntegration();
    ~NovaForgeBuilderIntegration();

    // ── Library loading ───────────────────────────────────────────────────

    /// Load all *.json part definitions from a directory.
    /// Each file must match the PartDef JSON schema (id, name, category, …).
    /// Returns number of parts loaded.
    size_t LoadPartLibrary(const std::string& partsDir);

    /// Load all *.json ship template files from a directory.
    /// Creates a pre-populated BuildSession for each template.
    size_t LoadShipTemplates(const std::string& shipsDir);

    /// Register a single PartDef directly (for runtime-generated parts).
    void   RegisterPart(Builder::PartDef def);

    /// Total parts in the catalogue.
    size_t PartCount() const;

    /// Get a part definition by id (nullptr if not found).
    const Builder::PartDef* GetPartDef(uint32_t id) const;

    /// All parts in a given category.
    std::vector<Builder::PartDef> GetPartsByCategory(Builder::PartCategory cat) const;

    // ── Session management ────────────────────────────────────────────────

    /// Open a new empty build session for the given target type.
    uint32_t OpenBuildSession(BuildTargetType type, const std::string& name = "");

    /// Open a build session and pre-load a saved assembly from file.
    uint32_t LoadBuildSession(const std::string& filePath);

    /// Save the active session (or a specific session by id) to JSON.
    bool SaveSession(uint32_t sessionId, const std::string& filePath);

    /// Close and destroy a session. Unsaved changes are discarded.
    void CloseSession(uint32_t sessionId);

    /// Make a session the "active" one for player interaction.
    void SetActive(uint32_t sessionId);

    uint32_t           ActiveSessionId()  const;
    BuildSession*      GetActiveSession();
    BuildSession*      GetSession(uint32_t sessionId);

    std::vector<uint32_t> AllSessionIds() const;

    // ── Build actions (operate on the active session) ─────────────────────

    /// Enter build mode.
    void BeginBuild();

    /// Exit build mode.
    void EndBuild();

    bool IsBuilding() const;

    /// Place a part at the given 4×4 row-major transform.
    /// Checks snap compatibility and (optionally) the crafting gate.
    /// Returns instance id (0 on failure).
    uint32_t PlacePart(uint32_t defId, const float transform[16]);

    /// Remove a placed part from the active assembly.
    bool RemovePart(uint32_t instanceId);

    /// Weld two snap points together (snap must be compatible).
    bool WeldParts(uint32_t instA, uint32_t snapA, uint32_t instB, uint32_t snapB);

    /// Unweld a snap connection.
    bool UnweldParts(uint32_t instA, uint32_t snapA);

    // ── Interior modules (active session) ────────────────────────────────

    /// Add a module slot to the active session's InteriorNode.
    size_t AddInteriorSlot(const Builder::ModuleSlot& slot);

    /// Place a module in the given slot index.
    bool PlaceModule(size_t slotIndex, const Builder::InteriorModule& mod);

    /// Remove a module from a slot.
    void RemoveModule(size_t slotIndex);

    /// Get the module at a slot (nullptr if empty).
    const Builder::InteriorModule* GetModule(size_t slotIndex) const;

    size_t InteriorSlotCount()    const;
    size_t InteriorModuleCount()  const;

    // ── Snap preview / validation ─────────────────────────────────────────

    /// Find compatible snap candidates for the given part placed near the
    /// active assembly (used for live snap-preview highlight).
    std::vector<Builder::SnapCandidate> FindSnapCandidates(
        uint32_t newDefId, const Builder::SnapPoint& newSnap) const;

    // ── Combat / damage ───────────────────────────────────────────────────

    /// Apply damage to a part in the active session.
    Builder::PartHealthState ApplyDamage(const Builder::DamageEvent& evt);

    /// Repair a part (add HP).
    void RepairPart(uint32_t instanceId, float amount);

    /// Repair all parts in the active assembly.
    void RepairAll();

    /// True if a part has been destroyed.
    bool IsPartDestroyed(uint32_t instanceId) const;

    std::vector<uint32_t> GetDestroyedParts() const;

    // ── Collision shapes ──────────────────────────────────────────────────

    /// Build compound collision shapes for the active assembly.
    std::vector<Builder::CollisionShape> BuildCollision() const;

    // ── Stats ─────────────────────────────────────────────────────────────

    BuildSessionStats CalculateStats(uint32_t sessionId) const;
    BuildSessionStats CalculateActiveStats() const;

    // ── Per-frame tick ────────────────────────────────────────────────────

    void Tick(float dt);

    // ── Snap compatibility setup ──────────────────────────────────────────

    /// Register bidirectional snap type compatibility (e.g. "hull"↔"hull").
    void RegisterSnapCompatibility(const std::string& typeA, const std::string& typeB);

    // ── Crafting gate ─────────────────────────────────────────────────────

    void SetCraftingGate(CraftingGateFn fn);
    void SetConsumePart(ConsumePartFn fn);

    // ── Event callbacks ───────────────────────────────────────────────────

    void SetCallbacks(BuilderIntegrationCallbacks callbacks);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
