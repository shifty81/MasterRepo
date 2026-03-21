// Core/RingBuffer/RingBuffer.cpp
// RingBuffer<T,N,Policy> is header-only.
// This translation unit satisfies the CMake source list.
#include "Core/RingBuffer/RingBuffer.h"

namespace Core {
// Explicit instantiation of common specialisations (helps catch template errors early).
template class RingBuffer<int,   16>;
template class RingBuffer<float, 64>;
template class RingBuffer<int,   64, OverflowPolicy::Overwrite>;
} // namespace Core
