#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Tools {

// ── Data types ────────────────────────────────────────────────────────────────

enum class ChangeKind : uint8_t {
    Added    = 0,
    Modified = 1,
    Removed  = 2,
    Renamed  = 3,
};

struct FileChange {
    std::string path;
    std::string oldPath;    // non-empty only for Renamed
    ChangeKind  kind{ChangeKind::Modified};
    uint64_t    timestampMs{0};
    uint64_t    sizeBytes{0};
};

struct ChangeTrackerStats {
    uint64_t watchedFiles{0};
    uint64_t added{0};
    uint64_t modified{0};
    uint64_t removed{0};
    uint64_t renamed{0};
    uint64_t totalEvents{0};
};

// ── ChangeTracker ─────────────────────────────────────────────────────────────

class ChangeTracker {
public:
    ChangeTracker();
    ~ChangeTracker();

    // Add a directory (and optionally recurse into subdirs) to the watch list.
    // Extensions: only watch files with these extensions (empty = watch all).
    void WatchDirectory(const std::string& dir,
                        bool recursive,
                        const std::vector<std::string>& extensions = {});

    // Add a single file to the watch list.
    void WatchFile(const std::string& path);

    // Remove a watch.
    void Unwatch(const std::string& pathOrDir);

    // Remove all watches.
    void Clear();

    // Per-frame tick — scan for changes and fire callbacks.
    void Tick();

    // Polling interval in milliseconds.
    void SetPollingInterval(uint64_t ms);

    // Drain all pending change events since the last Tick().
    std::vector<FileChange> PendingChanges() const;

    // Clear the pending queue.
    void FlushPending();

    // Callback — fired once per detected change event.
    void OnChange(std::function<void(const FileChange&)> callback);

    // Callback — fired only for a specific change kind.
    void OnChangeKind(ChangeKind kind,
                      std::function<void(const FileChange&)> callback);

    // Snapshot the current state of all watched files/dirs.
    // Subsequent calls to Tick() will report changes relative to this snapshot.
    void Snapshot();

    // Diff the current filesystem state against the last snapshot
    // without modifying internal tracking state.
    std::vector<FileChange> DiffSnapshot() const;

    // Watched file count.
    size_t WatchedCount() const;

    // Stats.
    ChangeTrackerStats Stats() const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Tools
