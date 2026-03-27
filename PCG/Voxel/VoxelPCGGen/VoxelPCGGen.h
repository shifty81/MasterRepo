#pragma once
/**
 * @file VoxelPCGGen.h
 * @brief PCG driver that fills a VoxelVolume using the project's PCG systems.
 *
 * VoxelPCGGen bridges:
 *  ┌─────────────────────────────────────────────────────┐
 *  │  NoiseGenerator / FractalNoise  → terrain height    │
 *  │  BiomeGenerator                 → material palette  │
 *  │  CaveGenerator                  → cave carving      │
 *  │  StructureGenerator             → room stamps       │
 *  │  PlanetGenerator (params)       → planet surface    │
 *  └─────────────────────────────────────────────────────┘
 *
 * Usage:
 *   VoxelVolume vol;
 *   VoxelPCGGen gen;
 *   gen.SetSeed(42);
 *   gen.GenerateTerrain(vol, {0,0,0}, {256,128,256});  // world-block coords
 *   gen.CarveCaves(vol, {0,0,0}, {256,128,256});
 *   gen.StampStructure(vol, room, {originX,originY,originZ});
 *   gen.GeneratePlanetSurface(vol, {0,0,0}, radius_blocks, biomeId);
 *   gen.GenerateAsteroid(vol, {cx,cy,cz}, radius_blocks);
 *   gen.GenerateShipHull(vol, {0,0,0}, {w,h,d});
 */

#include "PCG/Voxel/VoxelChiselSystem/VoxelChiselSystem.h"
#include "PCG/Structures/StructureGenerator/StructureGenerator.h"
#include <cmath>
#include <cstdint>
#include <string>

namespace PCG {

// Simple inline noise helpers (avoids heavy dependency in header)
namespace detail {
    inline float Hash(int32_t x, int32_t y, int32_t z, uint64_t seed) {
        uint64_t h = (uint64_t)(x * 1664525ull + y * 22695477ull + z * 6364136223846793005ull + seed);
        h ^= h >> 33; h *= 0xff51afd7ed558ccdULL; h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ULL; h ^= h >> 33;
        return (float)(h & 0xFFFFF) / (float)0xFFFFF;
    }

    // 3D octave noise in [0,1]
    inline float OctaveNoise(float wx, float wy, float wz, int octaves, float freq, uint64_t seed) {
        float value = 0.f, amp = 0.5f, maxV = 0.f;
        for (int o = 0; o < octaves; ++o) {
            int ix = (int)(wx * freq), iy = (int)(wy * freq), iz = (int)(wz * freq);
            float fx = wx*freq - ix, fy = wy*freq - iy, fz = wz*freq - iz;
            // Tri-linear interpolation of 8 corners
            float c[8];
            for (int k=0;k<2;++k) for(int j=0;j<2;++j) for(int i=0;i<2;++i)
                c[k*4+j*2+i] = Hash(ix+i, iy+j, iz+k, seed + o*997ull);
            auto lerp = [](float a, float b, float t){ return a + t*(b-a); };
            float v = lerp(lerp(lerp(c[0],c[1],fx),lerp(c[2],c[3],fx),fy),
                           lerp(lerp(c[4],c[5],fx),lerp(c[6],c[7],fx),fy), fz);
            value += v * amp; maxV += amp; amp *= 0.5f; freq *= 2.f;
        }
        return (maxV > 0.f) ? value / maxV : 0.f;
    }
} // namespace detail

// PCG configuration for terrain generation
struct VoxelTerrainConfig {
    uint64_t seed            = 42;
    int      octaves         = 6;
    float    frequency       = 0.03f;     ///< base frequency (per block)
    float    heightScale     = 64.f;      ///< max terrain height in blocks
    float    seaLevelFrac    = 0.35f;     ///< sea level as fraction of heightScale
    bool     generateCaves   = true;
    float    caveDensity     = 0.42f;     ///< noise threshold for caves
    bool     generateOre     = true;
    float    oreDensity      = 0.06f;     ///< fraction of stone replaced by ore
};

class VoxelPCGGen {
public:
    VoxelPCGGen() = default;

    void SetSeed(uint64_t seed) { m_seed = seed; }
    uint64_t GetSeed() const    { return m_seed; }

    // ── Terrain ──────────────────────────────────────────────────────────
    /**
     * Fill terrain in a volume of VoxelBlocks [origin..origin+size).
     * Coords are in VoxelBlock space (each block = 16 sub-voxels = 1 m).
     */
    void GenerateTerrain(VoxelVolume& vol,
                         IVec3 originBlocks, IVec3 sizeBlocks,
                         const VoxelTerrainConfig& cfg = {}) const
    {
        const int bsx = sizeBlocks.x, bsy = sizeBlocks.y, bsz = sizeBlocks.z;
        const int ox  = originBlocks.x, oy = originBlocks.y, oz = originBlocks.z;

        for (int bz = 0; bz < bsz; ++bz)
        for (int bx = 0; bx < bsx; ++bx) {
            // Heightmap at each XZ column
            float nx = (float)(ox + bx), nz = (float)(oz + bz);
            float n  = detail::OctaveNoise(nx, 0.f, nz, cfg.octaves, cfg.frequency, m_seed);
            int   surfaceBlock = oy + (int)(n * cfg.heightScale);
            float seaBlock     = oy + cfg.seaLevelFrac * cfg.heightScale;

            for (int by = 0; by < bsy; ++by) {
                int   wy  = oy + by;
                bool  above = (wy > surfaceBlock);
                bool  surf  = (wy == surfaceBlock);
                bool  below = (wy < surfaceBlock);

                VoxelMaterial mat = VoxelMaterial::Air;
                if (above && wy < (int)seaBlock) mat = VoxelMaterial::Water;
                else if (surf)  mat = (wy < (int)seaBlock) ? VoxelMaterial::Sand : VoxelMaterial::Grass;
                else if (below) {
                    int depth = surfaceBlock - wy;
                    if (depth < 3)      mat = VoxelMaterial::Dirt;
                    else if (depth < 10) mat = VoxelMaterial::Stone;
                    else                mat = VoxelMaterial::Rock;

                    // Ore pockets
                    if (cfg.generateOre && mat == VoxelMaterial::Rock) {
                        float on = detail::Hash(ox+bx, wy, oz+bz, m_seed + 777);
                        if (on < cfg.oreDensity * 0.5f)       mat = VoxelMaterial::Ore_Iron;
                        else if (on < cfg.oreDensity)         mat = VoxelMaterial::Ore_Crystal;
                        else if (on < cfg.oreDensity * 1.3f)  mat = VoxelMaterial::Ore_Gold;
                    }
                }

                if (mat == VoxelMaterial::Air) continue; // skip air blocks

                // Sub-voxel coords: each block is 16×16×16
                int32_t svX0 = (ox + bx) * VoxelBlock::SIZE;
                int32_t svY0 = wy        * VoxelBlock::SIZE;
                int32_t svZ0 = (oz + bz) * VoxelBlock::SIZE;

                // Surface block: partial fill (sub-voxel heightmap)
                if (surf) {
                    float subN = detail::OctaveNoise(nx * 16.f, 0.f, nz * 16.f, 2, 0.1f, m_seed+1);
                    uint8_t fillY = (uint8_t)(subN * VoxelBlock::SIZE);
                    vol.FillBox(svX0, svY0, svZ0,
                                svX0 + VoxelBlock::SIZE - 1, svY0 + fillY,
                                svZ0 + VoxelBlock::SIZE - 1,
                                mat);
                } else {
                    vol.FillBox(svX0, svY0, svZ0,
                                svX0 + VoxelBlock::SIZE - 1,
                                svY0 + VoxelBlock::SIZE - 1,
                                svZ0 + VoxelBlock::SIZE - 1,
                                mat);
                }
            }
        }

        // Cave carving pass
        if (cfg.generateCaves) CarveCaves(vol, originBlocks, sizeBlocks, cfg);
    }

    // ── Caves ─────────────────────────────────────────────────────────────
    void CarveCaves(VoxelVolume& vol,
                    IVec3 originBlocks, IVec3 sizeBlocks,
                    const VoxelTerrainConfig& cfg = {}) const
    {
        const float threshold = cfg.caveDensity;
        for (int bz = 0; bz < sizeBlocks.z; ++bz)
        for (int by = 0; by < sizeBlocks.y; ++by)
        for (int bx = 0; bx < sizeBlocks.x; ++bx) {
            float n = detail::OctaveNoise(
                (float)(originBlocks.x + bx) * 0.8f,
                (float)(originBlocks.y + by) * 0.8f,
                (float)(originBlocks.z + bz) * 0.8f,
                3, 0.12f, m_seed + 1234);
            if (n > threshold) {
                int32_t sx = (originBlocks.x + bx) * VoxelBlock::SIZE;
                int32_t sy = (originBlocks.y + by) * VoxelBlock::SIZE;
                int32_t sz = (originBlocks.z + bz) * VoxelBlock::SIZE;
                // Carve a sphere within the block for organic cave feel
                int32_t mid = VoxelBlock::SIZE / 2;
                float r = (n - threshold) / (1.f - threshold) * (VoxelBlock::SIZE * 0.7f);
                for (int k = 0; k < VoxelBlock::SIZE; ++k)
                for (int j = 0; j < VoxelBlock::SIZE; ++j)
                for (int i = 0; i < VoxelBlock::SIZE; ++i) {
                    float dx = i - mid, dy = j - mid, dz = k - mid;
                    if (dx*dx + dy*dy + dz*dz <= r*r)
                        vol.SetCell(sx+i, sy+j, sz+k, VoxelMaterial::Air);
                }
            }
        }
    }

    // ── Stamp a StructureMap into the volume ──────────────────────────────
    void StampStructure(VoxelVolume& vol, const StructureMap& map,
                        IVec3 worldBlockOrigin) const
    {
        VoxelMaterial wallMat  = VoxelMaterial::Metal_Hull;
        VoxelMaterial floorMat = VoxelMaterial::Metal_Plate;

        for (auto& room : map.rooms) {
            // Fill room volume with floor
            for (int bz = room.boundsMin.z; bz <= room.boundsMax.z; ++bz)
            for (int by = room.boundsMin.y; by <= room.boundsMax.y; ++by)
            for (int bx = room.boundsMin.x; bx <= room.boundsMax.x; ++bx) {
                bool isWall = (bx == room.boundsMin.x || bx == room.boundsMax.x ||
                               bz == room.boundsMin.z || bz == room.boundsMax.z);
                bool isCeil = (by == room.boundsMax.y);
                bool isFloor= (by == room.boundsMin.y);
                VoxelMaterial mat = (isWall || isCeil || isFloor) ? wallMat : VoxelMaterial::Air;
                if (isFloor) mat = floorMat;
                if (mat == VoxelMaterial::Air) continue;
                int32_t sx = (worldBlockOrigin.x + bx) * VoxelBlock::SIZE;
                int32_t sy = (worldBlockOrigin.y + by) * VoxelBlock::SIZE;
                int32_t sz = (worldBlockOrigin.z + bz) * VoxelBlock::SIZE;
                vol.FillBox(sx, sy, sz,
                            sx + VoxelBlock::SIZE - 1, sy + VoxelBlock::SIZE - 1, sz + VoxelBlock::SIZE - 1,
                            mat);
            }
        }
    }

    // ── Planet surface shell ──────────────────────────────────────────────
    /**
     * Generate a spherical planet surface.
     * centre and radiusBlocks are in VoxelBlock space.
     */
    void GeneratePlanetSurface(VoxelVolume& vol,
                               IVec3 centreBlocks, int32_t radiusBlocks,
                               VoxelMaterial surfaceMat = VoxelMaterial::Rock,
                               VoxelMaterial coreMat    = VoxelMaterial::Stone) const
    {
        int32_t r = radiusBlocks, r2 = r * r;
        int32_t shell = 3; // shell thickness in blocks

        for (int32_t bz = -r; bz <= r; ++bz)
        for (int32_t by = -r; by <= r; ++by)
        for (int32_t bx = -r; bx <= r; ++bx) {
            int32_t dist2 = bx*bx + by*by + bz*bz;
            if (dist2 > r2) continue;

            bool isSurface = (dist2 >= (r-shell)*(r-shell));
            VoxelMaterial mat = isSurface ? surfaceMat : coreMat;

            // Noise perturbation on surface
            if (isSurface) {
                float n = detail::OctaveNoise(
                    (float)(centreBlocks.x + bx) * 0.3f,
                    (float)(centreBlocks.y + by) * 0.3f,
                    (float)(centreBlocks.z + bz) * 0.3f,
                    3, 0.2f, m_seed + 999);
                if (n > 0.6f) mat = VoxelMaterial::Ore_Crystal; // surface crystals
                if (n < 0.15f) mat = VoxelMaterial::Air;         // craters
            }

            int32_t wx = (centreBlocks.x + bx) * VoxelBlock::SIZE;
            int32_t wy = (centreBlocks.y + by) * VoxelBlock::SIZE;
            int32_t wz = (centreBlocks.z + bz) * VoxelBlock::SIZE;
            if (mat != VoxelMaterial::Air)
                vol.FillBox(wx, wy, wz,
                            wx + VoxelBlock::SIZE - 1,
                            wy + VoxelBlock::SIZE - 1,
                            wz + VoxelBlock::SIZE - 1,
                            mat);
        }
    }

    // ── Procedural asteroid ───────────────────────────────────────────────
    void GenerateAsteroid(VoxelVolume& vol, IVec3 centreBlocks,
                          int32_t radiusBlocks) const
    {
        int32_t r = radiusBlocks;
        for (int32_t bz = -r; bz <= r; ++bz)
        for (int32_t by = -r; by <= r; ++by)
        for (int32_t bx = -r; bx <= r; ++bx) {
            float n = detail::OctaveNoise(
                (float)(centreBlocks.x+bx)*0.5f,
                (float)(centreBlocks.y+by)*0.5f,
                (float)(centreBlocks.z+bz)*0.5f,
                4, 0.3f, m_seed + 54321);
            float dist = std::sqrt((float)(bx*bx + by*by + bz*bz));
            if (dist > (float)r * (0.5f + n * 0.5f)) continue;
            VoxelMaterial mat = VoxelMaterial::Rock;
            if (n > 0.7f) mat = VoxelMaterial::Ore_Iron;
            if (n > 0.88f) mat = VoxelMaterial::Ore_Gold;
            int32_t wx = (centreBlocks.x+bx)*VoxelBlock::SIZE;
            int32_t wy = (centreBlocks.y+by)*VoxelBlock::SIZE;
            int32_t wz = (centreBlocks.z+bz)*VoxelBlock::SIZE;
            vol.FillBox(wx,wy,wz, wx+VoxelBlock::SIZE-1, wy+VoxelBlock::SIZE-1, wz+VoxelBlock::SIZE-1, mat);
        }
    }

    // ── Ship hull (hollow rectangular shell) ─────────────────────────────
    void GenerateShipHull(VoxelVolume& vol, IVec3 originBlocks,
                          IVec3 sizeBlocks,
                          VoxelMaterial hullMat   = VoxelMaterial::Metal_Hull,
                          VoxelMaterial floorMat  = VoxelMaterial::Metal_Plate) const
    {
        int sx = sizeBlocks.x, sy = sizeBlocks.y, sz = sizeBlocks.z;
        for (int bz = 0; bz < sz; ++bz)
        for (int by = 0; by < sy; ++by)
        for (int bx = 0; bx < sx; ++bx) {
            bool wall = (bx==0||bx==sx-1||by==0||by==sy-1||bz==0||bz==sz-1);
            if (!wall) continue;
            VoxelMaterial mat = (by == 0) ? floorMat : hullMat;
            int32_t wx = (originBlocks.x+bx)*VoxelBlock::SIZE;
            int32_t wy = (originBlocks.y+by)*VoxelBlock::SIZE;
            int32_t wz = (originBlocks.z+bz)*VoxelBlock::SIZE;
            vol.FillBox(wx,wy,wz, wx+VoxelBlock::SIZE-1, wy+VoxelBlock::SIZE-1, wz+VoxelBlock::SIZE-1, mat);
        }
    }

private:
    uint64_t m_seed = 42;
};

} // namespace PCG
