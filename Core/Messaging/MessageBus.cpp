#include "Core/Messaging/MessageBus.h"

#include <algorithm>

namespace Core {

// ---------------------------------------------------------------------------
// Subscribe
// ---------------------------------------------------------------------------

SubscriptionID MessageBus::Subscribe(const std::string& channel, MessageHandler handler)
{
    std::lock_guard lock(m_Mutex);

    const SubscriptionID id = m_NextID++;

    SubscriptionRecord record{ id, channel, std::move(handler) };

    const bool isWildcard = (channel == "*") ||
                            (channel.size() >= 2 && channel.compare(channel.size() - 2, 2, "/*") == 0);

    if (isWildcard) {
        m_Wildcards.push_back(std::move(record));
    } else {
        m_Exact[channel].push_back(std::move(record));
    }

    m_IDToChannel[id] = channel;
    return id;
}

// ---------------------------------------------------------------------------
// Unsubscribe
// ---------------------------------------------------------------------------

void MessageBus::Unsubscribe(SubscriptionID id)
{
    std::lock_guard lock(m_Mutex);

    auto it = m_IDToChannel.find(id);
    if (it == m_IDToChannel.end()) return;

    const std::string& channel = it->second;

    const bool isWildcard = (channel == "*") ||
                            (channel.size() >= 2 && channel.compare(channel.size() - 2, 2, "/*") == 0);

    if (isWildcard) {
        std::erase_if(m_Wildcards, [id](const SubscriptionRecord& r) { return r.ID == id; });
    } else {
        auto mapIt = m_Exact.find(channel);
        if (mapIt != m_Exact.end()) {
            auto& vec = mapIt->second;
            std::erase_if(vec, [id](const SubscriptionRecord& r) { return r.ID == id; });
            if (vec.empty()) m_Exact.erase(mapIt);
        }
    }

    m_IDToChannel.erase(it);
}

// ---------------------------------------------------------------------------
// Publish
// ---------------------------------------------------------------------------

void MessageBus::Publish(const Message& message)
{
    std::vector<MessageHandler> toInvoke;

    {
        std::lock_guard lock(m_Mutex);

        const std::string_view channel = message.GetChannel();

        // Collect exact-match handlers.
        if (auto it = m_Exact.find(std::string(channel)); it != m_Exact.end()) {
            for (const auto& rec : it->second) {
                toInvoke.push_back(rec.Callback);
            }
        }

        // Collect wildcard handlers.
        for (const auto& rec : m_Wildcards) {
            if (ChannelMatches(rec.Channel, channel)) {
                toInvoke.push_back(rec.Callback);
            }
        }
    }

    // Invoke outside the lock to avoid deadlocks if a handler publishes.
    for (const auto& handler : toInvoke) {
        handler(message);
    }
}

// ---------------------------------------------------------------------------
// GetSubscriptionCount
// ---------------------------------------------------------------------------

std::size_t MessageBus::GetSubscriptionCount() const
{
    std::lock_guard lock(m_Mutex);
    return m_IDToChannel.size();
}

// ---------------------------------------------------------------------------
// ChannelMatches  (static, noexcept)
// ---------------------------------------------------------------------------

bool MessageBus::ChannelMatches(std::string_view pattern, std::string_view channel) noexcept
{
    // "*" matches everything.
    if (pattern == "*") return true;

    // "prefix/*" matches any channel starting with "prefix/".
    if (pattern.size() >= 2 && pattern.substr(pattern.size() - 2) == "/*") {
        const std::string_view prefix = pattern.substr(0, pattern.size() - 1); // keep trailing '/'
        return channel.size() >= prefix.size() &&
               channel.substr(0, prefix.size()) == prefix;
    }

    // Exact match fallback.
    return pattern == channel;
}

} // namespace Core
