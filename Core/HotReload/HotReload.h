#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <cstdint>

namespace Core {

enum class HotReloadKind { Script, Asset, Config, Plugin };

struct WatchEntry {
    std::string     path;
    HotReloadKind   kind       = HotReloadKind::Asset;
    std::string     lastHash;
    std::chrono::system_clock::time_point lastModified;
    bool            enabled    = true;
};

struct ReloadEvent {
    std::string   path;
    HotReloadKind kind;
    bool          success     = false;
    std::string   errorMessage;
};

class HotReload {
public:
    static HotReload& Instance();

    void Watch(const std::string& path, HotReloadKind kind = HotReloadKind::Asset);
    void WatchDirectory(const std::string& dir, HotReloadKind kind = HotReloadKind::Asset);
    void Unwatch(const std::string& path);
    void UnwatchAll();

    std::vector<ReloadEvent> Poll();

    using ReloadCallback = std::function<void(const ReloadEvent&)>;
    void SetCallback(HotReloadKind kind, ReloadCallback cb);

    ReloadEvent ForceReload(const std::string& path);

    const std::vector<WatchEntry>& Entries() const;
    size_t WatchCount() const;
    bool IsWatching(const std::string& path) const;

    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    uint64_t TotalReloads() const;
    uint64_t FailedReloads() const;

private:
    HotReload() = default;
    bool hasChanged(const WatchEntry& e) const;
    std::string hashFile(const std::string& path) const;

    std::vector<WatchEntry> m_entries;
    std::unordered_map<int, ReloadCallback> m_callbacks;
    bool m_enabled = true;
    uint64_t m_totalReloads  = 0;
    uint64_t m_failedReloads = 0;
};

} // namespace Core
