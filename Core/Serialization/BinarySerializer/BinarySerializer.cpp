#include "Core/Serialization/BinarySerializer/BinarySerializer.h"
// BinarySerializer is header-only; this stub ensures it appears in the build graph
// and allows static assertions to be verified at compile time.

namespace Core {

// Force instantiation of a few common specialisations so the compiler validates them.
namespace {
    static_assert(sizeof(BinaryWriter) > 0, "BinaryWriter must be non-empty");
    static_assert(sizeof(BinaryReader) > 0, "BinaryReader must be non-empty");
} // anonymous namespace

} // namespace Core
