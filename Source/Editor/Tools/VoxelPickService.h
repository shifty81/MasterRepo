#pragma once

namespace nf
{
    struct Int3
    {
        int x = 0;
        int y = 0;
        int z = 0;
    };

    struct Vec2
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct Vec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct Rect
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    class Camera;
    class GameWorld;

    struct VoxelPickResult
    {
        bool hit = false;
        Int3 chunkCoord {};
        Int3 voxelCoord {};
        Int3 adjacentVoxelCoord {};
        Vec3 worldPosition {};
        Vec3 normal {};
        float distance = 0.0f;
    };

    class VoxelPickService
    {
    public:
        VoxelPickResult PickVoxel(
            const Camera& camera,
            const Rect& viewportRect,
            const Vec2& mousePosition,
            const GameWorld& world) const;

    private:
        static bool IsPointInsideRect(const Rect& rect, const Vec2& point);
    };
}
