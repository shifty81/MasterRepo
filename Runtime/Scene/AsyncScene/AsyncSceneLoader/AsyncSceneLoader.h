#pragma once
/**
 * @file AsyncSceneLoader.h
 * @brief Background scene loading with progress tracking, cancellation, and swap-in callback.
 *
 * Features:
 *   - LoadAsync(scenePath, priority) → LoadHandle: kick off background load
 *   - Cancel(handle): cancel a pending or in-progress load
 *   - GetProgress(handle) → float [0,1]: bytes/entities loaded fraction
 *   - IsComplete(handle) → bool
 *   - IsCancelled(handle) → bool
 *   - SwapIn(handle): promote loaded scene to active (must call from main thread)
 *   - SetOnComplete(handle, cb): callback fired when load finishes (bg thread)
 *   - SetOnSwapIn(cb): global callback fired on SwapIn (main thread)
 *   - Tick(dt): pumps async queue (single-threaded stub; for real use integrate with JobSystem)
 *   - GetActiveScene() → scene descriptor string
 *   - PrefetchAsset(path): warm asset cache before full scene load
 *   - Shutdown(): cancel all pending loads and free resources
 */

#include <cstdint>
#include <functional>
#include <string>

namespace Runtime {

using LoadHandle = uint32_t;
static constexpr LoadHandle kInvalidLoadHandle = 0;

struct SceneDesc {
    std::string path;
    uint32_t    entityCount{0};
    bool        loaded{false};
};

class AsyncSceneLoader {
public:
    AsyncSceneLoader();
    ~AsyncSceneLoader();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Loading
    LoadHandle LoadAsync(const std::string& scenePath, int priority=0);
    void       Cancel   (LoadHandle h);

    // Status
    float GetProgress  (LoadHandle h) const; ///< [0,1]
    bool  IsComplete   (LoadHandle h) const;
    bool  IsCancelled  (LoadHandle h) const;
    bool  IsLoading    (LoadHandle h) const;

    // Swap-in: promote the completed scene to active
    bool  SwapIn(LoadHandle h);

    // Asset prefetch (no-op stub; just marks path as warm)
    void PrefetchAsset(const std::string& path);

    // Callbacks
    void SetOnComplete(LoadHandle h, std::function<void(LoadHandle)> cb);
    void SetOnSwapIn  (std::function<void(const SceneDesc&)> cb);

    // Active scene
    const SceneDesc* GetActiveScene() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
