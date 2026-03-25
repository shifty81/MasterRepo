#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Core {

// ── Data types ────────────────────────────────────────────────────────────────

struct WatchedFile {
    std::string path;
    uint64_t    lastModifiedMs{0};
    std::string tag;
};

struct ReloadEvent {
    std::string path;
    std::string tag;
    uint64_t    timestampMs{0};
};

struct HotReloadStats {
    uint64_t watchedFiles{0};
    uint64_t reloadCount{0};
    uint64_t lastCheckMs{0};
};

// ── HotReloadManager ─────────────────────────────────────────────────────────

class HotReloadManager {
public:
    HotReloadManager();
    ~HotReloadManager();

    // Watch management
    void Watch(const std::string& path, const std::string& tag = "");
    void Unwatch(const std::string& path);
    void Clear();

    // Bulk watch — registers every file in dir whose extension is in extensions
    void WatchDirectory(const std::string& dir,
                        const std::vector<std::string>& extensions);

    // Per-frame tick — detects modified files, fires callbacks
    void Tick();

    // Callbacks
    void OnReload(std::function<void(const ReloadEvent&)> callback);
    void OnReloadTag(const std::string& tag,
                     std::function<void(const ReloadEvent&)> callback);

    // Queries
    size_t                    WatchedCount()   const;
    std::vector<ReloadEvent>  PendingReloads() const;

    // Configuration
    void    SetPollingInterval(uint64_t ms);
    bool    IsWatching(const std::string& path) const;
    uint64_t LastReloadTime(const std::string& path) const;

    // Stats
    HotReloadStats Stats() const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Core
