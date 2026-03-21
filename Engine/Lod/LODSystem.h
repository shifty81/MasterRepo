#pragma once
/**
 * @file LODSystem.h
 * @brief Level-of-Detail manager for the engine render pipeline.
 *
 * LODSystem maintains per-object LOD bands and a camera-distance priority
 * queue.  Each frame the caller updates the camera position and calls
 * Evaluate() to obtain the active LOD index for each registered object.
 * Results feed directly into RenderPipeline LOD selection.
 *
 * Design:
 *   - Objects are identified by a 32-bit ObjectID.
 *   - Each object has N LOD bands; band 0 is highest detail.
 *   - Band selection is purely distance-based (squared distance for perf).
 *   - A hysteresis margin prevents flickering at band boundaries.
 */

#include <cstdint>
#include <vector>
#include <functional>
#include <unordered_map>

namespace Engine {

using ObjectID = uint32_t;

struct LODBand {
    float     maxDistanceSq{0.0f};  ///< Upper bound (squared) for this band
    float     screenSizeHint{1.0f}; ///< Normalised screen-size hint [0,1]
};

struct LODEntry {
    ObjectID             id{0};
    std::vector<LODBand> bands;      ///< Ordered low-distance → high-distance
    int32_t              currentBand{0};
    bool                 alwaysVisible{false};
};

struct LODResult {
    ObjectID id{0};
    int32_t  band{0};        ///< Active LOD band index
    float    distanceSq{0.0f};
    bool     culled{false};  ///< True if beyond all bands (should not render)
};

using LODChangedCallback = std::function<void(ObjectID, int32_t /*newBand*/)>;

/// LODSystem — distance-driven level-of-detail manager.
class LODSystem {
public:
    LODSystem();
    ~LODSystem();

    // ── registration ─────────────────────────────────────────
    void RegisterObject(ObjectID id, std::vector<LODBand> bands,
                        bool alwaysVisible = false);
    void UnregisterObject(ObjectID id);
    bool IsRegistered(ObjectID id) const;
    size_t ObjectCount() const;

    // ── per-frame ─────────────────────────────────────────────
    /// Update the camera origin (must be called before Evaluate).
    void SetCameraPosition(float x, float y, float z);

    /// Update the world-space position of a specific object.
    void SetObjectPosition(ObjectID id, float x, float y, float z);

    /// Evaluate LOD for all registered objects.
    /// Returns results sorted by ascending distanceSq (nearest first).
    const std::vector<LODResult>& Evaluate();

    // ── query ─────────────────────────────────────────────────
    int32_t GetBand(ObjectID id) const;
    bool    IsCulled(ObjectID id) const;

    // ── tuning ────────────────────────────────────────────────
    void SetHysteresis(float margin);  ///< Prevents band ping-pong
    void SetMaxCullDistance(float d);  ///< Objects beyond this are culled

    // ── callbacks ─────────────────────────────────────────────
    void OnLODChanged(LODChangedCallback cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
