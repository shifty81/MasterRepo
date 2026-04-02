#include "Networking/Channel/NetChannel.h"
#include <cstring>

namespace NF {

// ---------------------------------------------------------------------------
// Sending
// ---------------------------------------------------------------------------

void NetChannel::QueueSend(const NetMessage& msg)
{
    const auto& data = msg.GetData();
    if (data.empty()) return;

    // Frame: [uint32_t length] [message data]
    const uint32_t frameLen = static_cast<uint32_t>(data.size());
    const auto* lenBytes = reinterpret_cast<const uint8_t*>(&frameLen);
    m_SendBuffer.insert(m_SendBuffer.end(), lenBytes, lenBytes + sizeof(uint32_t));
    m_SendBuffer.insert(m_SendBuffer.end(), data.begin(), data.end());
}

int NetChannel::Flush()
{
    if (!m_Socket || !m_Socket->IsConnected()) return -1;
    if (m_SendBuffer.empty()) return 0;

    const int sent = m_Socket->Send(m_SendBuffer.data(), m_SendBuffer.size());
    if (sent < 0) return -1;

    m_BytesSent += m_SendBuffer.size();
    m_SendBuffer.clear();
    return 1;
}

// ---------------------------------------------------------------------------
// Receiving
// ---------------------------------------------------------------------------

int NetChannel::Poll()
{
    if (!m_Socket || !m_Socket->IsConnected()) return 0;

    // Read available data from socket into receive buffer.
    uint8_t tmp[4096];
    const int bytesRead = m_Socket->Receive(tmp, sizeof(tmp));
    if (bytesRead > 0) {
        m_RecvBuffer.insert(m_RecvBuffer.end(), tmp, tmp + bytesRead);
        m_BytesReceived += static_cast<uint64_t>(bytesRead);
    }

    const size_t before = m_RecvQueue.size();
    AssembleFrames();
    return static_cast<int>(m_RecvQueue.size() - before);
}

bool NetChannel::PopReceived(NetMessage& outMsg)
{
    if (m_RecvQueue.empty()) return false;
    outMsg = std::move(m_RecvQueue.front());
    m_RecvQueue.pop_front();
    return true;
}

// ---------------------------------------------------------------------------
// Frame assembly
// ---------------------------------------------------------------------------

void NetChannel::AssembleFrames()
{
    // Parse [uint32_t frameLen][frameLen bytes of message data]...
    while (m_RecvBuffer.size() >= sizeof(uint32_t))
    {
        uint32_t frameLen = 0;
        std::memcpy(&frameLen, m_RecvBuffer.data(), sizeof(uint32_t));

        const size_t totalNeeded = sizeof(uint32_t) + frameLen;
        if (m_RecvBuffer.size() < totalNeeded)
            break; // Not enough data yet.

        // We have a complete frame — parse the message.
        const uint8_t* msgData = m_RecvBuffer.data() + sizeof(uint32_t);
        m_RecvQueue.emplace_back(msgData, frameLen);

        // Remove consumed bytes.
        m_RecvBuffer.erase(m_RecvBuffer.begin(),
                           m_RecvBuffer.begin() + static_cast<ptrdiff_t>(totalNeeded));
    }
}

} // namespace NF
