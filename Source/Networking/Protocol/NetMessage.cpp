#include "Networking/Protocol/NetMessage.h"
#include <cstring>
#include <stdexcept>

namespace NF {

// ---------------------------------------------------------------------------
// Construct from received bytes
// ---------------------------------------------------------------------------

NetMessage::NetMessage(const uint8_t* data, size_t size)
{
    if (size < sizeof(NetMessageHeader))
        throw std::runtime_error("NetMessage: data too small for header");

    std::memcpy(&m_Header, data, sizeof(NetMessageHeader));

    if (size < sizeof(NetMessageHeader) + m_Header.PayloadSize)
        throw std::runtime_error("NetMessage: data too small for payload");

    m_Data.assign(data, data + sizeof(NetMessageHeader) + m_Header.PayloadSize);
}

// ---------------------------------------------------------------------------
// Payload access
// ---------------------------------------------------------------------------

std::vector<uint8_t> NetMessage::GetPayload() const
{
    if (m_Data.size() <= sizeof(NetMessageHeader))
        return {};
    return {m_Data.begin() + sizeof(NetMessageHeader), m_Data.end()};
}

// ---------------------------------------------------------------------------
// WriteString
// ---------------------------------------------------------------------------

void NetMessage::WriteString(const std::string& s)
{
    const uint32_t len = static_cast<uint32_t>(s.size());
    Write(len);
    m_Payload.insert(m_Payload.end(),
                     reinterpret_cast<const uint8_t*>(s.data()),
                     reinterpret_cast<const uint8_t*>(s.data()) + len);
}

// ---------------------------------------------------------------------------
// Seal — header + payload → m_Data
// ---------------------------------------------------------------------------

void NetMessage::Seal()
{
    m_Header.PayloadSize = static_cast<uint32_t>(m_Payload.size());
    m_Data.resize(sizeof(NetMessageHeader) + m_Payload.size());
    std::memcpy(m_Data.data(), &m_Header, sizeof(NetMessageHeader));
    if (!m_Payload.empty())
        std::memcpy(m_Data.data() + sizeof(NetMessageHeader),
                    m_Payload.data(), m_Payload.size());
}

// ---------------------------------------------------------------------------
// BeginRead
// ---------------------------------------------------------------------------

BinaryArchive NetMessage::BeginRead() const
{
    if (m_Data.size() <= sizeof(NetMessageHeader))
        return BinaryArchive(nullptr, 0);

    const std::byte* payloadStart =
        reinterpret_cast<const std::byte*>(m_Data.data() + sizeof(NetMessageHeader));
    return BinaryArchive(payloadStart, m_Header.PayloadSize);
}

} // namespace NF
