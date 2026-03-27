#pragma once
/**
 * @file ObjectiveMarker.h
 * @brief World-space UI markers: register, project to screen, distance, fade.
 *
 * Features:
 *   - MarkerDef: id, label, category, color (RGBA), size
 *   - AddMarker(id, worldX, worldY, worldZ, label, color) → bool
 *   - RemoveMarker(id) / RemoveAll()
 *   - SetMarkerVisible(id, on)
 *   - SetMarkerPosition(id, wx, wy, wz)
 *   - SetMarkerLabel(id, label)
 *   - SetMarkerColor(id, r, g, b, a)
 *   - SetMarkerCategory(id, cat)
 *   - SetCategoryVisible(cat, on)
 *   - UpdateCamera(viewMat4[], projMat4[], screenW, screenH)
 *   - ProjectMarker(id, outSX, outSY, outDepth) → bool: behind camera = false
 *   - GetVisibleMarkers(out[]) → uint32_t: markers in front of camera
 *   - SetMaxDistance(d): hide markers beyond this world distance
 *   - SetFadeStart(d): begin fading at this world distance
 *   - GetDistance(id) → float: from last camera position
 *   - SetOnEnterScreen(cb) / SetOnExitScreen(cb)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

class ObjectiveMarker {
public:
    ObjectiveMarker();
    ~ObjectiveMarker();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Marker lifecycle
    bool AddMarker   (uint32_t id, float wx, float wy, float wz,
                      const std::string& label,
                      float r = 1, float g = 1, float b = 1, float a = 1);
    void RemoveMarker(uint32_t id);
    void RemoveAll   ();

    // Properties
    void SetMarkerVisible (uint32_t id, bool on);
    void SetMarkerPosition(uint32_t id, float wx, float wy, float wz);
    void SetMarkerLabel   (uint32_t id, const std::string& label);
    void SetMarkerColor   (uint32_t id, float r, float g, float b, float a);
    void SetMarkerCategory(uint32_t id, uint32_t cat);
    void SetCategoryVisible(uint32_t cat, bool on);

    // Camera
    void UpdateCamera(const float viewMat4[16], const float projMat4[16],
                      float screenW, float screenH);

    // Projection
    bool     ProjectMarker  (uint32_t id,
                              float& outSX, float& outSY, float& outDepth) const;
    uint32_t GetVisibleMarkers(std::vector<uint32_t>& out) const;
    float    GetDistance    (uint32_t id) const;

    // Config
    void SetMaxDistance(float d);
    void SetFadeStart  (float d);

    // Callbacks
    void SetOnEnterScreen(std::function<void(uint32_t id)> cb);
    void SetOnExitScreen (std::function<void(uint32_t id)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
