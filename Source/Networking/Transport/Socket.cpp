#include "Networking/Transport/Socket.h"

namespace NF {

Socket::~Socket() {
    Close();
}

bool Socket::Connect(const std::string& /*host*/, uint16_t /*port*/) {
    // Stub: a real implementation would call platform socket APIs here.
    m_Connected = true;
    return true;
}

int Socket::Send(const void* /*data*/, std::size_t /*size*/) {
    if (!m_Connected) return -1;
    // Stub: returns success without actually sending.
    return 0;
}

int Socket::Receive(void* /*buffer*/, std::size_t /*maxSize*/) {
    if (!m_Connected) return -1;
    // Stub: returns 0 bytes received.
    return 0;
}

void Socket::Close() {
    m_Connected = false;
}

bool Socket::IsConnected() const noexcept {
    return m_Connected;
}

} // namespace NF
