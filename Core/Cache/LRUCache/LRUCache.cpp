// Core/Cache/LRUCache/LRUCache.cpp
// LRUCache<K,V,Capacity> is header-only.
// This translation unit satisfies the CMake source list and exercises the
// template with common specialisations so compiler errors surface at build time.
#include "Core/Cache/LRUCache/LRUCache.h"

#include <string>

namespace Core {
// Explicit instantiations of common specialisations.
template class LRUCache<std::string, int,         64>;
template class LRUCache<std::string, std::string, 128>;
template class LRUCache<int,         int,         256>;
} // namespace Core
