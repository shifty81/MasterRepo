#pragma once
/**
 * @file ObjectPool.h
 * @brief Generic typed object pool with pre-allocation, borrow/return, and stats.
 *
 * Features:
 *   - Pre-allocated fixed-capacity pool (no dynamic alloc during gameplay)
 *   - Optional grow: pool grows by a slab if empty (configurable)
 *   - Borrow() returns a pointer to a free object, null if pool is exhausted
 *   - Return() recycles object back to the pool
 *   - RAII guard: PooledObject<T> auto-returns on destruction
 *   - On-borrow / on-return callbacks for pooled-resource initialisation
 *   - Stats: capacity, in-use count, peak usage, borrow failures
 *   - Thread-safe variant (ObjectPoolTS) using mutex
 *   - Header-only template
 *
 * Typical usage:
 * @code
 *   ObjectPool<Projectile> pool;
 *   pool.Init(128);  // pre-allocate 128 projectiles
 *   pool.SetOnBorrow([](Projectile& p){ p.Reset(); });
 *   auto* proj = pool.Borrow();
 *   proj->Fire(origin, dir);
 *   // when done:
 *   pool.Return(proj);
 *   // or RAII:
 *   auto guard = pool.BorrowGuard();
 *   guard->Fire(origin, dir);
 *   // returns automatically when guard goes out of scope
 * @endcode
 */

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace Core {

// ── Non-thread-safe pool ─────────────────────────────────────────────────────

template<typename T>
class ObjectPool {
public:
    using InitCb   = std::function<void(T&)>;
    using ReturnCb = std::function<void(T&)>;

    ObjectPool() = default;
    ~ObjectPool() = default;

    void Init(uint32_t capacity, uint32_t growSlab=0) {
        m_capacity = capacity;
        m_growSlab = growSlab;
        m_objects.resize(capacity);
        m_free.reserve(capacity);
        for (uint32_t i=0; i<capacity; i++) m_free.push_back(&m_objects[i]);
    }

    void SetOnBorrow(InitCb   cb) { m_onBorrow = cb; }
    void SetOnReturn(ReturnCb cb) { m_onReturn = cb; }

    T* Borrow() {
        if (m_free.empty()) {
            if (m_growSlab > 0) Grow(m_growSlab);
            else { m_failures++; return nullptr; }
        }
        T* obj = m_free.back(); m_free.pop_back();
        m_inUse++;
        m_peak = std::max(m_peak, m_inUse);
        if (m_onBorrow) m_onBorrow(*obj);
        return obj;
    }

    void Return(T* obj) {
        if (!obj) return;
        if (m_onReturn) m_onReturn(*obj);
        m_free.push_back(obj);
        m_inUse--;
    }

    // RAII guard
    struct Guard {
        Guard(ObjectPool& pool, T* obj) : m_pool(pool), m_obj(obj) {}
        ~Guard() { if (m_obj) m_pool.Return(m_obj); }
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
        Guard(Guard&& o) : m_pool(o.m_pool), m_obj(o.m_obj) { o.m_obj=nullptr; }
        T* get()          const { return m_obj; }
        T* operator->()   const { return m_obj; }
        T& operator*()    const { return *m_obj; }
        explicit operator bool() const { return m_obj != nullptr; }
    private:
        ObjectPool& m_pool;
        T*          m_obj{nullptr};
    };

    Guard BorrowGuard() { return Guard(*this, Borrow()); }

    // Stats
    uint32_t Capacity()  const { return m_capacity; }
    uint32_t Available() const { return (uint32_t)m_free.size(); }
    uint32_t InUse()     const { return m_inUse; }
    uint32_t Peak()      const { return m_peak; }
    uint32_t Failures()  const { return m_failures; }

    void Reset() {
        m_free.clear();
        for (auto& obj : m_objects) m_free.push_back(&obj);
        m_inUse = 0;
    }

private:
    void Grow(uint32_t slab) {
        uint32_t oldSz = (uint32_t)m_objects.size();
        m_objects.resize(oldSz + slab);
        for (uint32_t i=oldSz; i<(uint32_t)m_objects.size(); i++)
            m_free.push_back(&m_objects[i]);
        m_capacity += slab;
    }

    std::vector<T>  m_objects;
    std::vector<T*> m_free;
    uint32_t        m_capacity{0};
    uint32_t        m_growSlab{0};
    uint32_t        m_inUse{0};
    uint32_t        m_peak{0};
    uint32_t        m_failures{0};
    InitCb          m_onBorrow;
    ReturnCb        m_onReturn;
};

// ── Thread-safe variant ──────────────────────────────────────────────────────

template<typename T>
class ObjectPoolTS : public ObjectPool<T> {
public:
    T* Borrow() {
        std::lock_guard<std::mutex> lk(m_mu);
        return ObjectPool<T>::Borrow();
    }
    void Return(T* obj) {
        std::lock_guard<std::mutex> lk(m_mu);
        ObjectPool<T>::Return(obj);
    }
private:
    std::mutex m_mu;
};

} // namespace Core
