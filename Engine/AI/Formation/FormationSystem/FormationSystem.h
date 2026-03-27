#pragma once
/**
 * @file FormationSystem.h
 * @brief AI formation controller: shape definitions, slot assignment, and maintain/break logic.
 *
 * Features:
 *   - Formation shapes: Line, Wedge, Circle, Box, Column, Echelon
 *   - CreateFormation(shape, slotCount) → formationId
 *   - AssignAgent(formationId, agentId, slotIndex): assign agent to slot
 *   - AutoAssign(formationId, agentIds[]): nearest-slot Hungarian-style assignment
 *   - GetSlotWorldPos(formationId, slotIndex, leaderPos, leaderYaw) → Vec3
 *   - Update(formationId, leaderPos, leaderYaw): recompute slot world positions
 *   - GetTargetPos(agentId) → Vec3: world position the agent should move toward
 *   - SetSpacing(formationId, spacing): distance between slots
 *   - BreakFormation(formationId): release all slot assignments
 *   - DisbandFormation(formationId): remove formation entirely
 *   - SetFormationShape(formationId, shape): change shape on-the-fly
 *   - GetFormationCentre(formationId) → Vec3
 */

#include <cstdint>
#include <vector>

namespace Engine {

enum class FormationShape : uint8_t {
    Line, Wedge, Circle, Box, Column, Echelon
};

struct FVec3 { float x, y, z; };

class FormationSystem {
public:
    FormationSystem();
    ~FormationSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Formation lifecycle
    uint32_t CreateFormation (FormationShape shape, uint32_t slotCount, float spacing=2.f);
    void     DisbandFormation(uint32_t formId);
    void     BreakFormation  (uint32_t formId);

    // Slot assignment
    bool AssignAgent (uint32_t formId, uint32_t agentId, uint32_t slotIndex);
    void AutoAssign  (uint32_t formId, const std::vector<uint32_t>& agentIds);
    void UnassignAgent(uint32_t agentId);

    // Per-frame update
    void Update(uint32_t formId, FVec3 leaderPos, float leaderYaw);

    // Query
    FVec3    GetSlotWorldPos  (uint32_t formId, uint32_t slotIdx,
                               FVec3 leaderPos, float leaderYaw) const;
    FVec3    GetTargetPos     (uint32_t agentId) const;
    FVec3    GetFormationCentre(uint32_t formId) const;
    uint32_t GetSlotCount     (uint32_t formId) const;
    int32_t  GetAgentSlot     (uint32_t agentId) const; ///< -1 if unassigned

    // Config
    void SetSpacing     (uint32_t formId, float spacing);
    void SetFormationShape(uint32_t formId, FormationShape shape);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
