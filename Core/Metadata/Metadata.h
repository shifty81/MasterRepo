#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Core::Metadata {

/// A single metadata entry: key, value, and an optional category for grouping.
struct MetadataEntry {
    std::string Key;
    std::string Value;
    std::string Category;  // Empty string means uncategorised.
};

/// Thread-safe key-value metadata tagging system.
///
/// Attach arbitrary string metadata to any object by composing a MetadataStore
/// alongside it.  Entries may optionally be grouped by category for structured
/// queries (e.g. "rendering", "physics").
///
/// Usage:
///   Core::Metadata::MetadataStore store;
///   store.Add("author", "engine-team", "info");
///   auto val = store.Get("author");            // "engine-team"
///   auto all = store.GetByCategory("info");    // [{author, engine-team, info}]
///   store.Remove("author");
class MetadataStore {
public:
    MetadataStore()  = default;
    ~MetadataStore() = default;

    MetadataStore(const MetadataStore&)            = delete;
    MetadataStore& operator=(const MetadataStore&) = delete;

    // --- Mutators ---

    /// Add or overwrite metadata with the given key.
    /// If the key already exists its value and category are replaced.
    void Add(const std::string& key, const std::string& value, const std::string& category = "")
    {
        std::lock_guard lock(m_Mutex);
        m_Entries[key] = MetadataEntry{ key, value, category };
    }

    /// Remove the entry with the given key.  No-op if not found.
    void Remove(const std::string& key)
    {
        std::lock_guard lock(m_Mutex);
        m_Entries.erase(key);
    }

    /// Remove all entries.
    void Clear()
    {
        std::lock_guard lock(m_Mutex);
        m_Entries.clear();
    }

    // --- Queries ---

    /// Return true if an entry with the given key exists.
    [[nodiscard]] bool Has(const std::string& key) const
    {
        std::lock_guard lock(m_Mutex);
        return m_Entries.count(key) != 0;
    }

    /// Return the value for the given key, or std::nullopt if not found.
    [[nodiscard]] std::optional<std::string> Get(const std::string& key) const
    {
        std::lock_guard lock(m_Mutex);
        auto it = m_Entries.find(key);
        if (it == m_Entries.end()) return std::nullopt;
        return it->second.Value;
    }

    /// Return all entries as a flat vector.
    [[nodiscard]] std::vector<MetadataEntry> GetAll() const
    {
        std::lock_guard lock(m_Mutex);
        std::vector<MetadataEntry> result;
        result.reserve(m_Entries.size());
        for (const auto& [k, entry] : m_Entries) {
            result.push_back(entry);
        }
        return result;
    }

    /// Return all entries whose category matches the given string.
    [[nodiscard]] std::vector<MetadataEntry> GetByCategory(const std::string& category) const
    {
        std::lock_guard lock(m_Mutex);
        std::vector<MetadataEntry> result;
        for (const auto& [k, entry] : m_Entries) {
            if (entry.Category == category) {
                result.push_back(entry);
            }
        }
        return result;
    }

    /// Return the total number of stored entries.
    [[nodiscard]] std::size_t Count() const
    {
        std::lock_guard lock(m_Mutex);
        return m_Entries.size();
    }

private:
    mutable std::mutex                                  m_Mutex;
    std::unordered_map<std::string, MetadataEntry>      m_Entries;
};

} // namespace Core::Metadata
