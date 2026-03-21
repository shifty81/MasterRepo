// Core/Allocator/PoolAllocator/PoolAllocator.cpp
// PoolAllocator<T,N> is header-only.
// This translation unit satisfies the CMake source list and exercises the template.
#include "Core/Allocator/PoolAllocator/PoolAllocator.h"

namespace Core {
// Explicit instantiations of common specialisations (catches template errors early).
struct _PATestType { int value{0}; };
template class PoolAllocator<_PATestType, 16>;
template class PoolAllocator<int,         64>;
} // namespace Core
