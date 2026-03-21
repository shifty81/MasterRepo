#pragma once
/**
 * @file RingBuffer.h
 * @brief Header-only fixed-capacity ring buffer template with configurable overflow policy.
 *
 * Usage:
 *   Core::RingBuffer<float, 32> buf;
 *   buf.Push(3.14f);
 *   float v = buf.Pop();
 *   float f = buf.Peek();
 *
 *   // Overwrite mode (newest discards oldest when full):
 *   Core::RingBuffer<int, 8, Core::OverflowPolicy::Overwrite> log;
 *
 * Design:
 *   - Fixed capacity N determined at compile time → no heap allocation.
 *   - Two overflow policies:
 *     - Reject: Push() returns false when full; existing data preserved.
 *     - Overwrite: Push() overwrites the oldest element when full.
 *   - Bidirectional iterator for range-for support.
 *   - IsFull() / IsEmpty() / Size() / Capacity() accessors.
 *   - PeekBack() returns the most recently pushed element.
 *   - Clear() resets without touching storage.
 */

#include <cstddef>
#include <cstdint>
#include <optional>
#include <iterator>
#include <cassert>

namespace Core {

enum class OverflowPolicy : uint8_t {
    Reject,     ///< Push() fails (returns false) when buffer is full
    Overwrite,  ///< Push() silently discards the oldest element when full
};

template<typename T, size_t N, OverflowPolicy Policy = OverflowPolicy::Reject>
class RingBuffer {
    static_assert(N > 0, "RingBuffer capacity must be > 0");
public:
    using value_type = T;
    static constexpr size_t kCapacity = N;

    RingBuffer() = default;

    // ── capacity ──────────────────────────────────────────────
    constexpr size_t Capacity() const noexcept { return N; }
    size_t           Size()     const noexcept { return m_size; }
    bool             IsEmpty()  const noexcept { return m_size == 0; }
    bool             IsFull()   const noexcept { return m_size == N; }

    // ── push ──────────────────────────────────────────────────
    bool Push(const T& value) {
        if (IsFull()) {
            if constexpr (Policy == OverflowPolicy::Reject) {
                return false;
            } else {
                // Overwrite: advance read pointer
                m_head = (m_head + 1) % N;
                --m_size;
            }
        }
        m_storage[m_tail] = value;
        m_tail = (m_tail + 1) % N;
        ++m_size;
        return true;
    }

    bool Push(T&& value) {
        if (IsFull()) {
            if constexpr (Policy == OverflowPolicy::Reject) {
                return false;
            } else {
                m_head = (m_head + 1) % N;
                --m_size;
            }
        }
        m_storage[m_tail] = std::move(value);
        m_tail = (m_tail + 1) % N;
        ++m_size;
        return true;
    }

    // ── pop ───────────────────────────────────────────────────
    std::optional<T> Pop() {
        if (IsEmpty()) return std::nullopt;
        T val = m_storage[m_head];
        m_head = (m_head + 1) % N;
        --m_size;
        return val;
    }

    // ── peek (oldest element) ─────────────────────────────────
    std::optional<T> Peek() const {
        if (IsEmpty()) return std::nullopt;
        return m_storage[m_head];
    }

    // ── peek back (newest element) ────────────────────────────
    std::optional<T> PeekBack() const {
        if (IsEmpty()) return std::nullopt;
        size_t idx = (m_tail == 0) ? N - 1 : m_tail - 1;
        return m_storage[idx];
    }

    // ── indexed access ────────────────────────────────────────
    /// Index 0 = oldest element, Index Size()-1 = newest.
    const T& operator[](size_t i) const {
        return m_storage[(m_head + i) % N];
    }
    T& operator[](size_t i) {
        return m_storage[(m_head + i) % N];
    }

    // ── clear ─────────────────────────────────────────────────
    void Clear() noexcept { m_head = m_tail = m_size = 0; }

    // ── iterator ──────────────────────────────────────────────
    struct iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;
        using reference         = T&;

        RingBuffer* buf{nullptr};
        size_t      idx{0};

        reference operator*()  const { return (*buf)[idx]; }
        pointer   operator->() const { return &(*buf)[idx]; }
        iterator& operator++() { ++idx; return *this; }
        iterator  operator++(int) { iterator tmp=*this; ++(*this); return tmp; }
        bool operator==(const iterator& o) const { return buf==o.buf && idx==o.idx; }
        bool operator!=(const iterator& o) const { return !(*this==o); }
    };

    struct const_iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = const T*;
        using reference         = const T&;

        const RingBuffer* buf{nullptr};
        size_t            idx{0};

        reference operator*()  const { return (*buf)[idx]; }
        pointer   operator->() const { return &(*buf)[idx]; }
        const_iterator& operator++() { ++idx; return *this; }
        const_iterator  operator++(int) { const_iterator tmp=*this; ++(*this); return tmp; }
        bool operator==(const const_iterator& o) const { return buf==o.buf && idx==o.idx; }
        bool operator!=(const const_iterator& o) const { return !(*this==o); }
    };

    iterator       begin()        { return {this, 0}; }
    iterator       end()          { return {this, m_size}; }
    const_iterator begin()  const { return {this, 0}; }
    const_iterator end()    const { return {this, m_size}; }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend()   const { return end(); }

private:
    T      m_storage[N]{};
    size_t m_head{0};
    size_t m_tail{0};
    size_t m_size{0};
};

} // namespace Core
