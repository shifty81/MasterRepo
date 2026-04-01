#pragma once
#include <string>
#include <cstddef>
#include <cstdint>

namespace NF {

/// @brief Stub TCP socket providing connect/send/receive semantics.
class Socket {
public:
    Socket() noexcept = default;
    ~Socket();

    // Non-copyable.
    Socket(const Socket&)            = delete;
    Socket& operator=(const Socket&) = delete;

    /// @brief Establish a connection to host:port.
    /// @return True on success.
    bool Connect(const std::string& host, uint16_t port);

    /// @brief Send data over the socket.
    /// @param data  Pointer to the data buffer.
    /// @param size  Number of bytes to send.
    /// @return Number of bytes sent, or -1 on error.
    int Send(const void* data, std::size_t size);

    /// @brief Receive data from the socket.
    /// @param buffer   Destination buffer.
    /// @param maxSize  Maximum number of bytes to read.
    /// @return Number of bytes received, or -1 on error.
    int Receive(void* buffer, std::size_t maxSize);

    /// @brief Close the socket connection.
    void Close();

    /// @brief Returns true if the socket is currently connected.
    [[nodiscard]] bool IsConnected() const noexcept;

private:
    bool m_Connected{false};
};

} // namespace NF
