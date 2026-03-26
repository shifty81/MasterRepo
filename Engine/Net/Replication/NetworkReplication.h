#pragma once
/**
 * @file NetworkReplication.h
 * @brief ECS component replication layer for multiplayer.
 *
 * Sits on top of `Engine/Net/NetContext` and `Runtime/StateSync` to provide
 * high-level, type-safe component replication:
 *   - Per-component dirty tracking and delta serialisation
 *   - Authority model: server-authoritative with client prediction
 *   - Interest management: only replicate entities within a given range
 *   - Priority queue: high-priority entities replicated every tick,
 *     low-priority entities replicated on a slower cadence
 *
 * Typical usage:
 * @code
 *   NetworkReplication rep;
 *   rep.Init();
 *   rep.RegisterComponent("Transform", serializeFn, deserializeFn);
 *   rep.SetAuthority(NetworkAuthority::Server);
 *   rep.MarkDirty(entityId, "Transform");
 *   auto packets = rep.BuildDeltaPackets();   // call each server tick
 *   rep.ApplyIncomingPacket(rawBytes, size);  // call on each client
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

// ── Authority model ───────────────────────────────────────────────────────────

enum class NetworkAuthority : uint8_t { Server = 0, Client = 1, SharedPredicted = 2 };

// ── A serialised component delta packet ──────────────────────────────────────

struct ReplicationPacket {
    uint32_t             entityId{0};
    std::string          componentName;
    std::vector<uint8_t> payload;    ///< binary-serialised component delta
    uint64_t             sequenceNo{0};
    bool                 isFullSnapshot{false};
};

// ── Replication priority ──────────────────────────────────────────────────────

enum class ReplicationPriority : uint8_t { Critical = 0, High = 1, Normal = 2, Low = 3 };

// ── NetworkReplication ────────────────────────────────────────────────────────

class NetworkReplication {
public:
    NetworkReplication();
    ~NetworkReplication();

    void Init();
    void Shutdown();

    // ── Component registration ────────────────────────────────────────────────

    using SerializeFn   = std::function<std::vector<uint8_t>(uint32_t entityId)>;
    using DeserializeFn = std::function<void(uint32_t entityId,
                                             const uint8_t* data, uint32_t size)>;

    void RegisterComponent(const std::string& name,
                           SerializeFn   serializeFn,
                           DeserializeFn deserializeFn,
                           ReplicationPriority priority = ReplicationPriority::Normal);
    void UnregisterComponent(const std::string& name);

    // ── Authority & interest ──────────────────────────────────────────────────

    void SetAuthority(NetworkAuthority authority);
    NetworkAuthority GetAuthority() const;

    void SetInterestRange(float radius);

    // ── Dirty tracking ────────────────────────────────────────────────────────

    void MarkDirty(uint32_t entityId, const std::string& component);
    void MarkAllDirty(uint32_t entityId);
    void ClearDirty(uint32_t entityId, const std::string& component);

    // ── Packet production & consumption ──────────────────────────────────────

    /// Build all pending delta packets (call on server each tick).
    std::vector<ReplicationPacket> BuildDeltaPackets();

    /// Build a full snapshot for a newly connected client.
    std::vector<ReplicationPacket> BuildFullSnapshot(uint32_t clientId);

    /// Apply an incoming packet (call on client each tick).
    void ApplyIncomingPacket(const uint8_t* data, uint32_t size);
    void ApplyPacket(const ReplicationPacket& packet);

    // ── Stats ─────────────────────────────────────────────────────────────────

    uint32_t DirtyComponentCount() const;
    uint64_t BytesSentThisFrame()  const;
    uint64_t BytesRecvThisFrame()  const;
    void     ResetFrameStats();

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnPacketBuilt(std::function<void(const ReplicationPacket&)> cb);
    void OnPacketApplied(std::function<void(const ReplicationPacket&)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
