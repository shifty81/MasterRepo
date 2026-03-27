#pragma once
/**
 * @file VoxelChiselSystem.h
 * @brief 16×16×16 sub-voxel chisel block system.
 *
 * Architecture
 * ────────────
 *  VoxelMaterial  – material enum (Air, Stone, Dirt, Metal, Crystal …)
 *  VoxelCell      – 2-byte sub-voxel: material + density (0=empty, 255=full)
 *  VoxelBlock     – 16³ = 4 096 VoxelCells packed flat (z*256 + y*16 + x)
 *  VoxelChunk     – CHUNK_BLOCKS³ array of VoxelBlocks (default 4³ = 64 blocks)
 *  VoxelVolume    – sparse map of chunks keyed by (cx,cy,cz); lazy allocation
 *  VoxelChiselOp  – atomic edit operation (set/fill/sphere/box) for undo support
 *
 * Each VoxelBlock covers kBlockWorldSize world-units (default 1.0 m).
 * Each sub-voxel cell therefore covers kBlockWorldSize / 16 ≈ 0.0625 m.
 *
 * PCG integration
 * ───────────────
 *  VoxelChiselSystem exposes FillFromHeightmap(), FillFromNoise(),
 *  StampStructure(), and CarveBox() so PCG generators can sculpt arbitrary
 *  volumes at sub-voxel precision.
 */

#include <cstdint>
#include <array>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cmath>

namespace PCG {

// ── Material palette ──────────────────────────────────────────────────────
enum class VoxelMaterial : uint8_t {
    Air          = 0,
    Stone        = 1,
    Dirt         = 2,
    Grass        = 3,
    Sand         = 4,
    Gravel       = 5,
    Rock         = 6,
    Ore_Iron     = 7,
    Ore_Crystal  = 8,
    Ore_Gold     = 9,
    Metal_Hull   = 10,
    Metal_Plate  = 11,
    Metal_Frame  = 12,
    Ice          = 13,
    Snow         = 14,
    Wood         = 15,
    Leaf         = 16,
    Water        = 17,
    Lava         = 18,
    GlowCrystal  = 19,
    Void         = 20,   ///< permanently empty (carved, never re-filled)
    _Count       = 21
};

/// Returns true for materials that block light/movement
inline bool VoxelIsSolid(VoxelMaterial m) {
    return m != VoxelMaterial::Air && m != VoxelMaterial::Water && m != VoxelMaterial::Void;
}

/// Returns true for emissive materials
inline bool VoxelIsEmissive(VoxelMaterial m) {
    return m == VoxelMaterial::Lava || m == VoxelMaterial::GlowCrystal;
}

/// Returns RGBA color hint for a material (for debug / editor preview)
inline uint32_t VoxelMaterialColor(VoxelMaterial m) {
    switch (m) {
        case VoxelMaterial::Stone:       return 0x888888FF;
        case VoxelMaterial::Dirt:        return 0x8B6040FF;
        case VoxelMaterial::Grass:       return 0x44AA44FF;
        case VoxelMaterial::Sand:        return 0xDDCC88FF;
        case VoxelMaterial::Gravel:      return 0x9A9070FF;
        case VoxelMaterial::Rock:        return 0x707070FF;
        case VoxelMaterial::Ore_Iron:    return 0xAA7755FF;
        case VoxelMaterial::Ore_Crystal: return 0x88DDFFFF;
        case VoxelMaterial::Ore_Gold:    return 0xFFCC00FF;
        case VoxelMaterial::Metal_Hull:  return 0x5599BBFF;
        case VoxelMaterial::Metal_Plate: return 0x4488AAFF;
        case VoxelMaterial::Metal_Frame: return 0x336677FF;
        case VoxelMaterial::Ice:         return 0xAADDFFFF;
        case VoxelMaterial::Snow:        return 0xEEEEFFFF;
        case VoxelMaterial::Wood:        return 0x996633FF;
        case VoxelMaterial::Leaf:        return 0x33AA33FF;
        case VoxelMaterial::Water:       return 0x2266AA88;
        case VoxelMaterial::Lava:        return 0xFF4400FF;
        case VoxelMaterial::GlowCrystal: return 0x44FFAAFF;
        default:                         return 0x00000000;
    }
}

// ── Sub-voxel cell (2 bytes) ──────────────────────────────────────────────
struct VoxelCell {
    VoxelMaterial material = VoxelMaterial::Air;
    uint8_t       density  = 0;    ///< 0=hollow air, 255=full solid

    bool IsSolid() const { return VoxelIsSolid(material); }
    bool IsAir()   const { return material == VoxelMaterial::Air; }
};

// ── 16×16×16 chisel block ─────────────────────────────────────────────────
struct VoxelBlock {
    static constexpr uint8_t  SIZE   = 16;
    static constexpr uint32_t VOLUME = SIZE * SIZE * SIZE; // 4096

    std::array<VoxelCell, VOLUME> cells;

    // Flat index: z is outermost, then y, then x
    static uint32_t Idx(uint8_t x, uint8_t y, uint8_t z) {
        return static_cast<uint32_t>(z) * SIZE * SIZE
             + static_cast<uint32_t>(y) * SIZE
             + static_cast<uint32_t>(x);
    }

    VoxelCell&       At(uint8_t x, uint8_t y, uint8_t z)       { return cells[Idx(x,y,z)]; }
    const VoxelCell& At(uint8_t x, uint8_t y, uint8_t z) const { return cells[Idx(x,y,z)]; }

    bool IsSolid(uint8_t x, uint8_t y, uint8_t z) const { return cells[Idx(x,y,z)].IsSolid(); }

    void Fill(VoxelMaterial mat, uint8_t density = 255) {
        for (auto& c : cells) { c.material = mat; c.density = (mat == VoxelMaterial::Air ? 0 : density); }
    }

    void Clear() { Fill(VoxelMaterial::Air, 0); }

    bool IsEmpty() const {
        for (auto& c : cells) if (c.material != VoxelMaterial::Air) return false;
        return true;
    }

    /// Chisel a sub-region [x0..x1, y0..y1, z0..z1] to given material
    void ChiselBox(uint8_t x0, uint8_t y0, uint8_t z0,
                   uint8_t x1, uint8_t y1, uint8_t z1,
                   VoxelMaterial mat, uint8_t density = 255)
    {
        for (uint8_t z = z0; z <= z1 && z < SIZE; ++z)
        for (uint8_t y = y0; y <= y1 && y < SIZE; ++y)
        for (uint8_t x = x0; x <= x1 && x < SIZE; ++x)
            At(x,y,z) = { mat, (mat == VoxelMaterial::Air ? uint8_t(0) : density) };
    }

    /// Chisel a sphere of radius r (in sub-voxel units) centred at (cx,cy,cz)
    void ChiselSphere(float cx, float cy, float cz, float r, VoxelMaterial mat) {
        int i0 = std::max(0, (int)(cx - r));
        int i1 = std::min((int)SIZE - 1, (int)(cx + r));
        int j0 = std::max(0, (int)(cy - r));
        int j1 = std::min((int)SIZE - 1, (int)(cy + r));
        int k0 = std::max(0, (int)(cz - r));
        int k1 = std::min((int)SIZE - 1, (int)(cz + r));
        float r2 = r * r;
        for (int k = k0; k <= k1; ++k)
        for (int j = j0; j <= j1; ++j)
        for (int i = i0; i <= i1; ++i) {
            float dx = i - cx, dy = j - cy, dz = k - cz;
            if (dx*dx + dy*dy + dz*dz <= r2)
                At((uint8_t)i, (uint8_t)j, (uint8_t)k) = {
                    mat, (mat == VoxelMaterial::Air ? uint8_t(0) : uint8_t(255))
                };
        }
    }

    VoxelBlock() { cells.fill({VoxelMaterial::Air, 0}); }
};

// ── Chunk key ─────────────────────────────────────────────────────────────
struct ChunkKey3D {
    int32_t cx = 0, cy = 0, cz = 0;
    bool operator==(const ChunkKey3D& o) const {
        return cx == o.cx && cy == o.cy && cz == o.cz;
    }
};

} // namespace PCG

namespace std {
template<> struct hash<PCG::ChunkKey3D> {
    size_t operator()(const PCG::ChunkKey3D& k) const noexcept {
        size_t h = (size_t)(k.cx * 73856093) ^ (size_t)(k.cy * 19349663) ^ (size_t)(k.cz * 83492791);
        return h;
    }
};
} // namespace std

namespace PCG {

// ── Voxel chunk (CHUNK_BLOCKS³ array of VoxelBlocks) ─────────────────────
class VoxelChunk {
public:
    static constexpr uint8_t  CHUNK_BLOCKS = 4;    ///< blocks per chunk side
    static constexpr uint32_t CHUNK_VOLUME = CHUNK_BLOCKS * CHUNK_BLOCKS * CHUNK_BLOCKS; // 64

    ChunkKey3D key;

    VoxelBlock& BlockAt(uint8_t bx, uint8_t by, uint8_t bz) {
        return m_blocks[bz * CHUNK_BLOCKS * CHUNK_BLOCKS + by * CHUNK_BLOCKS + bx];
    }
    const VoxelBlock& BlockAt(uint8_t bx, uint8_t by, uint8_t bz) const {
        return m_blocks[bz * CHUNK_BLOCKS * CHUNK_BLOCKS + by * CHUNK_BLOCKS + bx];
    }

    /// Access sub-voxel using chunk-local coordinates [0 .. CHUNK_BLOCKS*16)
    VoxelCell& CellAt(uint16_t x, uint16_t y, uint16_t z) {
        uint8_t bx = (uint8_t)(x / VoxelBlock::SIZE);
        uint8_t by = (uint8_t)(y / VoxelBlock::SIZE);
        uint8_t bz = (uint8_t)(z / VoxelBlock::SIZE);
        uint8_t sx = (uint8_t)(x % VoxelBlock::SIZE);
        uint8_t sy = (uint8_t)(y % VoxelBlock::SIZE);
        uint8_t sz = (uint8_t)(z % VoxelBlock::SIZE);
        return BlockAt(bx,by,bz).At(sx,sy,sz);
    }

    void FillAll(VoxelMaterial mat) {
        for (auto& b : m_blocks) b.Fill(mat);
    }

    bool IsDirty() const { return m_dirty; }
    void MarkDirty()     { m_dirty = true;  }
    void ClearDirty()    { m_dirty = false; }

private:
    std::array<VoxelBlock, CHUNK_VOLUME> m_blocks;
    bool m_dirty = false;
};

// ── Atomic edit op (for undo/redo support) ────────────────────────────────
struct VoxelChiselOp {
    enum class Type { SetCell, FillBlock, FillSphere, FillBox, CarveBox };
    Type          type = Type::SetCell;
    ChunkKey3D    chunk;
    int32_t       wx = 0, wy = 0, wz = 0;   ///< world-block coords
    uint8_t       x0 = 0, y0 = 0, z0 = 0;   ///< sub-voxel range start
    uint8_t       x1 = 0, y1 = 0, z1 = 0;   ///< sub-voxel range end
    VoxelMaterial material = VoxelMaterial::Air;
    uint8_t       density  = 255;
};

// ── Voxel volume — sparse map of chunks ───────────────────────────────────
class VoxelVolume {
public:
    /// World-space size of a single VoxelBlock (macro block)
    static constexpr float kBlockWorldSize = 1.0f;
    /// World-space size of a sub-voxel cell
    static constexpr float kCellWorldSize  = kBlockWorldSize / VoxelBlock::SIZE; // 0.0625 m

    VoxelVolume() = default;

    // ── Chunk allocation ─────────────────────────────────────────────────
    VoxelChunk* GetChunk(ChunkKey3D key) {
        auto it = m_chunks.find(key);
        return (it != m_chunks.end()) ? it->second.get() : nullptr;
    }

    VoxelChunk& GetOrCreateChunk(ChunkKey3D key) {
        auto& ptr = m_chunks[key];
        if (!ptr) { ptr = std::make_unique<VoxelChunk>(); ptr->key = key; }
        return *ptr;
    }

    size_t ChunkCount() const { return m_chunks.size(); }

    // ── World-coordinate cell access ─────────────────────────────────────
    /// Convert world voxel coord (block coord * 16 + sub-voxel) to chunk + local
    VoxelCell* GetCell(int32_t wx, int32_t wy, int32_t wz) {
        // wx,wy,wz are in sub-voxel space (world_block * 16 + sub)
        int32_t chunkSide = VoxelChunk::CHUNK_BLOCKS * VoxelBlock::SIZE;  // 64
        auto FloorDiv = [](int32_t a, int32_t b) -> int32_t {
            return (a / b) - (a % b != 0 && (a ^ b) < 0);
        };
        ChunkKey3D key{FloorDiv(wx, chunkSide), FloorDiv(wy, chunkSide), FloorDiv(wz, chunkSide)};
        auto* chunk = GetChunk(key);
        if (!chunk) return nullptr;
        int32_t lx = wx - key.cx * chunkSide;
        int32_t ly = wy - key.cy * chunkSide;
        int32_t lz = wz - key.cz * chunkSide;
        return &chunk->CellAt((uint16_t)lx, (uint16_t)ly, (uint16_t)lz);
    }

    void SetCell(int32_t wx, int32_t wy, int32_t wz, VoxelMaterial mat, uint8_t density = 255) {
        int32_t chunkSide = VoxelChunk::CHUNK_BLOCKS * VoxelBlock::SIZE;
        auto FloorDiv = [](int32_t a, int32_t b) -> int32_t {
            return (a / b) - (a % b != 0 && (a ^ b) < 0);
        };
        ChunkKey3D key{FloorDiv(wx, chunkSide), FloorDiv(wy, chunkSide), FloorDiv(wz, chunkSide)};
        auto& chunk = GetOrCreateChunk(key);
        int32_t lx = wx - key.cx * chunkSide;
        int32_t ly = wy - key.cy * chunkSide;
        int32_t lz = wz - key.cz * chunkSide;
        chunk.CellAt((uint16_t)lx, (uint16_t)ly, (uint16_t)lz) = {mat, mat == VoxelMaterial::Air ? uint8_t(0) : density};
        chunk.MarkDirty();
    }

    /// Fill an axis-aligned box in sub-voxel world space
    void FillBox(int32_t x0, int32_t y0, int32_t z0,
                 int32_t x1, int32_t y1, int32_t z1,
                 VoxelMaterial mat, uint8_t density = 255)
    {
        for (int32_t z = z0; z <= z1; ++z)
        for (int32_t y = y0; y <= y1; ++y)
        for (int32_t x = x0; x <= x1; ++x)
            SetCell(x, y, z, mat, density);
    }

    // Iterate all loaded chunks
    void ForEachChunk(std::function<void(const ChunkKey3D&, VoxelChunk&)> fn) {
        for (auto& [k, v] : m_chunks) fn(k, *v);
    }
    void ForEachChunk(std::function<void(const ChunkKey3D&, const VoxelChunk&)> fn) const {
        for (auto& [k, v] : m_chunks) fn(k, *v);
    }

private:
    std::unordered_map<ChunkKey3D, std::unique_ptr<VoxelChunk>> m_chunks;
};

// ── VoxelChiselSystem — edit API + statistics ─────────────────────────────
class VoxelChiselSystem {
public:
    explicit VoxelChiselSystem(VoxelVolume* volume = nullptr) : m_volume(volume) {}

    void SetVolume(VoxelVolume* v) { m_volume = v; }
    VoxelVolume* GetVolume()       { return m_volume; }

    // ── Chisel operations (sub-voxel world coords) ──────────────────────
    void SetCell(int32_t x, int32_t y, int32_t z, VoxelMaterial mat) {
        if (!m_volume) return;
        m_volume->SetCell(x, y, z, mat);
        ++m_stats.cellsModified;
        PushOp({VoxelChiselOp::Type::SetCell, {}, x, y, z, 0,0,0, 0,0,0, mat, 255});
    }

    void FillBox(int32_t x0, int32_t y0, int32_t z0,
                 int32_t x1, int32_t y1, int32_t z1,
                 VoxelMaterial mat)
    {
        if (!m_volume) return;
        m_volume->FillBox(x0, y0, z0, x1, y1, z1, mat);
        m_stats.cellsModified += (uint64_t)(x1-x0+1)*(y1-y0+1)*(z1-z0+1);
        PushOp({VoxelChiselOp::Type::FillBox, {}, x0, y0, z0,
                (uint8_t)0,(uint8_t)0,(uint8_t)0,
                (uint8_t)0,(uint8_t)0,(uint8_t)0, mat, 255});
    }

    void CarveBox(int32_t x0, int32_t y0, int32_t z0,
                  int32_t x1, int32_t y1, int32_t z1)
    {
        FillBox(x0, y0, z0, x1, y1, z1, VoxelMaterial::Air);
    }

    // ── Statistics ────────────────────────────────────────────────────────
    struct Stats {
        uint64_t cellsModified = 0;
        uint32_t opsApplied    = 0;
        uint32_t chunksTouched = 0;
    };
    const Stats& GetStats() const { return m_stats; }
    void         ResetStats()     { m_stats = {}; }

    // ── Op history (basic undo support) ──────────────────────────────────
    const std::vector<VoxelChiselOp>& OpHistory() const { return m_history; }

private:
    VoxelVolume*              m_volume = nullptr;
    Stats                     m_stats;
    std::vector<VoxelChiselOp> m_history;

    void PushOp(VoxelChiselOp op) {
        m_history.push_back(op);
        ++m_stats.opsApplied;
        // Keep last 1000 ops
        if (m_history.size() > 1000) m_history.erase(m_history.begin());
    }
};

} // namespace PCG
