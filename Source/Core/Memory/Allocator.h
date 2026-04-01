#pragma once
#include <cstddef>
#include <array>

namespace NF {

/// @brief Abstract base class for all NovaForge allocators.
class Allocator {
public:
    virtual ~Allocator() = default;

    /// @brief Allocate a contiguous block of memory.
    /// @param size  Number of bytes to allocate.
    /// @param align Alignment requirement in bytes (must be a power of two).
    /// @return Pointer to the allocated block, or nullptr on failure.
    virtual void* Allocate(size_t size, size_t align = alignof(std::max_align_t)) = 0;

    /// @brief Return a previously allocated block to the allocator.
    /// @param ptr Pointer obtained from Allocate(); nullptr is a no-op.
    virtual void Deallocate(void* ptr) = 0;
};

/// @brief Bump/linear allocator. O(1) alloc; individual free is not supported.
/// All memory is reclaimed at once via Reset().
class LinearAllocator : public Allocator {
public:
    /// @brief Construct over an externally-owned backing buffer.
    /// @param buffer   Pointer to the raw memory region.
    /// @param capacity Total size of the region in bytes.
    LinearAllocator(void* buffer, size_t capacity) noexcept;

    void* Allocate(size_t size, size_t align = alignof(std::max_align_t)) override;

    /// @brief No-op; individual deallocation is not supported by this allocator.
    void  Deallocate(void* ptr) override;

    /// @brief Reset the offset to zero, logically freeing all allocations.
    void Reset() noexcept;

    /// @brief Number of bytes currently occupied (including alignment padding).
    [[nodiscard]] size_t Used() const noexcept;

    /// @brief Total capacity of the backing buffer in bytes.
    [[nodiscard]] size_t Capacity() const noexcept;

private:
    std::byte* m_Buffer{nullptr};
    size_t     m_Capacity{0};
    size_t     m_Offset{0};
};

/// @brief Fixed-size pool allocator backed by a stack-based free list.
/// O(1) alloc and free; capacity is fixed at compile time.
/// @tparam T Object type whose size governs slot size.
/// @tparam N Maximum number of live objects at any given time.
template<typename T, size_t N>
class PoolAllocator : public Allocator {
public:
    PoolAllocator() noexcept {
        for (size_t i = 0; i < N; ++i)
            m_FreeStack[i] = &m_Pool[i];
        m_FreeTop = N;
    }

    void* Allocate(size_t size, size_t /*align*/) override {
        if (size > sizeof(T) || m_FreeTop == 0)
            return nullptr;
        return m_FreeStack[--m_FreeTop];
    }

    void Deallocate(void* ptr) override {
        if (!ptr || m_FreeTop >= N)
            return;
        m_FreeStack[m_FreeTop++] = static_cast<Slot*>(ptr);
    }

private:
    struct alignas(T) Slot { std::byte Data[sizeof(T)]; };

    std::array<Slot,  N> m_Pool;
    std::array<Slot*, N> m_FreeStack;
    size_t               m_FreeTop{0};
};

} // namespace NF
