#pragma once
#include "Core/Math/Vector.h"
#include "Core/Serialization/Archive.h"
#include "Game/Voxel/VoxelType.h"
#include <cstdint>
#include <vector>

namespace NF::Game {

// ---------------------------------------------------------------------------
// Replicated player state — compact snapshot for network transmission.
// ---------------------------------------------------------------------------

/// @brief Compact replicated state for a single player.
struct NetPlayerState {
    uint32_t  ClientId{0};       ///< Unique client identifier.
    NF::Vector3 Position;        ///< Feet position in world space.
    float     Yaw{0.f};          ///< Camera yaw (radians).
    float     Pitch{0.f};        ///< Camera pitch (radians).
    float     Health{100.f};     ///< Current rig health.
    float     Energy{100.f};     ///< Current rig energy.
    bool      Grounded{false};   ///< On the ground?
    uint8_t   ToolSlot{0};       ///< Active tool index.
};

// ---------------------------------------------------------------------------
// Replicated voxel edit — a single authoritative voxel change.
// ---------------------------------------------------------------------------

/// @brief A single voxel edit to be replicated to all clients.
struct NetVoxelEdit {
    int32_t WorldX{0};
    int32_t WorldY{0};
    int32_t WorldZ{0};
    VoxelId NewType{0};          ///< The new voxel type at this position.
    uint32_t Sequence{0};        ///< Monotonic edit number for ordering.
};

// ---------------------------------------------------------------------------
// World snapshot — delta or full.
// ---------------------------------------------------------------------------

/// @brief A complete or delta world snapshot broadcast from server to clients.
struct NetWorldSnapshot {
    uint32_t Tick{0};            ///< Server tick number.
    uint32_t SnapshotSeq{0};     ///< Monotonic snapshot sequence.
    std::vector<NetPlayerState> Players;   ///< All player states.
    std::vector<NetVoxelEdit>   VoxelEdits; ///< Voxel changes since last snapshot.
};

// ---------------------------------------------------------------------------
// Client input — per-frame input sent from client to server.
// ---------------------------------------------------------------------------

/// @brief Per-frame input snapshot sent from client to server.
struct NetClientInput {
    uint32_t ClientId{0};
    float    Forward{0.f};       ///< Movement forward/back [-1,1].
    float    Right{0.f};         ///< Movement strafe [-1,1].
    bool     Jump{false};
    bool     Sprint{false};
    float    MouseDeltaX{0.f};   ///< Mouse look delta X.
    float    MouseDeltaY{0.f};   ///< Mouse look delta Y.
    uint32_t InputSeq{0};        ///< Client-side input sequence for reconciliation.
};

// ---------------------------------------------------------------------------
// NetReplicator — server-side snapshot builder, client-side state applicator.
// ---------------------------------------------------------------------------

/// @brief Builds snapshots from the authoritative world and applies them on clients.
///
/// On the server: call @c BuildSnapshot() each tick to capture the current
/// world state into a @c NetWorldSnapshot.
///
/// On the client: call @c ApplySnapshot() to merge a received snapshot
/// into the local world prediction.
class NetReplicator {
public:
    NetReplicator() = default;

    // ---- Server-side --------------------------------------------------------

    /// @brief Capture a full snapshot of the current world state.
    ///
    /// @param tick          Current server tick number.
    /// @param players       All connected player states.
    /// @param pendingEdits  Voxel edits since the last snapshot (consumed).
    /// @return A @c NetWorldSnapshot ready for serialisation and broadcast.
    [[nodiscard]] NetWorldSnapshot BuildSnapshot(
        uint32_t tick,
        const std::vector<NetPlayerState>& players,
        std::vector<NetVoxelEdit>& pendingEdits);

    /// @brief Record a voxel edit for inclusion in the next snapshot.
    void RecordVoxelEdit(int32_t wx, int32_t wy, int32_t wz, VoxelId newType);

    // ---- Client-side --------------------------------------------------------

    /// @brief Apply a received snapshot.  Returns false if it is stale.
    bool ApplySnapshot(const NetWorldSnapshot& snapshot);

    // ---- Serialisation ------------------------------------------------------

    /// @brief Serialise a snapshot into raw bytes for network transmission.
    [[nodiscard]] static std::vector<uint8_t> SerializeSnapshot(
        const NetWorldSnapshot& snapshot);

    /// @brief Deserialise a snapshot from raw bytes.
    [[nodiscard]] static NetWorldSnapshot DeserializeSnapshot(
        const uint8_t* data, size_t size);

    // ---- Accessors ----------------------------------------------------------

    /// @brief Last applied snapshot sequence (client-side).
    [[nodiscard]] uint32_t GetLastAppliedSeq() const noexcept { return m_LastAppliedSeq; }

    /// @brief Pending voxel edits waiting to be included in the next snapshot.
    [[nodiscard]] const std::vector<NetVoxelEdit>& GetPendingEdits() const noexcept {
        return m_PendingEdits;
    }

private:
    uint32_t m_NextSnapshotSeq{0};
    uint32_t m_LastAppliedSeq{0};
    uint32_t m_NextEditSeq{0};
    std::vector<NetVoxelEdit> m_PendingEdits;
};

} // namespace NF::Game
