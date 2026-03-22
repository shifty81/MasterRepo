#pragma once
/**
 * @file AssetStreamer.h
 * @brief Asynchronous asset streaming manager with priority queues and eviction.
 *
 * AssetStreamer decouples asset loading from the game loop:
 *   - AssetHandle: opaque 64-bit id referencing a streamable asset.
 *   - AssetState: Unloaded, Queued, Loading, Resident, Evicting, Failed.
 *   - Request(path, priority): enqueue an asset for load; returns AssetHandle.
 *   - Cancel(handle): remove from queue if not yet loading.
 *   - Touch(handle): boost priority of an already-queued or resident asset.
 *   - Evict(handle): mark resident asset for removal on the next GC pass.
 *   - Update(budget): process at most budget bytes of pending load work;
 *     call once per frame from the main thread.
 *   - Workers: configurable number of background I/O threads.
 *   - Budget: per-frame byte budget prevents hitching.
 *   - Cache: LRU eviction when total resident memory exceeds MemoryBudgetBytes.
 *   - Callbacks: OnLoaded, OnEvicted, OnFailed fired on the main thread.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace Runtime {

// ── Asset handle ──────────────────────────────────────────────────────────
using AssetHandle = uint64_t;
static constexpr AssetHandle kInvalidAssetHandle = 0;

// ── Asset state ───────────────────────────────────────────────────────────
enum class AssetState : uint8_t {
    Unloaded, Queued, Loading, Resident, Evicting, Failed
};
const char* AssetStateName(AssetState s);

// ── Load priority ─────────────────────────────────────────────────────────
enum class StreamPriority : uint8_t { Critical = 0, High = 1, Normal = 2, Low = 3 };

// ── Asset entry (public view) ─────────────────────────────────────────────
struct AssetInfo {
    AssetHandle  handle{kInvalidAssetHandle};
    std::string  path;
    AssetState   state{AssetState::Unloaded};
    StreamPriority priority{StreamPriority::Normal};
    size_t       sizeBytes{0};    ///< 0 until load completes
    double       loadTimeMs{0};
};

// ── Streamer config ───────────────────────────────────────────────────────
struct AssetStreamerConfig {
    size_t   workerThreads{2};
    size_t   memoryBudgetMB{512};        ///< Total resident memory limit
    size_t   frameBytesBudget{4 * 1024 * 1024}; ///< Per-Update() budget
    uint32_t maxQueueDepth{256};
};

// ── Callbacks ─────────────────────────────────────────────────────────────
using AssetLoadedCb  = std::function<void(AssetHandle, const std::string& path)>;
using AssetEvictedCb = std::function<void(AssetHandle)>;
using AssetFailedCb  = std::function<void(AssetHandle, const std::string& error)>;

// ── Streamer ──────────────────────────────────────────────────────────────
class AssetStreamer {
public:
    AssetStreamer();
    explicit AssetStreamer(const AssetStreamerConfig& cfg);
    ~AssetStreamer();

    AssetStreamer(const AssetStreamer&) = delete;
    AssetStreamer& operator=(const AssetStreamer&) = delete;

    // ── configuration ─────────────────────────────────────────
    void SetConfig(const AssetStreamerConfig& cfg);
    const AssetStreamerConfig& Config() const;

    // ── streaming control ─────────────────────────────────────
    AssetHandle   Request(const std::string& path,
                          StreamPriority priority = StreamPriority::Normal);
    bool          Cancel(AssetHandle handle);
    void          Touch(AssetHandle handle,
                        StreamPriority newPriority = StreamPriority::High);
    void          Evict(AssetHandle handle);
    void          EvictAll();

    // ── per-frame tick ────────────────────────────────────────
    /// Process pending loads up to the configured frame byte budget.
    /// Call once per frame on the main thread.
    void Update();

    // ── queries ───────────────────────────────────────────────
    AssetState          StateOf(AssetHandle handle) const;
    const AssetInfo*    InfoOf(AssetHandle handle) const;
    bool                IsResident(AssetHandle handle) const;
    size_t              ResidentCount() const;
    size_t              QueueDepth() const;
    size_t              ResidentBytes() const;

    // ── callbacks ─────────────────────────────────────────────
    void OnLoaded(AssetLoadedCb cb);
    void OnEvicted(AssetEvictedCb cb);
    void OnFailed(AssetFailedCb cb);

    // ── shutdown ──────────────────────────────────────────────
    void Shutdown();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
