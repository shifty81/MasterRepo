#include "Game/Net/NetReplicator.h"
#include <cstring>

namespace NF::Game {

// ---------------------------------------------------------------------------
// Server-side: build snapshot
// ---------------------------------------------------------------------------

NetWorldSnapshot NetReplicator::BuildSnapshot(
    uint32_t tick,
    const std::vector<NetPlayerState>& players,
    std::vector<NetVoxelEdit>& pendingEdits)
{
    NetWorldSnapshot snapshot;
    snapshot.Tick        = tick;
    snapshot.SnapshotSeq = m_NextSnapshotSeq++;
    snapshot.Players     = players;
    snapshot.VoxelEdits  = std::move(pendingEdits);
    pendingEdits.clear();
    return snapshot;
}

void NetReplicator::RecordVoxelEdit(int32_t wx, int32_t wy, int32_t wz, VoxelId newType)
{
    m_PendingEdits.push_back({wx, wy, wz, newType, m_NextEditSeq++});
}

// ---------------------------------------------------------------------------
// Client-side: apply snapshot
// ---------------------------------------------------------------------------

bool NetReplicator::ApplySnapshot(const NetWorldSnapshot& snapshot)
{
    // Discard stale snapshots.
    if (snapshot.SnapshotSeq <= m_LastAppliedSeq && m_LastAppliedSeq > 0)
        return false;

    m_LastAppliedSeq = snapshot.SnapshotSeq;
    return true;
}

// ---------------------------------------------------------------------------
// Serialise / deserialise
// ---------------------------------------------------------------------------

std::vector<uint8_t> NetReplicator::SerializeSnapshot(const NetWorldSnapshot& snapshot)
{
    BinaryArchive ar;

    // Header fields.
    uint32_t tick = snapshot.Tick;
    uint32_t seq  = snapshot.SnapshotSeq;
    ar.Serialize(tick);
    ar.Serialize(seq);

    // Player count + player states.
    uint32_t playerCount = static_cast<uint32_t>(snapshot.Players.size());
    ar.Serialize(playerCount);
    for (const auto& p : snapshot.Players) {
        uint32_t  cid  = p.ClientId;
        float     px   = p.Position.X;
        float     py   = p.Position.Y;
        float     pz   = p.Position.Z;
        float     yaw  = p.Yaw;
        float     pit  = p.Pitch;
        float     hp   = p.Health;
        float     en   = p.Energy;
        uint8_t   gnd  = p.Grounded ? 1 : 0;
        uint8_t   tool = p.ToolSlot;

        ar.Serialize(cid);
        ar.Serialize(px);  ar.Serialize(py);  ar.Serialize(pz);
        ar.Serialize(yaw); ar.Serialize(pit);
        ar.Serialize(hp);  ar.Serialize(en);
        ar.Serialize(gnd); ar.Serialize(tool);
    }

    // Voxel edit count + edits.
    uint32_t editCount = static_cast<uint32_t>(snapshot.VoxelEdits.size());
    ar.Serialize(editCount);
    for (const auto& e : snapshot.VoxelEdits) {
        int32_t  wx  = e.WorldX;
        int32_t  wy  = e.WorldY;
        int32_t  wz  = e.WorldZ;
        VoxelId  vt  = e.NewType;
        uint32_t esq = e.Sequence;

        ar.Serialize(wx); ar.Serialize(wy); ar.Serialize(wz);
        ar.Serialize(vt); ar.Serialize(esq);
    }

    // Convert std::byte buffer to uint8_t vector.
    const auto& byteData = ar.Data();
    std::vector<uint8_t> result(byteData.size());
    if (!byteData.empty())
        std::memcpy(result.data(), byteData.data(), byteData.size());
    return result;
}

NetWorldSnapshot NetReplicator::DeserializeSnapshot(const uint8_t* data, size_t size)
{
    BinaryArchive ar(reinterpret_cast<const std::byte*>(data), size);

    NetWorldSnapshot snapshot;
    ar.Serialize(snapshot.Tick);
    ar.Serialize(snapshot.SnapshotSeq);

    uint32_t playerCount = 0;
    ar.Serialize(playerCount);
    snapshot.Players.resize(playerCount);
    for (uint32_t i = 0; i < playerCount; ++i) {
        auto& p = snapshot.Players[i];
        ar.Serialize(p.ClientId);
        ar.Serialize(p.Position.X);
        ar.Serialize(p.Position.Y);
        ar.Serialize(p.Position.Z);
        ar.Serialize(p.Yaw);
        ar.Serialize(p.Pitch);
        ar.Serialize(p.Health);
        ar.Serialize(p.Energy);
        uint8_t gnd = 0;
        ar.Serialize(gnd);
        p.Grounded = (gnd != 0);
        ar.Serialize(p.ToolSlot);
    }

    uint32_t editCount = 0;
    ar.Serialize(editCount);
    snapshot.VoxelEdits.resize(editCount);
    for (uint32_t i = 0; i < editCount; ++i) {
        auto& e = snapshot.VoxelEdits[i];
        ar.Serialize(e.WorldX);
        ar.Serialize(e.WorldY);
        ar.Serialize(e.WorldZ);
        ar.Serialize(e.NewType);
        ar.Serialize(e.Sequence);
    }

    return snapshot;
}

} // namespace NF::Game
