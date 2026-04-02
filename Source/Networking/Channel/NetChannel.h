#pragma once
#include "Networking/Protocol/NetMessage.h"
#include "Networking/Transport/Socket.h"
#include <vector>
#include <deque>
#include <cstdint>

namespace NF {

/// @brief Framed, reliable, ordered message channel over a Socket.
///
/// Provides length-prefixed message framing on top of the raw byte-stream
/// Socket transport.  Messages are queued for sending and received as
/// complete @c NetMessage objects.
///
/// Wire format for each frame:
/// @code
///   [FrameLength: 4 bytes, uint32_t] [MessageData: FrameLength bytes]
/// @endcode
///
/// The FrameLength includes the 6-byte NetMessageHeader plus the payload.
class NetChannel {
public:
    NetChannel() = default;
    ~NetChannel() = default;

    // Non-copyable.
    NetChannel(const NetChannel&)            = delete;
    NetChannel& operator=(const NetChannel&) = delete;

    // ---- Connection ---------------------------------------------------------

    /// @brief Attach this channel to an existing, connected socket.
    ///
    /// Takes ownership of the socket reference (non-owning pointer).
    /// The socket must already be connected.
    void Attach(Socket* socket) noexcept { m_Socket = socket; }

    /// @brief Returns true when the underlying socket is connected.
    [[nodiscard]] bool IsConnected() const noexcept {
        return m_Socket && m_Socket->IsConnected();
    }

    // ---- Sending ------------------------------------------------------------

    /// @brief Queue a message for sending.  Call @c Flush() to actually send.
    void QueueSend(const NetMessage& msg);

    /// @brief Send all queued messages over the socket.
    /// @return Number of messages flushed, or -1 on error.
    int Flush();

    // ---- Receiving ----------------------------------------------------------

    /// @brief Poll the socket for incoming data and assemble complete messages.
    ///
    /// Call this once per tick.  Complete messages are placed in the receive
    /// queue and can be retrieved via @c PopReceived().
    /// @return Number of complete messages received this tick.
    int Poll();

    /// @brief Pop the next received message (FIFO).
    /// @param outMsg Populated with the received message.
    /// @return True if a message was available.
    bool PopReceived(NetMessage& outMsg);

    /// @brief Number of pending received messages.
    [[nodiscard]] size_t ReceivedCount() const noexcept { return m_RecvQueue.size(); }

    // ---- Stats --------------------------------------------------------------

    /// @brief Total bytes sent since channel creation.
    [[nodiscard]] uint64_t BytesSent()     const noexcept { return m_BytesSent; }

    /// @brief Total bytes received since channel creation.
    [[nodiscard]] uint64_t BytesReceived() const noexcept { return m_BytesReceived; }

private:
    Socket* m_Socket{nullptr};

    // Send queue: accumulated raw bytes ready for socket write.
    std::vector<uint8_t> m_SendBuffer;

    // Receive assembly: partial frame data waiting for completion.
    std::vector<uint8_t> m_RecvBuffer;

    // Complete received messages.
    std::deque<NetMessage> m_RecvQueue;

    uint64_t m_BytesSent{0};
    uint64_t m_BytesReceived{0};

    /// @brief Try to parse complete frames from the receive buffer.
    void AssembleFrames();
};

} // namespace NF
