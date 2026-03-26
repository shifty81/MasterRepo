#pragma once
/**
 * @file CloudSyncManager.h
 * @brief Optional cloud sync for saves, workspaces, and asset metadata.
 *
 * Offline-first: sync only happens when explicitly triggered or when the
 * user opts in.  Supports multiple backends via a provider interface
 * (local copy, WebDAV stub, custom HTTP).
 *
 * Typical usage:
 * @code
 *   CloudSyncManager cloud;
 *   CloudSyncConfig cfg;
 *   cfg.enabled   = true;
 *   cfg.endpoint  = "http://localhost:8080/sync";
 *   cfg.localRoot = "Saves/";
 *   cloud.Init(cfg);
 *   cloud.Push("Saves/save_slot_0.json");
 *   cloud.Pull("Saves/save_slot_0.json");
 *   cloud.SyncAll();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Core {

// ── Sync direction ────────────────────────────────────────────────────────────

enum class SyncDirection : uint8_t { Push = 0, Pull = 1, Both = 2 };

// ── Sync status for a single file ─────────────────────────────────────────────

enum class SyncStatus : uint8_t { Idle, Syncing, Success, Failed, Conflict };

// ── File sync record ──────────────────────────────────────────────────────────

struct SyncRecord {
    std::string  path;
    SyncStatus   status{SyncStatus::Idle};
    SyncDirection lastDirection{SyncDirection::Both};
    uint64_t     lastSyncMs{0};
    std::string  errorMessage;
};

// ── Cloud sync configuration ──────────────────────────────────────────────────

struct CloudSyncConfig {
    bool        enabled{false};
    std::string endpoint;        ///< remote URL (empty = local-only dry run)
    std::string apiKey;          ///< auth token
    std::string localRoot;       ///< root directory to sync
    uint32_t    retryAttempts{3};
    uint32_t    timeoutMs{10000};
};

// ── CloudSyncManager ──────────────────────────────────────────────────────────

class CloudSyncManager {
public:
    CloudSyncManager();
    ~CloudSyncManager();

    void Init(const CloudSyncConfig& config = {});
    void Shutdown();

    bool IsEnabled()   const;
    bool IsSyncing()   const;

    // ── Operations ────────────────────────────────────────────────────────────

    bool Push(const std::string& localPath);
    bool Pull(const std::string& remotePath, const std::string& localDest = "");
    bool SyncAll();
    bool SyncDirectory(const std::string& localDir, SyncDirection direction);

    // ── Conflict resolution ───────────────────────────────────────────────────

    enum class ConflictPolicy { KeepLocal, KeepRemote, KeepNewest };
    void SetConflictPolicy(ConflictPolicy policy);

    // ── Query ─────────────────────────────────────────────────────────────────

    std::vector<SyncRecord> Records()              const;
    SyncRecord              GetRecord(const std::string& path) const;
    uint32_t                PendingCount()         const;
    float                   Progress()             const; ///< 0–1 during SyncAll

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnFileSynced(std::function<void(const SyncRecord&)> cb);
    void OnSyncError(std::function<void(const SyncRecord&)> cb);
    void OnConflict(std::function<void(const std::string& path)> cb);
    void OnAllComplete(std::function<void(uint32_t success, uint32_t failed)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Core
