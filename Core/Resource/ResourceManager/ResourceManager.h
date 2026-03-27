#pragma once
/**
 * @file ResourceManager.h
 * @brief Ref-counted async resource cache: Load/Unload/Get, hot-swap, memory budget, eviction.
 *
 * Features:
 *   - Resource<T>: shared ownership handle; releases when last ref drops
 *   - Load<T>(path, cb) → ResourceHandle (non-blocking, fires cb when ready)
 *   - LoadSync<T>(path) → Resource<T>
 *   - Get<T>(path) → Resource<T> (nullptr if not loaded)
 *   - Unload(path): mark as evictable (freed when no refs)
 *   - Reload(path): hot-swap; existing handles see new data
 *   - SetMemoryBudget(bytes): trigger LRU eviction when exceeded
 *   - GetMemoryUsage() → bytes
 *   - GetStats() → loaded count, pending count, total bytes
 *   - RegisterLoader<T>(ext, loaderFn)
 *   - WaitAll(): block until all pending loads complete
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Core {

using ResourceHandle = uint64_t;

struct ResourceStats {
    uint32_t loaded {0};
    uint32_t pending{0};
    uint64_t bytes  {0};
};

// Opaque resource base
struct ResourceBase {
    std::string path;
    uint64_t    sizeBytes{0};
    virtual ~ResourceBase() = default;
};

// Typed resource alias
template<typename T>
using Resource = std::shared_ptr<T>;

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    void Init    ();
    void Shutdown();
    void Tick    (float dt); ///< dispatch pending callbacks

    // Loader registration
    using LoaderFn = std::function<std::shared_ptr<ResourceBase>(const std::string& path)>;
    void RegisterLoader(const std::string& ext, LoaderFn fn);

    // Async load
    ResourceHandle LoadAsync(const std::string& path,
                              std::function<void(std::shared_ptr<ResourceBase>)> cb);

    // Sync load
    std::shared_ptr<ResourceBase> LoadSync(const std::string& path);

    // Access
    std::shared_ptr<ResourceBase> Get    (const std::string& path) const;
    bool                          IsLoaded(const std::string& path) const;
    bool                          IsPending(const std::string& path) const;

    // Lifecycle
    void Unload(const std::string& path);
    void Reload(const std::string& path);
    void UnloadAll();

    // Memory
    void     SetMemoryBudget(uint64_t bytes);
    uint64_t GetMemoryUsage () const;
    void     TriggerEviction();

    // Stats
    ResourceStats GetStats() const;

    void WaitAll();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
