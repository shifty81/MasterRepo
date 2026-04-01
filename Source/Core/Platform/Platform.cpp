#include "Core/Platform/Platform.h"
#include <chrono>
#include <fstream>

#if defined(_WIN32)
    #define NF_PLATFORM_NAME "Windows"
#elif defined(__APPLE__)
    #define NF_PLATFORM_NAME "macOS"
#elif defined(__linux__)
    #define NF_PLATFORM_NAME "Linux"
#else
    #define NF_PLATFORM_NAME "Unknown"
#endif

namespace NF {
namespace Platform {

std::string_view GetPlatformName() noexcept {
    return NF_PLATFORM_NAME;
}

double GetTimestamp() noexcept {
    using Clock = std::chrono::steady_clock;
    static const auto kEpoch = Clock::now();
    return std::chrono::duration<double>(Clock::now() - kEpoch).count();
}

bool FileExists(std::string_view path) noexcept {
    const std::string p{path};
    std::ifstream f{p};
    return f.good();
}

std::optional<std::vector<std::byte>> ReadFile(std::string_view path) {
    const std::string p{path};
    std::ifstream f{p, std::ios::binary | std::ios::ate};
    if (!f)
        return std::nullopt;

    const auto size = f.tellg();
    if (size < 0)
        return std::nullopt;

    f.seekg(0, std::ios::beg);
    std::vector<std::byte> buffer(static_cast<size_t>(size));

    if (!f.read(reinterpret_cast<char*>(buffer.data()), size))
        return std::nullopt;

    return buffer;
}

} // namespace Platform
} // namespace NF
