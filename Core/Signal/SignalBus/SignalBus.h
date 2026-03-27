#pragma once
/**
 * @file SignalBus.h
 * @brief Typed pub/sub broker: register/unregister listeners, fire, batched dispatch.
 *
 * Features:
 *   - String-keyed topic (or compile-time hash)
 *   - Subscribe<T>(topic, handler) → returns SubscriptionHandle
 *   - Unsubscribe(handle)
 *   - Fire<T>(topic, payload) → immediate or enqueued
 *   - Batched mode: Enqueue → DispatchAll()
 *   - Priority: lower value = called first
 *   - Wildcard subscriptions: topic prefix matching ("physics.*")
 *   - One-shot subscriptions (auto-unsubscribe after first delivery)
 *   - Thread-safe fire (mutex per topic)
 *   - Stats: pending count, fired count per topic
 *
 * Typical usage:
 * @code
 *   SignalBus bus;
 *   bus.Init();
 *   auto h = bus.Subscribe("player.death", [](const Signal& s){
 *       auto& d = s.Get<DeathData>(); Log(d.killer);
 *   });
 *   bus.Fire("player.death", Signal::Make(DeathData{"enemy"}));
 *   bus.Unsubscribe(h);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Core {

struct Signal {
    std::string topic;
    std::shared_ptr<void> payload;
    uint64_t timestamp{0};   ///< monotonic counter

    template<class T>
    static Signal Make(T data) {
        Signal s;
        s.payload = std::make_shared<T>(std::move(data));
        return s;
    }
    template<class T>
    const T& Get() const { return *static_cast<const T*>(payload.get()); }
};

using SignalHandler = std::function<void(const Signal&)>;
using SubscriptionHandle = uint64_t;

class SignalBus {
public:
    SignalBus();
    ~SignalBus();

    void Init    ();
    void Shutdown();

    // Subscribe
    SubscriptionHandle Subscribe (const std::string& topic, SignalHandler handler,
                                   int32_t priority=0, bool oneShot=false);
    void               Unsubscribe(SubscriptionHandle handle);
    bool               IsValid    (SubscriptionHandle handle) const;

    // Fire
    void Fire   (const std::string& topic, Signal sig);         ///< immediate
    void Enqueue(const std::string& topic, Signal sig);         ///< batched
    void DispatchAll();                                          ///< flush queue

    // Batch mode
    void SetBatchMode(bool enabled);
    bool IsBatchMode()              const;
    uint32_t PendingCount()         const;

    // Stats
    uint64_t FiredCount(const std::string& topic) const;
    void     ResetStats();

    // Clear
    void UnsubscribeAll(const std::string& topic);
    void Clear();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
