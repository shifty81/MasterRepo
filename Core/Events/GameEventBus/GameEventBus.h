#pragma once
/**
 * @file GameEventBus.h
 * @brief High-performance, type-safe game event bus with priorities and filters.
 *
 * The GameEventBus is a central publish-subscribe dispatcher for in-game
 * events.  Events are plain structs identified by a string or integer channel.
 *
 * Features:
 *   - Subscribe with handler functor (or lambda)
 *   - Unsubscribe by returned token
 *   - One-shot subscriptions (auto-unsubscribe after first fire)
 *   - Deferred posting (queued, dispatched on FlushDeferred())
 *   - Filter predicate per subscription
 *   - Priority ordering (higher = called first)
 *   - Thread-safe posting (queue + mutex)
 *
 * Typical usage:
 * @code
 *   GameEventBus bus;
 *   bus.Init();
 *
 *   auto token = bus.Subscribe("player_damaged",
 *       [](const GameEvent& e){ UpdateHealthBar(e.GetFloat("amount")); });
 *
 *   GameEvent e("player_damaged");
 *   e.Set("amount", 25.f);
 *   bus.Post(e);
 *   bus.FlushDeferred();
 *   bus.Unsubscribe(token);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Core {

using EventValue = std::variant<bool,int32_t,float,std::string>;

struct GameEvent {
    explicit GameEvent(const std::string& channel) : channel(channel) {}
    std::string channel;
    uint64_t    timestamp{0};     ///< frame number or tick, set by bus

    void Set(const std::string& key, bool v)               { data[key]=v; }
    void Set(const std::string& key, int32_t v)            { data[key]=v; }
    void Set(const std::string& key, float v)              { data[key]=v; }
    void Set(const std::string& key, const std::string& v) { data[key]=v; }

    bool        GetBool(const std::string& k, bool def=false)       const;
    int32_t     GetInt(const std::string& k, int32_t def=0)         const;
    float       GetFloat(const std::string& k, float def=0.f)       const;
    std::string GetString(const std::string& k, const std::string& def="") const;
    bool        Has(const std::string& k) const { return data.count(k)>0; }

    std::unordered_map<std::string,EventValue> data;
};

using EventHandler  = std::function<void(const GameEvent&)>;
using EventFilter   = std::function<bool(const GameEvent&)>;
using SubscribeToken = uint64_t;

class GameEventBus {
public:
    GameEventBus();
    ~GameEventBus();

    void Init();
    void Shutdown();

    // Subscription
    SubscribeToken Subscribe(const std::string& channel, EventHandler handler,
                              int32_t priority = 0,
                              EventFilter filter = nullptr);
    SubscribeToken SubscribeOnce(const std::string& channel, EventHandler handler,
                                  EventFilter filter = nullptr);
    void Unsubscribe(SubscribeToken token);
    void UnsubscribeAll(const std::string& channel);

    // Immediate dispatch
    void Post(const GameEvent& event);
    void Post(const std::string& channel);   ///< event with no payload

    // Deferred dispatch (safe to call from worker threads)
    void PostDeferred(const GameEvent& event);
    void FlushDeferred();               ///< call once per frame on main thread

    // Stats
    uint32_t SubscriberCount(const std::string& channel) const;
    uint32_t PendingDeferredCount() const;

    void SetFrameCounter(uint64_t frame);   ///< set timestamp on posted events

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Core
