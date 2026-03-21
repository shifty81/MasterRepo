// Core/Signal/Signal.cpp
// Signal<Args...> is a header-only template; no compiled symbols needed.
// This translation unit satisfies the CMake source list.
#include "Core/Signal/Signal.h"

// Explicit instantiation of common specialisations.
namespace Core {
template class Signal<>;
template class Signal<int>;
template class Signal<float>;
} // namespace Core
