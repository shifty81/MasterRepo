#pragma once
/**
 * @file NetworkPacketSystem.h
 * @brief Typed packet serialization, sequence numbers, ACK, fragmentation & reassembly.
 *
 * Features:
 *   - RegisterPacketType(typeId, name) → bool
 *   - CreatePacket(typeId, seqNum) → packetId
 *   - WriteUInt8/16/32/Float/String(packetId, val)
 *   - ReadUInt8/16/32/Float/String(packetId, out&) → bool
 *   - Serialize(packetId, outBytes[], outSize) → uint32_t
 *   - Deserialize(bytes[], size) → packetId
 *   - Fragment(packetId, mtu, outFragments[]) → uint32_t
 *   - Reassemble(fragments[], count) → packetId
 *   - SendAck(seqNum) / IsAcked(seqNum) → bool
 *   - GetNextSeqNum() → uint32_t
 *   - GetPacketTypeId(packetId) → uint32_t
 *   - GetPacketSize(packetId) → uint32_t
 *   - DropPacket(packetId)
 *   - SetOnReceive(typeId, cb): dispatch callback per packet type
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

class NetworkPacketSystem {
public:
    NetworkPacketSystem();
    ~NetworkPacketSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Type registry
    bool RegisterPacketType(uint32_t typeId, const std::string& name);

    // Packet creation
    uint32_t CreatePacket  (uint32_t typeId, uint32_t seqNum = 0);
    void     DropPacket    (uint32_t packetId);

    // Write
    void WriteUInt8 (uint32_t pid, uint8_t  v);
    void WriteUInt16(uint32_t pid, uint16_t v);
    void WriteUInt32(uint32_t pid, uint32_t v);
    void WriteFloat (uint32_t pid, float    v);
    void WriteString(uint32_t pid, const std::string& v);

    // Read (cursor-based sequential read)
    bool ReadUInt8 (uint32_t pid, uint8_t&  out);
    bool ReadUInt16(uint32_t pid, uint16_t& out);
    bool ReadUInt32(uint32_t pid, uint32_t& out);
    bool ReadFloat (uint32_t pid, float&    out);
    bool ReadString(uint32_t pid, std::string& out);

    // Serialization
    uint32_t Serialize  (uint32_t pid, std::vector<uint8_t>& out) const;
    uint32_t Deserialize(const std::vector<uint8_t>& bytes);

    // Fragmentation
    uint32_t Fragment  (uint32_t pid, uint32_t mtu,
                        std::vector<uint32_t>& outFragmentIds);
    uint32_t Reassemble(const std::vector<uint32_t>& fragmentIds);

    // ACK
    void SendAck  (uint32_t seqNum);
    bool IsAcked  (uint32_t seqNum) const;
    uint32_t GetNextSeqNum();

    // Query
    uint32_t GetPacketTypeId(uint32_t pid) const;
    uint32_t GetPacketSize  (uint32_t pid) const;

    // Dispatch
    void SetOnReceive(uint32_t typeId,
                      std::function<void(uint32_t pid)> cb);
    void DispatchReceived(uint32_t pid);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
