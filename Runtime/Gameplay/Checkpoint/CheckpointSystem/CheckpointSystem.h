#pragma once
/**
 * @file CheckpointSystem.h
 * @brief Save/restore game-state checkpoints with named snapshots and respawn logic.
 *
 * Features:
 *   - RegisterCheckpoint(id, pos, label): add a checkpoint location to the world
 *   - ActivateCheckpoint(id): mark as last activated, call OnActivate callback
 *   - SaveSnapshot(tag, blob[], size): store arbitrary state blob under a tag
 *   - LoadSnapshot(tag, outBlob[], outSize) → bool: restore previously saved blob
 *   - GetActiveCheckpointId() → uint32_t
 *   - GetActiveCheckpointPos() → Vec3
 *   - RespawnAt(agentId): fire OnRespawn callback with active checkpoint pos
 *   - GetCheckpointPos(id) → Vec3
 *   - GetCheckpointLabel(id) → string
 *   - GetCheckpointCount() → uint32_t
 *   - IsActivated(id) → bool
 *   - SetOnActivate(cb): callback(checkpointId)
 *   - SetOnRespawn(cb): callback(agentId, pos)
 *   - ClearSnapshots()
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct CPVec3 { float x,y,z; };

class CheckpointSystem {
public:
    CheckpointSystem();
    ~CheckpointSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Checkpoint registry
    void RegisterCheckpoint(uint32_t id, CPVec3 pos, const std::string& label="");
    void ActivateCheckpoint(uint32_t id);

    // Snapshot store
    void SaveSnapshot(const std::string& tag, const uint8_t* blob, uint32_t size);
    bool LoadSnapshot(const std::string& tag, std::vector<uint8_t>& outBlob) const;
    void ClearSnapshots();

    // Query
    uint32_t    GetActiveCheckpointId () const;
    CPVec3      GetActiveCheckpointPos() const;
    CPVec3      GetCheckpointPos      (uint32_t id) const;
    std::string GetCheckpointLabel    (uint32_t id) const;
    uint32_t    GetCheckpointCount    () const;
    bool        IsActivated           (uint32_t id) const;

    // Respawn
    void RespawnAt(uint32_t agentId);

    // Callbacks
    void SetOnActivate(std::function<void(uint32_t checkpointId)> cb);
    void SetOnRespawn (std::function<void(uint32_t agentId, CPVec3 pos)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
