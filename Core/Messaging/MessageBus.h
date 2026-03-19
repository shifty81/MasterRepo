#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Core/Messaging/Message.h"

namespace Core {

/// Opaque handle returned by Subscribe(); pass to Unsubscribe() to remove a handler.
using SubscriptionID = uint64_t;

/// Callback type for message handlers.
using MessageHandler = std::function<void(const Message&)>;

/// Thread-safe, string-channel publish/subscribe message bus.
///
/// MessageBus complements the typed EventBus with a looser coupling model:
/// publishers and subscribers agree only on a channel name (string) and an
/// optional payload format, making it well-suited for cross-module and
/// cross-layer communication where compile-time type coupling is undesirable.
///
/// Wildcard support:
///   Subscribe to "physics/*" to receive messages published on any channel
///   that begins with "physics/" (e.g. "physics/collision", "physics/step").
///   A plain "*" subscription receives every message.
///
/// Usage:
///   Core::MessageBus bus;
///
///   auto id = bus.Subscribe("audio/play", [](const Core::Message& msg) {
///       auto clip = msg.GetPayloadAs<std::string>();
///       PlaySound(clip);
///   });
///
///   bus.Publish(Core::Message{"audio/play", std::string("boom.wav")});
///   bus.Unsubscribe(id);
class MessageBus {
public:
    MessageBus()  = default;
    ~MessageBus() = default;

    MessageBus(const MessageBus&)            = delete;
    MessageBus& operator=(const MessageBus&) = delete;

    /// Register a handler for the given channel.
    /// The channel may end with "/*" for prefix-wildcard matching,
    /// or be exactly "*" to receive all messages.
    /// Returns a SubscriptionID that can be passed to Unsubscribe().
    [[nodiscard]] SubscriptionID Subscribe(const std::string& channel, MessageHandler handler);

    /// Remove a previously registered handler.
    void Unsubscribe(SubscriptionID id);

    /// Publish a message.  All matching handlers are invoked synchronously
    /// on the caller's thread in registration order.
    void Publish(const Message& message);

    /// Return the number of active subscriptions (useful for diagnostics).
    [[nodiscard]] std::size_t GetSubscriptionCount() const;

private:
    // --- Internal types ---

    struct SubscriptionRecord {
        SubscriptionID  ID;
        std::string     Channel;
        MessageHandler  Callback;
    };

    // --- Helpers ---

    /// Determine whether a subscription's channel pattern matches a message channel.
    static bool ChannelMatches(std::string_view pattern, std::string_view channel) noexcept;

    // --- State ---

    mutable std::mutex                   m_Mutex;
    SubscriptionID                       m_NextID = 1;
    /// Exact-match subscriptions keyed by channel for O(1) lookup.
    std::unordered_map<std::string, std::vector<SubscriptionRecord>> m_Exact;
    /// Wildcard subscriptions checked linearly on every publish.
    std::vector<SubscriptionRecord>      m_Wildcards;
    /// Reverse map: SubscriptionID → channel string (for fast unsubscribe).
    std::unordered_map<SubscriptionID, std::string> m_IDToChannel;
};

} // namespace Core
