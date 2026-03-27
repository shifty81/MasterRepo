#pragma once
/**
 * @file PortalSystem.h
 * @brief Portal pairs: teleport remap, render culling rect, linked chain traversal.
 *
 * Features:
 *   - Portal pair: two portals (A and B), each with world position + orientation
 *   - TransformThrough(portalId, worldPos, worldDir) → remapped pos + dir
 *   - TeleportEntity: entity crosses portal → computes exit transform
 *   - Render culling: GetScissorRect(portalId, viewPos, viewDir) → screen rect
 *   - Recursive portal depth limit (max 2 hops by default)
 *   - PortalCamera: virtual camera for portal render pass
 *   - Link / Unlink portals
 *   - One-way portals (entry-only flag)
 *   - On-teleport callback
 *   - Multiple portal pairs
 *
 * Typical usage:
 * @code
 *   PortalSystem ps;
 *   ps.Init();
 *   uint32_t pA = ps.CreatePortal({0,1,0}, {0,0,1});
 *   uint32_t pB = ps.CreatePortal({10,1,0}, {0,0,-1});
 *   ps.Link(pA, pB);
 *   ps.SetOnTeleport([](uint32_t entity, uint32_t portal){ Log("tp"); });
 *   if (ps.CheckCross(entityId, prevPos, newPos)) ps.TeleportEntity(entityId, pA);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct PortalTransform {
    float position   [3]{};
    float orientation[4]{0,0,0,1};  ///< quaternion
};

struct PortalScissor {
    float minX{-1}, minY{-1}, maxX{1}, maxY{1}; ///< NDC [-1,1]
};

class PortalSystem {
public:
    PortalSystem();
    ~PortalSystem();

    void Init    ();
    void Shutdown();

    // Portal lifecycle
    uint32_t CreatePortal(const float worldPos[3], const float forward[3],
                           float width=2.f, float height=3.f);
    void     DestroyPortal(uint32_t id);
    bool     HasPortal    (uint32_t id) const;

    // Linking
    void Link  (uint32_t portalA, uint32_t portalB);
    void Unlink(uint32_t id);
    uint32_t LinkedPortal(uint32_t id) const;  ///< returns 0 if unlinked

    // Teleportation
    bool           CheckCross   (uint32_t portalId,
                                  const float prevPos[3], const float newPos[3]) const;
    PortalTransform TransformThrough(uint32_t portalId, const float inPos[3],
                                      const float inDir[3]) const;
    void           TeleportEntity(uint32_t entityId, uint32_t portalId,
                                   float inPos[3], float inDir[3]);

    // Rendering
    PortalScissor  GetScissorRect(uint32_t portalId,
                                   const float viewPos[3], const float viewDir[3]) const;
    PortalTransform GetVirtualCamera(uint32_t portalId,
                                      const float camPos[3], const float camDir[3]) const;

    // Flags
    void SetOneWay(uint32_t id, bool oneWay);
    void SetMaxRecursionDepth(uint32_t depth);

    // Callback
    void SetOnTeleport(std::function<void(uint32_t entityId, uint32_t portalId)> cb);

    std::vector<uint32_t> GetAll() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
