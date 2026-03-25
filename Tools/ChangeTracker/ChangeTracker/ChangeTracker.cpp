#include "Tools/ChangeTracker/ChangeTracker/ChangeTracker.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace Tools {

// ── helpers ───────────────────────────────────────────────────────────────────

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

static uint64_t GetModifiedMs(const fs::path& p) {
    std::error_code ec;
    auto ft = fs::last_write_time(p, ec);
    if (ec) return 0;
    auto sc = std::chrono::clock_cast<std::chrono::system_clock>(ft);
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            sc.time_since_epoch()).count());
}

static uint64_t GetSizeBytes(const fs::path& p) {
    std::error_code ec;
    return static_cast<uint64_t>(fs::file_size(p, ec));
}

static std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool ExtensionMatches(const fs::path& p,
                              const std::vector<std::string>& exts) {
    if (exts.empty()) return true;
    std::string ext = ToLower(p.extension().string());
    for (const auto& e : exts) if (ToLower(e) == ext) return true;
    return false;
}

// ── Impl ─────────────────────────────────────────────────────────────────────

struct WatchSpec {
    std::string              path;
    bool                     isDir{false};
    bool                     recursive{false};
    std::vector<std::string> extensions;
};

struct FileSnapshot {
    uint64_t modifiedMs{0};
    uint64_t sizeBytes{0};
};

struct ChangeTracker::Impl {
    std::vector<WatchSpec>                               specs;
    std::map<std::string, FileSnapshot>                  snapshot;  // last known state
    std::vector<FileChange>                              pending;
    std::function<void(const FileChange&)>               onAny;
    std::map<ChangeKind, std::function<void(const FileChange&)>> kindCallbacks;
    uint64_t pollingIntervalMs{1000};
    uint64_t lastTickMs{0};
    ChangeTrackerStats stats{};
};

// ── ChangeTracker ─────────────────────────────────────────────────────────────

ChangeTracker::ChangeTracker() : m_impl(new Impl{}) {}
ChangeTracker::~ChangeTracker() { delete m_impl; }

void ChangeTracker::WatchDirectory(const std::string& dir, bool recursive,
                                   const std::vector<std::string>& extensions) {
    WatchSpec s;
    s.path       = dir;
    s.isDir      = true;
    s.recursive  = recursive;
    s.extensions = extensions;
    m_impl->specs.push_back(s);
}

void ChangeTracker::WatchFile(const std::string& path) {
    WatchSpec s;
    s.path  = path;
    s.isDir = false;
    m_impl->specs.push_back(s);
    // Seed snapshot
    m_impl->snapshot[path] = { GetModifiedMs(path), GetSizeBytes(path) };
}

void ChangeTracker::Unwatch(const std::string& pathOrDir) {
    auto& v = m_impl->specs;
    v.erase(std::remove_if(v.begin(), v.end(),
                           [&](const WatchSpec& s){ return s.path == pathOrDir; }),
            v.end());
    // Remove snapshot entries whose paths are under this directory or match exactly
    const fs::path base(pathOrDir);
    for (auto it = m_impl->snapshot.begin(); it != m_impl->snapshot.end(); ) {
        const fs::path entryPath(it->first);
        // Match if paths are equal or the entry is inside the watched directory
        bool match = (entryPath == base);
        if (!match) {
            // Check if base is a proper prefix (i.e. a parent directory)
            auto baseStr = base.string();
            auto entryStr = entryPath.string();
            if (entryStr.size() > baseStr.size() &&
                entryStr[baseStr.size()] == fs::path::preferred_separator &&
                entryStr.compare(0, baseStr.size(), baseStr) == 0) {
                match = true;
            }
        }
        if (match) it = m_impl->snapshot.erase(it);
        else ++it;
    }
}

void ChangeTracker::Clear() {
    m_impl->specs.clear();
    m_impl->snapshot.clear();
    m_impl->pending.clear();
}

static void CollectFiles(const WatchSpec& spec,
                          std::map<std::string, FileSnapshot>& out) {
    std::error_code ec;
    if (!spec.isDir) {
        out[spec.path] = { GetModifiedMs(spec.path), GetSizeBytes(spec.path) };
        return;
    }
    // Use a directory_iterator for non-recursive
    if (!spec.recursive) {
        for (const auto& de : fs::directory_iterator(spec.path, ec)) {
            if (ec) break;
            if (!de.is_regular_file()) continue;
            if (!ExtensionMatches(de.path(), spec.extensions)) continue;
            out[de.path().string()] = {
                GetModifiedMs(de.path()), GetSizeBytes(de.path()) };
        }
        return;
    }
    for (const auto& de : fs::recursive_directory_iterator(spec.path, ec)) {
        if (ec) break;
        if (!de.is_regular_file()) continue;
        if (!ExtensionMatches(de.path(), spec.extensions)) continue;
        out[de.path().string()] = {
            GetModifiedMs(de.path()), GetSizeBytes(de.path()) };
    }
}

void ChangeTracker::Tick() {
    uint64_t now = NowMs();
    if (now - m_impl->lastTickMs < m_impl->pollingIntervalMs) return;
    m_impl->lastTickMs = now;

    std::map<std::string, FileSnapshot> current;
    for (const auto& spec : m_impl->specs) CollectFiles(spec, current);

    m_impl->stats.watchedFiles = current.size();

    // Detect added / modified
    for (const auto& [path, snap] : current) {
        auto it = m_impl->snapshot.find(path);
        if (it == m_impl->snapshot.end()) {
            // Added
            FileChange ev{ path, "", ChangeKind::Added, now, snap.sizeBytes };
            m_impl->pending.push_back(ev);
            ++m_impl->stats.added; ++m_impl->stats.totalEvents;
            if (m_impl->onAny) m_impl->onAny(ev);
            auto cb = m_impl->kindCallbacks.find(ChangeKind::Added);
            if (cb != m_impl->kindCallbacks.end()) cb->second(ev);
        } else if (snap.modifiedMs != it->second.modifiedMs ||
                   snap.sizeBytes  != it->second.sizeBytes) {
            // Modified
            FileChange ev{ path, "", ChangeKind::Modified, now, snap.sizeBytes };
            m_impl->pending.push_back(ev);
            ++m_impl->stats.modified; ++m_impl->stats.totalEvents;
            if (m_impl->onAny) m_impl->onAny(ev);
            auto cb = m_impl->kindCallbacks.find(ChangeKind::Modified);
            if (cb != m_impl->kindCallbacks.end()) cb->second(ev);
        }
    }

    // Detect removed
    for (const auto& [path, snap] : m_impl->snapshot) {
        if (current.find(path) == current.end()) {
            FileChange ev{ path, "", ChangeKind::Removed, now, 0 };
            m_impl->pending.push_back(ev);
            ++m_impl->stats.removed; ++m_impl->stats.totalEvents;
            if (m_impl->onAny) m_impl->onAny(ev);
            auto cb = m_impl->kindCallbacks.find(ChangeKind::Removed);
            if (cb != m_impl->kindCallbacks.end()) cb->second(ev);
        }
    }

    m_impl->snapshot = current;
}

void ChangeTracker::SetPollingInterval(uint64_t ms) {
    m_impl->pollingIntervalMs = ms;
}

std::vector<FileChange> ChangeTracker::PendingChanges() const {
    return m_impl->pending;
}

void ChangeTracker::FlushPending() { m_impl->pending.clear(); }

void ChangeTracker::OnChange(std::function<void(const FileChange&)> callback) {
    m_impl->onAny = std::move(callback);
}

void ChangeTracker::OnChangeKind(ChangeKind kind,
                                  std::function<void(const FileChange&)> callback) {
    m_impl->kindCallbacks[kind] = std::move(callback);
}

void ChangeTracker::Snapshot() {
    m_impl->snapshot.clear();
    for (const auto& spec : m_impl->specs)
        CollectFiles(spec, m_impl->snapshot);
}

std::vector<FileChange> ChangeTracker::DiffSnapshot() const {
    std::map<std::string, FileSnapshot> current;
    for (const auto& spec : m_impl->specs) CollectFiles(spec, current);

    uint64_t now = NowMs();
    std::vector<FileChange> changes;

    for (const auto& [path, snap] : current) {
        auto it = m_impl->snapshot.find(path);
        if (it == m_impl->snapshot.end()) {
            changes.push_back({ path, "", ChangeKind::Added, now, snap.sizeBytes });
        } else if (snap.modifiedMs != it->second.modifiedMs ||
                   snap.sizeBytes  != it->second.sizeBytes) {
            changes.push_back({ path, "", ChangeKind::Modified, now, snap.sizeBytes });
        }
    }
    for (const auto& [path, snap] : m_impl->snapshot) {
        if (current.find(path) == current.end())
            changes.push_back({ path, "", ChangeKind::Removed, now, 0 });
    }
    return changes;
}

size_t ChangeTracker::WatchedCount() const { return m_impl->snapshot.size(); }

ChangeTrackerStats ChangeTracker::Stats() const { return m_impl->stats; }

} // namespace Tools
