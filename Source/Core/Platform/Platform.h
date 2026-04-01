#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace NF {
namespace Platform {

/// @brief Return the name of the host operating system (e.g. "Linux", "Windows", "macOS").
[[nodiscard]] std::string_view GetPlatformName() noexcept;

/// @brief Return elapsed seconds since an arbitrary epoch with high precision.
///        Suitable for frame-timing and profiling; not wall-clock time.
[[nodiscard]] double GetTimestamp() noexcept;

/// @brief Test whether a file exists and is accessible at the given path.
/// @param path Filesystem path to test.
/// @return true if the file exists and can be opened for reading.
[[nodiscard]] bool FileExists(std::string_view path) noexcept;

/// @brief Read the entire contents of a binary file into a byte vector.
/// @param path Path to the file.
/// @return The file bytes on success, std::nullopt if the file cannot be read.
[[nodiscard]] std::optional<std::vector<std::byte>> ReadFile(std::string_view path);

} // namespace Platform
} // namespace NF
