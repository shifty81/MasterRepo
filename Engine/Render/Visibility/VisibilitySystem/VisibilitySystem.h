#pragma once
/**
 * @file VisibilitySystem.h
 * @brief PVS-style portal/sector graph: UpdateVisibility, IsVisible, frustum helper.
 *
 * Features:
 *   - Sector: AABB, list of connected portal ids
 *   - Portal: two sector ids, convex 2D quad (world-space), open/closed state
 *   - AddSector(sectorId, min[3], max[3])
 *   - AddPortal(portalId, sectorA, sectorB, verts[4][3])
 *   - UpdateVisibility(viewPos[3], viewSectorId): flood-fill via open portals
 *   - IsVisible(sectorId) → bool (result of last UpdateVisibility)
 *   - GetVisibleSectors() → vector of sector ids
 *   - FrustumCull: SetFrustumPlanes(planes[6][4]) — skip portals outside frustum
 *   - ClosePortal/OpenPortal(portalId)
 *   - FindSector(worldPos) → sectorId (-1 if none)
 *   - Reset()
 *
 * Typical usage:
 * @code
 *   VisibilitySystem vs;
 *   vs.Init();
 *   vs.AddSector(0, {0,0,0}, {10,5,10});
 *   vs.AddSector(1, {10,0,0}, {20,5,10});
 *   vs.AddPortal(0, 0, 1, verts);
 *   vs.UpdateVisibility({5,2,5}, 0);
 *   bool see1 = vs.IsVisible(1);
 * @endcode
 */

#include <cstdint>
#include <vector>

namespace Engine {

class VisibilitySystem {
public:
    VisibilitySystem();
    ~VisibilitySystem();

    void Init    ();
    void Shutdown();

    // Sector
    void AddSector   (int32_t sectorId, const float mn[3], const float mx[3]);
    void RemoveSector(int32_t sectorId);

    // Portal
    void AddPortal   (int32_t portalId, int32_t sectorA, int32_t sectorB,
                       const float verts[4][3]);
    void RemovePortal(int32_t portalId);
    void OpenPortal  (int32_t portalId);
    void ClosePortal (int32_t portalId);
    bool IsPortalOpen(int32_t portalId) const;

    // Visibility update
    void UpdateVisibility(const float viewPos[3], int32_t viewSectorId);

    // Query
    bool                 IsVisible        (int32_t sectorId) const;
    std::vector<int32_t> GetVisibleSectors() const;
    int32_t              FindSector       (const float worldPos[3]) const;

    // Frustum culling assist
    void SetFrustumPlanes(const float planes[6][4]);
    void ClearFrustum    ();

    void Reset();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
