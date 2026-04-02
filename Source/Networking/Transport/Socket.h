#pragma once
#include <string>
#include <cstddef>
#include <cstdint>

namespace NF {

/// @brief TCP socket with real POSIX / Winsock2 I/O.
///
/// Supports both client-side (Connect) and server-side (Listen/Accept)
/// workflows.  Non-blocking mode is available for server accept loops.
class Socket {
public:
    Socket() noexcept = default;
    ~Socket();

    /// @brief Construct a Socket that wraps an already-connected fd.
    /// Used internally by Accept().
    explicit Socket(int fd) noexcept;

    // Move-only.
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    Socket(const Socket&)            = delete;
    Socket& operator=(const Socket&) = delete;

    // ---- Client-side --------------------------------------------------------

    /// @brief Establish a TCP connection to host:port.
    /// @return True on success.
    bool Connect(const std::string& host, uint16_t port);

    // ---- Server-side --------------------------------------------------------

    /// @brief Bind and listen on the given port (INADDR_ANY).
    /// @param port   TCP port to listen on.
    /// @param backlog Maximum pending connections.
    /// @return True on success.
    bool Listen(uint16_t port, int backlog = 8);

    /// @brief Accept a pending incoming connection.
    ///
    /// Returns a new connected Socket.  In non-blocking mode this may
    /// return an invalid (not-connected) socket if no connection is
    /// pending — check IsConnected() on the result.
    [[nodiscard]] Socket Accept();

    // ---- I/O ----------------------------------------------------------------

    /// @brief Send data over the socket.
    /// @return Number of bytes sent, or -1 on error.
    int Send(const void* data, std::size_t size);

    /// @brief Receive data from the socket.
    /// @return Number of bytes received (0 = peer closed), or -1 on
    ///         error / would-block.
    int Receive(void* buffer, std::size_t maxSize);

    // ---- Control ------------------------------------------------------------

    /// @brief Close the socket.
    void Close();

    /// @brief Set non-blocking mode.
    /// @return True on success.
    bool SetNonBlocking(bool nonBlocking);

    /// @brief Returns true if the socket holds a valid connected (or
    ///        listening) file descriptor.
    [[nodiscard]] bool IsConnected() const noexcept;

    /// @brief Returns the raw file descriptor (-1 if closed).
    [[nodiscard]] int GetFD() const noexcept { return m_FD; }

private:
    int  m_FD{-1};          ///< Platform socket file descriptor.
    bool m_Connected{false};
};

} // namespace NF
