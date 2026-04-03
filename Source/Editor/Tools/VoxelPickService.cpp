#include "Editor/Tools/VoxelPickService.h"

namespace nf
{
    VoxelPickResult VoxelPickService::PickVoxel(
        const Camera&,
        const Rect& viewportRect,
        const Vec2& mousePosition,
        const GameWorld&) const
    {
        VoxelPickResult result {};
        if (!IsPointInsideRect(viewportRect, mousePosition))
        {
            return result;
        }

        // Scaffold only.
        // This marks the service integration point. Real implementation should:
        // 1. convert mousePosition to viewport-local coordinates
        // 2. derive NDC
        // 3. build a world ray from camera
        // 4. raymarch voxel storage in GameWorld
        // 5. fill hit, voxelCoord, adjacentVoxelCoord, normal, distance
        return result;
    }

    bool VoxelPickService::IsPointInsideRect(const Rect& rect, const Vec2& point)
    {
        return point.x >= rect.x
            && point.y >= rect.y
            && point.x <= rect.x + rect.width
            && point.y <= rect.y + rect.height;
    }
}
