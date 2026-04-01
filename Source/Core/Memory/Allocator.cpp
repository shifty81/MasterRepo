#include "Core/Memory/Allocator.h"
#include <cassert>
#include <cstdint>

namespace NF {

namespace {
    std::byte* AlignUp(std::byte* ptr, size_t align) noexcept {
        const uintptr_t addr    = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t aligned = (addr + (align - 1)) & ~(align - 1);
        return reinterpret_cast<std::byte*>(aligned);
    }
} // anonymous namespace

LinearAllocator::LinearAllocator(void* buffer, size_t capacity) noexcept
    : m_Buffer(static_cast<std::byte*>(buffer))
    , m_Capacity(capacity)
    , m_Offset(0)
{}

void* LinearAllocator::Allocate(size_t size, size_t align) {
    assert(align > 0 && (align & (align - 1)) == 0 && "Alignment must be a power of two");
    std::byte* base    = m_Buffer + m_Offset;
    std::byte* aligned = AlignUp(base, align);
    size_t     padding = static_cast<size_t>(aligned - base);

    if (m_Offset + padding + size > m_Capacity)
        return nullptr;

    m_Offset += padding + size;
    return aligned;
}

void LinearAllocator::Deallocate(void* /*ptr*/) {
    // Linear allocators only support bulk reset; individual frees are ignored.
}

void LinearAllocator::Reset() noexcept {
    m_Offset = 0;
}

size_t LinearAllocator::Used() const noexcept {
    return m_Offset;
}

size_t LinearAllocator::Capacity() const noexcept {
    return m_Capacity;
}

} // namespace NF
