#pragma once

#include <any>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace Core {

/// Lightweight, copyable message for cross-module communication.
///
/// Unlike the typed Event system, a Message carries an opaque payload
/// (std::any) and is routed by a string-based channel name, making it
/// ideal for loosely-coupled publish/subscribe patterns.
///
/// Usage:
///   Core::Message msg("physics/collision", CollisionData{...}, senderID);
///   bus.Publish(msg);
class Message {
public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    /// Construct a message with channel, payload, and optional sender ID.
    explicit Message(std::string channel,
                     std::any    payload  = {},
                     uint64_t    senderID = 0) noexcept
        : m_Channel(std::move(channel))
        , m_Payload(std::move(payload))
        , m_SenderID(senderID)
        , m_Timestamp(Clock::now()) {}

    // --- Accessors ---

    [[nodiscard]] std::string_view GetChannel()   const noexcept { return m_Channel; }
    [[nodiscard]] const std::any&  GetPayload()   const noexcept { return m_Payload; }
    [[nodiscard]] uint64_t         GetSenderID()  const noexcept { return m_SenderID; }
    [[nodiscard]] TimePoint        GetTimestamp() const noexcept { return m_Timestamp; }

    /// Convenience: retrieve the payload as a concrete type.
    /// Throws std::bad_any_cast if the stored type does not match.
    template <typename T>
    [[nodiscard]] const T& GetPayloadAs() const { return std::any_cast<const T&>(m_Payload); }

    /// Check whether the message carries a payload value.
    [[nodiscard]] bool HasPayload() const noexcept { return m_Payload.has_value(); }

private:
    std::string m_Channel;
    std::any    m_Payload;
    uint64_t    m_SenderID  = 0;
    TimePoint   m_Timestamp{};
};

} // namespace Core
