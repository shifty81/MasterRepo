#include "Runtime/Streaming/AssetStreamer/AssetStreamer.h"
#include <algorithm>
#include <chrono>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <fstream>

namespace Runtime {

const char* AssetStateName(AssetState s) {
    switch (s) {
    case AssetState::Unloaded:  return "Unloaded";
    case AssetState::Queued:    return "Queued";
    case AssetState::Loading:   return "Loading";
    case AssetState::Resident:  return "Resident";
    case AssetState::Evicting:  return "Evicting";
    case AssetState::Failed:    return "Failed";
    }
    return "Unknown";
}

// ── Work item ─────────────────────────────────────────────────────────────
struct WorkItem {
    AssetHandle  handle{kInvalidAssetHandle};
    std::string  path;
    StreamPriority priority{StreamPriority::Normal};

    bool operator<(const WorkItem& o) const {
        return static_cast<int>(priority) > static_cast<int>(o.priority);
    }
};

// ── Impl ─────────────────────────────────────────────────────────────────
struct AssetStreamer::Impl {
    AssetStreamerConfig            cfg;
    std::unordered_map<AssetHandle, AssetInfo> assets;
    std::deque<WorkItem>           queue;   // sorted by priority on insert
    mutable std::mutex             mtx;
    std::condition_variable        cv;
    std::atomic<bool>              running{false};
    std::vector<std::thread>       workers;
    std::atomic<AssetHandle>       nextHandle{1};
    size_t                         residentBytes{0};

    std::vector<AssetLoadedCb>  loadedCbs;
    std::vector<AssetEvictedCb> evictedCbs;
    std::vector<AssetFailedCb>  failedCbs;

    // Pending main-thread callbacks
    struct PendingCallback {
        enum class Kind { Loaded, Evicted, Failed } kind;
        AssetHandle handle;
        std::string info;
    };
    std::vector<PendingCallback> pending;
    std::mutex pendingMtx;

    void workerLoop() {
        while (running.load()) {
            WorkItem item;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this]{ return !queue.empty() || !running.load(); });
                if (!running.load()) break;
                if (queue.empty()) continue;
                item = queue.front();
                queue.pop_front();
                // Mark as Loading
                auto it = assets.find(item.handle);
                if (it != assets.end()) it->second.state = AssetState::Loading;
            }

            // Simulate load: just read file size if it exists
            bool ok = false;
            size_t size = 0;
            double ms = 0;
            {
                auto t0 = std::chrono::steady_clock::now();
                std::ifstream f(item.path, std::ios::binary | std::ios::ate);
                if (f.is_open()) { size = static_cast<size_t>(f.tellg()); ok = true; }
                auto t1 = std::chrono::steady_clock::now();
                ms = std::chrono::duration<double,std::milli>(t1-t0).count();
            }

            {
                std::lock_guard<std::mutex> lock(mtx);
                auto it = assets.find(item.handle);
                if (it != assets.end()) {
                    if (ok) {
                        it->second.state     = AssetState::Resident;
                        it->second.sizeBytes = size;
                        it->second.loadTimeMs = ms;
                        residentBytes += size;
                    } else {
                        it->second.state = AssetState::Failed;
                    }
                }
            }
            {
                std::lock_guard<std::mutex> lock(pendingMtx);
                PendingCallback cb;
                cb.kind   = ok ? PendingCallback::Kind::Loaded : PendingCallback::Kind::Failed;
                cb.handle = item.handle;
                cb.info   = ok ? item.path : "File not found: " + item.path;
                pending.push_back(cb);
            }
        }
    }
};

AssetStreamer::AssetStreamer() : m_impl(new Impl()) {
    m_impl->running.store(true);
    for (size_t i = 0; i < m_impl->cfg.workerThreads; ++i)
        m_impl->workers.emplace_back([this]{ m_impl->workerLoop(); });
}

AssetStreamer::AssetStreamer(const AssetStreamerConfig& cfg) : m_impl(new Impl()) {
    m_impl->cfg = cfg;
    m_impl->running.store(true);
    for (size_t i = 0; i < cfg.workerThreads; ++i)
        m_impl->workers.emplace_back([this]{ m_impl->workerLoop(); });
}

AssetStreamer::~AssetStreamer() {
    Shutdown();
    delete m_impl;
}

void AssetStreamer::SetConfig(const AssetStreamerConfig& cfg) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->cfg = cfg;
}
const AssetStreamerConfig& AssetStreamer::Config() const { return m_impl->cfg; }

AssetHandle AssetStreamer::Request(const std::string& path,
                                    StreamPriority priority) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    // Return existing handle if already tracked
    for (auto& [h, info] : m_impl->assets)
        if (info.path == path) return h;

    AssetHandle handle = m_impl->nextHandle.fetch_add(1);
    AssetInfo info;
    info.handle   = handle;
    info.path     = path;
    info.state    = AssetState::Queued;
    info.priority = priority;
    m_impl->assets[handle] = info;

    WorkItem item{handle, path, priority};
    // Insert sorted by priority (highest priority first)
    auto it = std::lower_bound(m_impl->queue.begin(), m_impl->queue.end(), item);
    m_impl->queue.insert(it, item);
    m_impl->cv.notify_one();
    return handle;
}

bool AssetStreamer::Cancel(AssetHandle handle) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    auto& q = m_impl->queue;
    auto it = std::find_if(q.begin(), q.end(),
        [handle](const WorkItem& w){ return w.handle == handle; });
    if (it == q.end()) return false;
    q.erase(it);
    auto ait = m_impl->assets.find(handle);
    if (ait != m_impl->assets.end()) ait->second.state = AssetState::Unloaded;
    return true;
}

void AssetStreamer::Touch(AssetHandle handle, StreamPriority newPriority) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    for (auto& item : m_impl->queue) {
        if (item.handle == handle) { item.priority = newPriority; break; }
    }
    auto it = m_impl->assets.find(handle);
    if (it != m_impl->assets.end()) it->second.priority = newPriority;
}

void AssetStreamer::Evict(AssetHandle handle) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    auto it = m_impl->assets.find(handle);
    if (it == m_impl->assets.end()) return;
    if (it->second.state == AssetState::Resident) {
        m_impl->residentBytes = m_impl->residentBytes > it->second.sizeBytes
            ? m_impl->residentBytes - it->second.sizeBytes : 0;
        it->second.state = AssetState::Unloaded;
        std::lock_guard<std::mutex> plock(m_impl->pendingMtx);
        m_impl->pending.push_back({Impl::PendingCallback::Kind::Evicted, handle, {}});
    }
}

void AssetStreamer::EvictAll() {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    for (auto& [h, info] : m_impl->assets) {
        if (info.state == AssetState::Resident) {
            info.state = AssetState::Unloaded;
            std::lock_guard<std::mutex> plock(m_impl->pendingMtx);
            m_impl->pending.push_back({Impl::PendingCallback::Kind::Evicted, h, {}});
        }
    }
    m_impl->residentBytes = 0;
}

void AssetStreamer::Update() {
    std::vector<Impl::PendingCallback> cbs;
    {
        std::lock_guard<std::mutex> lock(m_impl->pendingMtx);
        cbs.swap(m_impl->pending);
    }
    for (auto& cb : cbs) {
        switch (cb.kind) {
        case Impl::PendingCallback::Kind::Loaded:
            for (auto& fn : m_impl->loadedCbs) fn(cb.handle, cb.info);
            break;
        case Impl::PendingCallback::Kind::Evicted:
            for (auto& fn : m_impl->evictedCbs) fn(cb.handle);
            break;
        case Impl::PendingCallback::Kind::Failed:
            for (auto& fn : m_impl->failedCbs) fn(cb.handle, cb.info);
            break;
        }
    }
}

AssetState AssetStreamer::StateOf(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    auto it = m_impl->assets.find(handle);
    return it != m_impl->assets.end() ? it->second.state : AssetState::Unloaded;
}

const AssetInfo* AssetStreamer::InfoOf(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    auto it = m_impl->assets.find(handle);
    return it != m_impl->assets.end() ? &it->second : nullptr;
}

bool AssetStreamer::IsResident(AssetHandle handle) const {
    return StateOf(handle) == AssetState::Resident;
}

size_t AssetStreamer::ResidentCount() const {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    size_t n = 0;
    for (auto& [h, info] : m_impl->assets)
        if (info.state == AssetState::Resident) ++n;
    return n;
}

size_t AssetStreamer::QueueDepth() const {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    return m_impl->queue.size();
}

size_t AssetStreamer::ResidentBytes() const {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    return m_impl->residentBytes;
}

void AssetStreamer::OnLoaded(AssetLoadedCb cb)  { m_impl->loadedCbs.push_back(std::move(cb)); }
void AssetStreamer::OnEvicted(AssetEvictedCb cb) { m_impl->evictedCbs.push_back(std::move(cb)); }
void AssetStreamer::OnFailed(AssetFailedCb cb)   { m_impl->failedCbs.push_back(std::move(cb)); }

void AssetStreamer::Shutdown() {
    m_impl->running.store(false);
    m_impl->cv.notify_all();
    for (auto& t : m_impl->workers) if (t.joinable()) t.join();
    m_impl->workers.clear();
}

} // namespace Runtime
