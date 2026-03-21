#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace Runtime {

/// NPC personality / behaviour archetype.
enum class NPCBehaviour {
    Idle,
    Patrol,
    Guard,
    Aggressive,
    Friendly,
    Fleeing,
};

/// Runtime-tunable NPC parameters.
struct NPCParams {
    NPCBehaviour behaviour{NPCBehaviour::Idle};
    float        visionRange{10.0f};   ///< Detection radius
    float        moveSpeed{3.0f};
    float        attackRange{1.5f};
    float        reactionTimeSec{0.3f};
    float        aggressionLevel{0.5f}; ///< 0.0 calm – 1.0 fully aggressive
};

/// Event fired when the NPC changes state.
struct NPCStateEvent {
    uint32_t    npcId{0};
    NPCBehaviour from{NPCBehaviour::Idle};
    NPCBehaviour to{NPCBehaviour::Idle};
    std::string  reason;
};

using NPCStateCallback   = std::function<void(const NPCStateEvent&)>;
using NPCActionCallback  = std::function<void(uint32_t npcId,
                                               const std::string& action)>;

/// NPCController — runtime NPC behaviour controller.
///
/// Manages one or more NPCs.  Behaviour trees are ticked each frame via
/// Update().  The AI chat interface can call TuneParams() mid-session to
/// interactively adjust behaviour without restarting simulation.
class NPCController {
public:
    NPCController();
    ~NPCController();

    // ── NPC management ────────────────────────────────────────
    /// Register an NPC with an initial parameter set.
    void     RegisterNPC(uint32_t npcId, const NPCParams& params);
    /// Remove an NPC from the controller.
    void     UnregisterNPC(uint32_t npcId);
    bool     HasNPC(uint32_t npcId) const;
    std::vector<uint32_t> GetNPCIds() const;

    // ── runtime tuning ────────────────────────────────────────
    /// Update NPC parameters at runtime (AI Chat / interactive tuning).
    void     TuneParams(uint32_t npcId, const NPCParams& params);
    NPCParams GetParams(uint32_t npcId) const;
    /// Force an immediate behaviour transition.
    void     ForceBehaviour(uint32_t npcId, NPCBehaviour behaviour,
                             const std::string& reason = "");

    // ── simulation ────────────────────────────────────────────
    /// Tick all registered NPCs. Call once per game frame.
    void     Update(float deltaTimeSec);
    /// Reset all NPCs to their initial state.
    void     Reset();

    // ── callbacks ─────────────────────────────────────────────
    void OnStateChange(NPCStateCallback cb);
    void OnAction(NPCActionCallback cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
