#pragma once
#include "Core/Serialization/Archive.h"
#include <cstdint>
#include <vector>
#include <string>

namespace NF {

// ---------------------------------------------------------------------------
// Message type identifiers — each message in the multiplayer protocol has a
// unique type so the receiver can decode it correctly.
// ---------------------------------------------------------------------------

/// @brief Enumeration of all multiplayer message types.
enum class NetMessageType : uint16_t {
    None              = 0,

    // ---- Handshake / session ----
    ClientHello       = 1,   ///< Client → Server: request to join.
    ServerWelcome     = 2,   ///< Server → Client: assign client id.
    ClientDisconnect  = 3,   ///< Client → Server: graceful leave.
    ServerKick        = 4,   ///< Server → Client: forced disconnect.

    // ---- Input replication ----
    ClientInput       = 10,  ///< Client → Server: per-frame input snapshot.

    // ---- State replication ----
    ServerSnapshot    = 20,  ///< Server → Client: full/delta world snapshot.
    ServerVoxelEdit   = 21,  ///< Server → Client: authoritative voxel edit.
    ServerPlayerState = 22,  ///< Server → Client: single player state update.

    // ---- Chat / debug ----
    ChatMessage       = 50,  ///< Bidirectional chat text.
    ServerPing        = 60,  ///< Server → Client: keepalive ping.
    ClientPong        = 61,  ///< Client → Server: keepalive response.
};

// ---------------------------------------------------------------------------
// Wire header — every message starts with these 6 bytes.
// ---------------------------------------------------------------------------

/// @brief Fixed-size header prepended to every network message.
///
/// Layout (little-endian): [Type: 2 bytes] [Pad: 2 bytes] [PayloadSize: 4 bytes]
struct NetMessageHeader {
    NetMessageType Type{NetMessageType::None};
    uint16_t       _pad{0};
    uint32_t       PayloadSize{0};
};

static_assert(sizeof(NetMessageHeader) == 8, "NetMessageHeader must be 8 bytes");

// ---------------------------------------------------------------------------
// NetMessage — a complete message with header + payload.
// ---------------------------------------------------------------------------

/// @brief A complete network message ready for send / receive.
///
/// Use @c Write() to serialise a message from its fields, then call
/// @c GetData() to get the raw bytes (header + payload).  On the
/// receiving side, construct from raw bytes and call @c ReadHeader().
class NetMessage {
public:
    /// @brief Construct an empty outbound message of the given type.
    explicit NetMessage(NetMessageType type = NetMessageType::None) noexcept
        : m_Header{type, 0} {}

    /// @brief Construct a message from received bytes (header + payload).
    /// @param data  Pointer to the full message data.
    /// @param size  Size in bytes.
    NetMessage(const uint8_t* data, size_t size);

    // ---- Accessors ----------------------------------------------------------

    [[nodiscard]] NetMessageType GetType()        const noexcept { return m_Header.Type; }
    [[nodiscard]] uint32_t       GetPayloadSize() const noexcept { return m_Header.PayloadSize; }
    [[nodiscard]] const NetMessageHeader& GetHeader() const noexcept { return m_Header; }

    /// @brief Full serialised bytes (header + payload).
    [[nodiscard]] const std::vector<uint8_t>& GetData() const noexcept { return m_Data; }

    /// @brief Payload bytes (after the header).  Empty if no payload.
    [[nodiscard]] std::vector<uint8_t> GetPayload() const;

    // ---- Serialisation helpers -----------------------------------------------

    /// @brief Write a trivially-copyable value into the payload.
    template<typename T>
    requires std::is_trivially_copyable_v<T>
    void Write(const T& val) {
        const auto* src = reinterpret_cast<const uint8_t*>(&val);
        m_Payload.insert(m_Payload.end(), src, src + sizeof(T));
    }

    /// @brief Write a string as [uint32_t length][chars].
    void WriteString(const std::string& s);

    /// @brief Finalise the message: prepend header with correct PayloadSize.
    ///
    /// Must be called after all Write*() calls and before GetData().
    void Seal();

    // ---- Deserialisation helpers ---------------------------------------------

    /// @brief Begin reading payload.  Returns a BinaryArchive in read mode
    ///        positioned at the start of the payload.
    [[nodiscard]] BinaryArchive BeginRead() const;

private:
    NetMessageHeader      m_Header;
    std::vector<uint8_t>  m_Payload; ///< Payload-only buffer (before Seal).
    std::vector<uint8_t>  m_Data;    ///< Full wire data (after Seal / received).
};

// ---------------------------------------------------------------------------
// Common message payload helpers — inline for convenience.
// ---------------------------------------------------------------------------

/// @brief Build a ClientHello message carrying a player name.
[[nodiscard]] inline NetMessage MakeClientHello(const std::string& playerName)
{
    NetMessage msg(NetMessageType::ClientHello);
    msg.WriteString(playerName);
    msg.Seal();
    return msg;
}

/// @brief Build a ServerWelcome message assigning a client id.
[[nodiscard]] inline NetMessage MakeServerWelcome(uint32_t clientId)
{
    NetMessage msg(NetMessageType::ServerWelcome);
    msg.Write(clientId);
    msg.Seal();
    return msg;
}

/// @brief Build a ServerPing keepalive.
[[nodiscard]] inline NetMessage MakeServerPing(uint32_t seq)
{
    NetMessage msg(NetMessageType::ServerPing);
    msg.Write(seq);
    msg.Seal();
    return msg;
}

/// @brief Build a ClientPong keepalive response.
[[nodiscard]] inline NetMessage MakeClientPong(uint32_t seq)
{
    NetMessage msg(NetMessageType::ClientPong);
    msg.Write(seq);
    msg.Seal();
    return msg;
}

} // namespace NF
