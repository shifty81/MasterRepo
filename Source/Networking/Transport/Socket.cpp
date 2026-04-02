#include "Networking/Transport/Socket.h"

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")

  // Map POSIX names.
  using ssize_t = int;
  #define NF_CLOSE_SOCKET ::closesocket
  #define NF_INVALID_SOCKET INVALID_SOCKET
  static int nf_set_nonblocking(int fd, bool nb) {
      unsigned long mode = nb ? 1 : 0;
      return ioctlsocket(static_cast<SOCKET>(fd), FIONBIO, &mode) == 0 ? 0 : -1;
  }
  static void nf_platform_init() {
      static bool once = false;
      if (!once) {
          WSADATA wd;
          WSAStartup(MAKEWORD(2, 2), &wd);
          once = true;
      }
  }
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <cerrno>

  #define NF_CLOSE_SOCKET ::close
  #define NF_INVALID_SOCKET (-1)
  static int nf_set_nonblocking(int fd, bool nb) {
      int flags = fcntl(fd, F_GETFL, 0);
      if (flags < 0) return -1;
      if (nb) flags |= O_NONBLOCK;
      else    flags &= ~O_NONBLOCK;
      return fcntl(fd, F_SETFL, flags);
  }
  static void nf_platform_init() { /* no-op on POSIX */ }
#endif

#include <cstring>

namespace NF {

// ---------------------------------------------------------------------------
// Constructors / Move / Destructor
// ---------------------------------------------------------------------------

Socket::Socket(int fd) noexcept : m_FD(fd), m_Connected(fd >= 0) {}

Socket::Socket(Socket&& other) noexcept
    : m_FD(other.m_FD), m_Connected(other.m_Connected)
{
    other.m_FD = -1;
    other.m_Connected = false;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        Close();
        m_FD        = other.m_FD;
        m_Connected = other.m_Connected;
        other.m_FD        = -1;
        other.m_Connected = false;
    }
    return *this;
}

Socket::~Socket() {
    Close();
}

// ---------------------------------------------------------------------------
// Connect (client-side)
// ---------------------------------------------------------------------------

bool Socket::Connect(const std::string& host, uint16_t port) {
    nf_platform_init();

    struct addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* result = nullptr;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0)
        return false;

    int fd = static_cast<int>(::socket(result->ai_family, result->ai_socktype, result->ai_protocol));
    if (fd < 0) { freeaddrinfo(result); return false; }

    // Disable Nagle for low latency.
    int nodelay = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
               reinterpret_cast<const char*>(&nodelay), sizeof(nodelay));

    if (::connect(fd, result->ai_addr, static_cast<int>(result->ai_addrlen)) != 0) {
        NF_CLOSE_SOCKET(fd);
        freeaddrinfo(result);
        return false;
    }

    freeaddrinfo(result);
    m_FD = fd;
    m_Connected = true;
    return true;
}

// ---------------------------------------------------------------------------
// Listen / Accept (server-side)
// ---------------------------------------------------------------------------

bool Socket::Listen(uint16_t port, int backlog) {
    nf_platform_init();

    int fd = static_cast<int>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (fd < 0) return false;

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (::bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
        NF_CLOSE_SOCKET(fd);
        return false;
    }

    if (::listen(fd, backlog) != 0) {
        NF_CLOSE_SOCKET(fd);
        return false;
    }

    m_FD = fd;
    m_Connected = true; // "connected" in the sense that it's a valid active socket
    return true;
}

Socket Socket::Accept() {
    if (m_FD < 0) return Socket{};

    struct sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);

    int clientFd = static_cast<int>(
        ::accept(m_FD, reinterpret_cast<struct sockaddr*>(&clientAddr), &addrLen));

    if (clientFd < 0) return Socket{};

    // Disable Nagle on accepted socket.
    int nodelay = 1;
    setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY,
               reinterpret_cast<const char*>(&nodelay), sizeof(nodelay));

    return Socket{clientFd};
}

// ---------------------------------------------------------------------------
// I/O
// ---------------------------------------------------------------------------

int Socket::Send(const void* data, std::size_t size) {
    if (m_FD < 0 || !m_Connected) return -1;

    auto sent = ::send(m_FD, static_cast<const char*>(data),
                       static_cast<int>(size), 0);
    if (sent < 0) {
#ifdef _WIN32
        if (WSAGetLastError() == WSAEWOULDBLOCK) return 0;
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
#endif
        m_Connected = false;
        return -1;
    }
    return static_cast<int>(sent);
}

int Socket::Receive(void* buffer, std::size_t maxSize) {
    if (m_FD < 0 || !m_Connected) return -1;

    auto received = ::recv(m_FD, static_cast<char*>(buffer),
                           static_cast<int>(maxSize), 0);
    if (received < 0) {
#ifdef _WIN32
        if (WSAGetLastError() == WSAEWOULDBLOCK) return -1;
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -1;
#endif
        m_Connected = false;
        return -1;
    }
    if (received == 0) {
        // Peer closed connection.
        m_Connected = false;
        return 0;
    }
    return static_cast<int>(received);
}

// ---------------------------------------------------------------------------
// Control
// ---------------------------------------------------------------------------

void Socket::Close() {
    if (m_FD >= 0) {
        NF_CLOSE_SOCKET(m_FD);
        m_FD = -1;
    }
    m_Connected = false;
}

bool Socket::SetNonBlocking(bool nonBlocking) {
    if (m_FD < 0) return false;
    return nf_set_nonblocking(m_FD, nonBlocking) == 0;
}

bool Socket::IsConnected() const noexcept {
    return m_Connected;
}

} // namespace NF
