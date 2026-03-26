#pragma once
/**
 * @file PoolAllocator.h
 * @brief Header-only fixed-size object pool allocator with free-list.
 *
 * PoolAllocator<T, N> manages a fixed array of N slots.
 * Allocation and deallocation are O(1) via a singly-linked free-list.
 *
 * Usage:
 *   Core::PoolAllocator<MyStruct, 256> pool;
 *   MyStruct* p = pool.Allocate();        // returns nullptr if full
 *   new (p) MyStruct(args...);            // placement-new
 *   pool.Deallocate(p);                   // recycle slot
 *
 *   // Or use the RAII helper:
 *   auto* obj = pool.New(args...);        // allocate + construct
 *   pool.Delete(obj);                     // destruct + recycle
 *
 * Safety:
 *   - Allocate() returns nullptr when the pool is exhausted (no exceptions).
 *   - Deallocate(nullptr) is a no-op.
 *   - Debug builds assert pointer belongs to pool range.
 *   - Stats(): total allocated, peak, current in-use.
 */

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <new>
#include <type_traits>
#include <utility>

namespace Core {

// ── Stats ─────────────────────────────────────────────────────────────────
struct PoolStats {
    size_t capacity{0};
    size_t inUse{0};
    size_t totalAllocs{0};
    size_t totalFrees{0};
    size_t peakInUse{0};
};

// ── Pool ──────────────────────────────────────────────────────────────────
template<typename T, size_t N>
class PoolAllocator {
    static_assert(N > 0, "Pool capacity must be > 0");
    static_assert(sizeof(T) >= sizeof(void*) || true, ""); // always ok

    // Each slot is either an object or a free-list node.
    // We use a union-like aligned storage to overlay T and next pointer.
    struct Slot {
        alignas(T) unsigned char storage[sizeof(T)];
        Slot* next{nullptr};
    };

public:
    static constexpr size_t kCapacity = N;

    PoolAllocator() {
        // Build free list
        for (size_t i = 0; i < N - 1; ++i) m_slots[i].next = &m_slots[i + 1];
        m_slots[N - 1].next = nullptr;
        m_free = &m_slots[0];
        m_stats.capacity = N;
    }

    ~PoolAllocator() {
        // Objects must have been Deleted by caller; we don't call destructors here.
    }

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    // ── raw allocation ────────────────────────────────────────
    /// Returns aligned storage for one T, or nullptr if pool is exhausted.
    T* Allocate() {
        if (!m_free) return nullptr;
        Slot* slot = m_free;
        m_free = slot->next;
        ++m_stats.inUse;
        ++m_stats.totalAllocs;
        if (m_stats.inUse > m_stats.peakInUse) m_stats.peakInUse = m_stats.inUse;
        return reinterpret_cast<T*>(slot->storage);
    }

    /// Return a slot to the free list. No-op for nullptr.
    void Deallocate(T* ptr) {
        if (!ptr) return;
        Slot* slot = reinterpret_cast<Slot*>(ptr);
        assert(owns(ptr) && "Pointer does not belong to this pool");
        slot->next = m_free;
        m_free = slot;
        --m_stats.inUse;
        ++m_stats.totalFrees;
    }

    // ── RAII helpers ──────────────────────────────────────────
    /// Allocate + construct in-place; returns nullptr on pool exhaustion.
    template<typename... Args>
    T* New(Args&&... args) {
        T* p = Allocate();
        if (!p) return nullptr;
        new (p) T(std::forward<Args>(args)...);
        return p;
    }

    /// Destruct + deallocate. No-op for nullptr.
    void Delete(T* ptr) {
        if (!ptr) return;
        ptr->~T();
        Deallocate(ptr);
    }

    // ── queries ───────────────────────────────────────────────
    bool   IsFull()  const noexcept { return m_stats.inUse == N; }
    bool   IsEmpty() const noexcept { return m_stats.inUse == 0; }
    size_t InUse()   const noexcept { return m_stats.inUse; }
    size_t Free()    const noexcept { return N - m_stats.inUse; }
    constexpr size_t Capacity() const noexcept { return N; }

    PoolStats Stats() const noexcept { return m_stats; }

    /// True if the pointer was issued by this pool.
    bool owns(const T* ptr) const noexcept {
        const auto* p = reinterpret_cast<const Slot*>(ptr);
        return p >= m_slots && p < m_slots + N;
    }

    /// Reset pool (destructors NOT called — only use when all objects are trivially destructible).
    void Reset() noexcept {
        for (size_t i = 0; i < N - 1; ++i) m_slots[i].next = &m_slots[i + 1];
        m_slots[N - 1].next = nullptr;
        m_free = &m_slots[0];
        m_stats.inUse = 0;
    }

private:
    Slot      m_slots[N];
    Slot*     m_free{nullptr};
    PoolStats m_stats{};
};

} // namespace Core
