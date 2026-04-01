#pragma once
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace NF {

/// @brief Key-value configuration store with simple INI-style file persistence.
///
/// Keys and values are stored as strings; typed access is provided through
/// Get<T>() / Set<T>() which use std::ostringstream / std::istringstream for
/// conversion, so any type with suitable stream operators is supported.
class ConfigSystem {
public:
    /// @brief Load key=value pairs from a text file.
    ///
    /// Lines beginning with '#', ';' or '[' are treated as comments/sections
    /// and are silently ignored.  Existing entries are replaced on collision.
    ///
    /// @param path Path to the configuration file.
    /// @return true if the file was opened and parsed successfully.
    bool Load(std::string_view path);

    /// @brief Persist all current key=value pairs to a text file.
    /// @param path Destination path; the file is created or overwritten.
    /// @return true on success.
    bool Save(std::string_view path) const;

    /// @brief Retrieve a value converted to type T.
    /// @tparam T Target type; must support operator>>(istream&, T&).
    /// @param key The configuration key.
    /// @return The converted value, or std::nullopt if the key is absent or
    ///         the conversion fails.
    template<typename T>
    [[nodiscard]] std::optional<T> Get(std::string_view key) const {
        auto it = m_Store.find(std::string(key));
        if (it == m_Store.end())
            return std::nullopt;

        T val{};
        std::istringstream ss(it->second);
        if (ss >> val)
            return val;
        return std::nullopt;
    }

    /// @brief Store a value, serialising it to a string via operator<<.
    /// @tparam T Source type; must support operator<<(ostream&, const T&).
    /// @param key   The configuration key.
    /// @param value The value to store.
    template<typename T>
    void Set(std::string_view key, const T& value) {
        std::ostringstream ss;
        ss << value;
        m_Store[std::string(key)] = ss.str();
    }

    /// @brief Remove all stored key-value pairs.
    void Clear() noexcept { m_Store.clear(); }

private:
    std::unordered_map<std::string, std::string> m_Store;
};

} // namespace NF
