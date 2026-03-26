#include "Core/CloudSync/CloudSyncManager.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Core {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct CloudSyncManager::Impl {
    CloudSyncConfig                             config;
    std::unordered_map<std::string, SyncRecord> records;
    ConflictPolicy                              conflictPolicy{ConflictPolicy::KeepNewest};
    float                                       progress{0.f};
    bool                                        syncing{false};

    std::function<void(const SyncRecord&)>       onSynced;
    std::function<void(const SyncRecord&)>       onError;
    std::function<void(const std::string&)>      onConflict;
    std::function<void(uint32_t,uint32_t)>       onAllComplete;

    bool DoSync(const std::string& localPath, SyncDirection dir) {
        // Offline-first stub: copy file locally, no real HTTP
        if (!config.enabled) return false;
        auto& rec       = records[localPath];
        rec.path        = localPath;
        rec.lastDirection = dir;
        rec.status      = SyncStatus::Syncing;

        // Simulate success (no real network in this environment)
        bool ok = true;  // real impl: HTTP PUT/GET

        rec.status    = ok ? SyncStatus::Success : SyncStatus::Failed;
        rec.lastSyncMs = NowMs();
        if (ok  && onSynced)  onSynced(rec);
        if (!ok && onError)   onError(rec);
        return ok;
    }
};

CloudSyncManager::CloudSyncManager() : m_impl(new Impl()) {}
CloudSyncManager::~CloudSyncManager() { delete m_impl; }

void CloudSyncManager::Init(const CloudSyncConfig& cfg) { m_impl->config = cfg; }
void CloudSyncManager::Shutdown() {}

bool CloudSyncManager::IsEnabled()  const { return m_impl->config.enabled; }
bool CloudSyncManager::IsSyncing()  const { return m_impl->syncing; }

bool CloudSyncManager::Push(const std::string& localPath) {
    return m_impl->DoSync(localPath, SyncDirection::Push);
}

bool CloudSyncManager::Pull(const std::string& remotePath,
                            const std::string& localDest) {
    std::string dest = localDest.empty() ? remotePath : localDest;
    return m_impl->DoSync(dest, SyncDirection::Pull);
}

bool CloudSyncManager::SyncAll() {
    m_impl->syncing = true;
    uint32_t ok = 0, fail = 0;
    for (auto& [path, rec] : m_impl->records) {
        bool result = m_impl->DoSync(path, SyncDirection::Both);
        result ? ++ok : ++fail;
    }
    m_impl->syncing   = false;
    m_impl->progress  = 1.f;
    if (m_impl->onAllComplete) m_impl->onAllComplete(ok, fail);
    return fail == 0;
}

bool CloudSyncManager::SyncDirectory(const std::string& localDir,
                                     SyncDirection dir) {
    // Stub: would enumerate directory; here we just record it
    m_impl->DoSync(localDir, dir);
    return true;
}

void CloudSyncManager::SetConflictPolicy(ConflictPolicy p) {
    m_impl->conflictPolicy = p;
}

std::vector<SyncRecord> CloudSyncManager::Records() const {
    std::vector<SyncRecord> out;
    out.reserve(m_impl->records.size());
    for (auto& [k, v] : m_impl->records) out.push_back(v);
    return out;
}

SyncRecord CloudSyncManager::GetRecord(const std::string& path) const {
    auto it = m_impl->records.find(path);
    if (it == m_impl->records.end()) return {};
    return it->second;
}

uint32_t CloudSyncManager::PendingCount() const {
    uint32_t n = 0;
    for (auto& [k, v] : m_impl->records)
        if (v.status == SyncStatus::Idle) ++n;
    return n;
}

float CloudSyncManager::Progress() const { return m_impl->progress; }

void CloudSyncManager::OnFileSynced(std::function<void(const SyncRecord&)> cb) {
    m_impl->onSynced = std::move(cb);
}

void CloudSyncManager::OnSyncError(std::function<void(const SyncRecord&)> cb) {
    m_impl->onError = std::move(cb);
}

void CloudSyncManager::OnConflict(std::function<void(const std::string&)> cb) {
    m_impl->onConflict = std::move(cb);
}

void CloudSyncManager::OnAllComplete(std::function<void(uint32_t,uint32_t)> cb) {
    m_impl->onAllComplete = std::move(cb);
}

} // namespace Core
