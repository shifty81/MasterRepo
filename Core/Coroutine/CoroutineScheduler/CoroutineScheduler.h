#pragma once
/**
 * @file CoroutineScheduler.h
 * @brief Stackless coroutine: WaitForSeconds, WaitForFrames, WaitUntil, yield, cancel.
 *
 * Features:
 *   - Coroutine represented as a generator-style object (CoroutineHandle)
 *   - StartCoroutine(generator-fn) → CoroutineHandle
 *   - StopCoroutine(handle)
 *   - StopAll()
 *   - Yield instructions: WaitForSeconds(t), WaitForFrames(n), WaitUntil(predicate)
 *   - Tick(dt): advances all live coroutines, decrements timers, checks predicates
 *   - IsAlive(handle)
 *   - On-complete callback per coroutine
 *   - Coroutines stored as std::function returning bool (true=still running)
 *   - Priority: lower int runs first in same frame
 *   - Max coroutine limit (configurable)
 *
 * Typical usage:
 * @code
 *   CoroutineScheduler sched;
 *   sched.Init();
 *
 *   auto h = sched.Start([&, step=0](CoroutineContext& ctx) mutable -> bool {
 *       if (step==0) { step=1; return ctx.WaitForSeconds(2.f); }
 *       if (step==1) { PlayEffect(); step=2; return ctx.WaitForFrames(3); }
 *       return false; // done
 *   });
 *
 *   sched.Tick(dt);  // call each frame
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Core {

struct CoroutineContext {
    float waitRemaining{0.f};
    int32_t waitFrames {0};
    std::function<bool()> waitPredicate;

    bool WaitForSeconds(float t) { waitRemaining=t; return true; }
    bool WaitForFrames (int32_t n){ waitFrames=n; return true; }
    bool WaitUntil     (std::function<bool()> pred){ waitPredicate=pred; return true; }
    bool Yield         ()         { waitFrames=1; return true; }
};

using CoroutineHandle   = uint64_t;
using CoroutineFn       = std::function<bool(CoroutineContext&)>;

class CoroutineScheduler {
public:
    CoroutineScheduler();
    ~CoroutineScheduler();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    CoroutineHandle Start(CoroutineFn fn, int32_t priority=0,
                           std::function<void()> onComplete={});
    void            Stop    (CoroutineHandle handle);
    void            StopAll ();
    bool            IsAlive (CoroutineHandle handle) const;
    uint32_t        Count   () const;

    void SetMaxCoroutines(uint32_t max);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
