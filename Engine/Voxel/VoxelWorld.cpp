#include "Engine/Voxel/VoxelWorld.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <tuple>

namespace Engine {

// ── VoxelChunk ────────────────────────────────────────────────────────────
VoxelType VoxelChunk::Get(uint32_t x, uint32_t y, uint32_t z) const {
    if (x>=kVoxelChunkSize||y>=kVoxelChunkSize||z>=kVoxelChunkSize) return kAir;
    return voxels[x][y][z];
}
void VoxelChunk::Set(uint32_t x, uint32_t y, uint32_t z, VoxelType t) {
    if (x>=kVoxelChunkSize||y>=kVoxelChunkSize||z>=kVoxelChunkSize) return;
    voxels[x][y][z] = t;
    dirty = true;
}
bool VoxelChunk::IsAir(uint32_t x, uint32_t y, uint32_t z) const {
    return Get(x,y,z) == kAir;
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct VoxelWorld::Impl {
    std::unordered_map<VoxelChunkCoord, VoxelChunk, VoxelChunkCoordHash> chunks;
    std::unordered_map<VoxelChunkCoord, VoxelMesh,  VoxelChunkCoordHash> meshes;
    int32_t streamRadius{3};
    std::vector<MeshReadyCb> meshCbs;
};

VoxelWorld::VoxelWorld() : m_impl(new Impl()) {}
VoxelWorld::~VoxelWorld() { delete m_impl; }

void VoxelWorld::SetStreamRadius(int32_t r) { m_impl->streamRadius = r; }

VoxelChunkCoord VoxelWorld::WorldToChunk(int32_t wx, int32_t wy, int32_t wz) const {
    auto divFloor = [](int32_t a, int32_t b) -> int32_t {
        return a/b - (a%b != 0 && (a^b) < 0);
    };
    int32_t s = static_cast<int32_t>(kVoxelChunkSize);
    return {divFloor(wx,s), divFloor(wy,s), divFloor(wz,s)};
}

VoxelChunk* VoxelWorld::loadChunk(VoxelChunkCoord coord) {
    auto& c = m_impl->chunks[coord];
    c.coord = coord;
    c.dirty = true;
    return &c;
}

void VoxelWorld::unloadChunk(VoxelChunkCoord coord) {
    m_impl->chunks.erase(coord);
    m_impl->meshes.erase(coord);
}

void VoxelWorld::UpdateStreaming(float vx, float vy, float vz) {
    VoxelChunkCoord centre = WorldToChunk(
        static_cast<int32_t>(vx),
        static_cast<int32_t>(vy),
        static_cast<int32_t>(vz));
    int32_t r = m_impl->streamRadius;
    for (int32_t dz=-r; dz<=r; ++dz)
        for (int32_t dy=-r; dy<=r; ++dy)
            for (int32_t dx=-r; dx<=r; ++dx) {
                VoxelChunkCoord c{centre.cx+dx, centre.cy+dy, centre.cz+dz};
                if (!m_impl->chunks.count(c)) loadChunk(c);
            }
    std::vector<VoxelChunkCoord> toRemove;
    for (auto& [c,_] : m_impl->chunks) {
        if (std::abs(c.cx-centre.cx)>r+1 ||
            std::abs(c.cy-centre.cy)>r+1 ||
            std::abs(c.cz-centre.cz)>r+1)
            toRemove.push_back(c);
    }
    for (auto& c : toRemove) unloadChunk(c);
}

size_t VoxelWorld::LoadedChunkCount() const { return m_impl->chunks.size(); }

const VoxelChunk* VoxelWorld::GetChunk(VoxelChunkCoord c) const {
    auto it = m_impl->chunks.find(c);
    return it != m_impl->chunks.end() ? &it->second : nullptr;
}
VoxelChunk* VoxelWorld::GetChunk(VoxelChunkCoord c) {
    auto it = m_impl->chunks.find(c);
    return it != m_impl->chunks.end() ? &it->second : nullptr;
}

VoxelType VoxelWorld::GetVoxel(int32_t wx, int32_t wy, int32_t wz) const {
    auto cc = WorldToChunk(wx,wy,wz);
    const auto* chunk = GetChunk(cc);
    if (!chunk) return kAir;
    uint32_t lx = static_cast<uint32_t>((wx % static_cast<int32_t>(kVoxelChunkSize) + kVoxelChunkSize) % kVoxelChunkSize);
    uint32_t ly = static_cast<uint32_t>((wy % static_cast<int32_t>(kVoxelChunkSize) + kVoxelChunkSize) % kVoxelChunkSize);
    uint32_t lz = static_cast<uint32_t>((wz % static_cast<int32_t>(kVoxelChunkSize) + kVoxelChunkSize) % kVoxelChunkSize);
    return chunk->Get(lx,ly,lz);
}

void VoxelWorld::SetVoxel(int32_t wx, int32_t wy, int32_t wz, VoxelType t) {
    auto cc = WorldToChunk(wx,wy,wz);
    auto* chunk = GetChunk(cc);
    if (!chunk) chunk = loadChunk(cc);
    uint32_t lx = static_cast<uint32_t>((wx % static_cast<int32_t>(kVoxelChunkSize) + kVoxelChunkSize) % kVoxelChunkSize);
    uint32_t ly = static_cast<uint32_t>((wy % static_cast<int32_t>(kVoxelChunkSize) + kVoxelChunkSize) % kVoxelChunkSize);
    uint32_t lz = static_cast<uint32_t>((wz % static_cast<int32_t>(kVoxelChunkSize) + kVoxelChunkSize) % kVoxelChunkSize);
    chunk->Set(lx,ly,lz,t);
}

bool VoxelWorld::IsAir(int32_t wx, int32_t wy, int32_t wz) const {
    return GetVoxel(wx,wy,wz) == kAir;
}

// ── Greedy mesh ───────────────────────────────────────────────────────────
void VoxelWorld::buildMesh(VoxelChunk& chunk) {
    auto& mesh = m_impl->meshes[chunk.coord];
    mesh.vertices.clear();
    mesh.indices.clear();
    mesh.chunkCoord  = chunk.coord;
    int32_t ox = chunk.coord.cx * static_cast<int32_t>(kVoxelChunkSize);
    int32_t oy = chunk.coord.cy * static_cast<int32_t>(kVoxelChunkSize);
    int32_t oz = chunk.coord.cz * static_cast<int32_t>(kVoxelChunkSize);
    uint32_t N = kVoxelChunkSize;

    // Simple per-face emission (not fully greedy for clarity, correct geometry)
    const int32_t dirs[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    const float   norms[6][3]= {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};

    for (uint32_t x=0; x<N; ++x) for (uint32_t y=0; y<N; ++y) for (uint32_t z=0; z<N; ++z) {
        VoxelType vt = chunk.Get(x,y,z);
        if (vt == kAir) continue;
        for (int f=0; f<6; ++f) {
            int32_t nx2 = static_cast<int32_t>(x)+dirs[f][0];
            int32_t ny2 = static_cast<int32_t>(y)+dirs[f][1];
            int32_t nz2 = static_cast<int32_t>(z)+dirs[f][2];
            // Check neighbour
            bool exposed = false;
            if (nx2<0||ny2<0||nz2<0||nx2>=(int32_t)N||ny2>=(int32_t)N||nz2>=(int32_t)N) {
                exposed = IsAir(ox+(int32_t)x+dirs[f][0],
                                oy+(int32_t)y+dirs[f][1],
                                oz+(int32_t)z+dirs[f][2]);
            } else {
                exposed = chunk.IsAir(static_cast<uint32_t>(nx2),
                                       static_cast<uint32_t>(ny2),
                                       static_cast<uint32_t>(nz2));
            }
            if (!exposed) continue;
            // Emit a quad (2 triangles)
            float fx = static_cast<float>(ox + static_cast<int32_t>(x));
            float fy = static_cast<float>(oy + static_cast<int32_t>(y));
            float fz = static_cast<float>(oz + static_cast<int32_t>(z));
            Math::Vec3 n{norms[f][0],norms[f][1],norms[f][2]};
            // Build 4 quad vertices offset from (fx,fy,fz)
            static const float quads[6][4][3] = {
                {{1,0,0},{1,1,0},{1,1,1},{1,0,1}},
                {{0,1,0},{0,0,0},{0,0,1},{0,1,1}},
                {{0,1,0},{1,1,0},{1,1,1},{0,1,1}},
                {{0,0,0},{1,0,0},{1,0,1},{0,0,1}},
                {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},
                {{1,0,0},{0,0,0},{0,1,0},{1,1,0}},
            };
            uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
            for (int v=0;v<4;++v) {
                VoxelVertex vtx;
                vtx.pos = {fx+quads[f][v][0], fy+quads[f][v][1], fz+quads[f][v][2]};
                vtx.normal   = n;
                vtx.material = vt;
                vtx.u = (v==1||v==2)?1.0f:0.0f;
                vtx.v = (v==2||v==3)?1.0f:0.0f;
                mesh.vertices.push_back(vtx);
            }
            mesh.indices.insert(mesh.indices.end(),
                {base,base+1,base+2, base,base+2,base+3});
        }
    }
    mesh.upToDate = true;
    chunk.dirty   = false;
    for (auto& cb : m_impl->meshCbs) cb(mesh);
}

void VoxelWorld::RebuildDirtyMeshes() {
    for (auto& [coord, chunk] : m_impl->chunks)
        if (chunk.dirty) buildMesh(chunk);
}

const VoxelMesh* VoxelWorld::GetMesh(VoxelChunkCoord coord) const {
    auto it = m_impl->meshes.find(coord);
    return it != m_impl->meshes.end() && it->second.upToDate ? &it->second : nullptr;
}

void VoxelWorld::OnMeshReady(MeshReadyCb cb) { m_impl->meshCbs.push_back(std::move(cb)); }

// ── Ray cast ─────────────────────────────────────────────────────────────
VoxelHit VoxelWorld::RayCast(const Math::Vec3& origin,
                               const Math::Vec3& direction,
                               float maxDistance) const
{
    float len = std::sqrt(direction.x*direction.x+direction.y*direction.y+direction.z*direction.z);
    if (len < 1e-7f) return {};
    Math::Vec3 d{direction.x/len,direction.y/len,direction.z/len};
    // DDA voxel traversal
    int32_t vx = static_cast<int32_t>(std::floor(origin.x));
    int32_t vy = static_cast<int32_t>(std::floor(origin.y));
    int32_t vz = static_cast<int32_t>(std::floor(origin.z));
    int32_t sx = d.x > 0 ? 1 : -1, sy = d.y > 0 ? 1 : -1, sz = d.z > 0 ? 1 : -1;
    auto calcT = [](float o, float dir, int32_t v, int32_t s) -> float {
        if (std::abs(dir) < 1e-7f) return 1e30f;
        return (static_cast<float>(v + (s>0?1:0)) - o) / dir;
    };
    float tx = calcT(origin.x,d.x,vx,sx);
    float ty = calcT(origin.y,d.y,vy,sy);
    float tz = calcT(origin.z,d.z,vz,sz);
    float dtx = std::abs(sx / (std::abs(d.x) < 1e-7f ? 1e7f : d.x));
    float dty = std::abs(sy / (std::abs(d.y) < 1e-7f ? 1e7f : d.y));
    float dtz = std::abs(sz / (std::abs(d.z) < 1e-7f ? 1e7f : d.z));
    Math::Vec3 normal{};
    float dist = 0.0f;
    while (dist < maxDistance) {
        VoxelType vt = GetVoxel(vx,vy,vz);
        if (vt != kAir) {
            VoxelHit hit;
            hit.hit = true;
            hit.point = {origin.x+d.x*dist, origin.y+d.y*dist, origin.z+d.z*dist};
            hit.normal = normal;
            hit.vx = vx; hit.vy = vy; hit.vz = vz;
            hit.material = vt;
            hit.distance = dist;
            return hit;
        }
        if (tx < ty && tx < tz) {
            dist = tx; tx += dtx; vx += sx; normal = {static_cast<float>(-sx),0,0};
        } else if (ty < tz) {
            dist = ty; ty += dty; vy += sy; normal = {0,static_cast<float>(-sy),0};
        } else {
            dist = tz; tz += dtz; vz += sz; normal = {0,0,static_cast<float>(-sz)};
        }
    }
    return {};
}

// ── Flood fill ────────────────────────────────────────────────────────────
std::vector<std::tuple<int32_t,int32_t,int32_t>>
VoxelWorld::FloodFill(int32_t wx, int32_t wy, int32_t wz, uint32_t maxVoxels) const {
    if (!IsAir(wx,wy,wz)) return {};
    using T3 = std::tuple<int32_t,int32_t,int32_t>;
    std::vector<T3> result;
    std::queue<T3> q;
    std::unordered_map<int64_t,bool> visited;
    auto key = [](int32_t x,int32_t y,int32_t z) -> int64_t {
        return (static_cast<int64_t>(x+32768)<<40) |
               (static_cast<int64_t>(y+32768)<<20) |
               static_cast<int64_t>(z+32768);
    };
    q.push({wx,wy,wz});
    visited[key(wx,wy,wz)] = true;
    const int32_t offs[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    while (!q.empty() && result.size() < maxVoxels) {
        auto [x,y,z] = q.front(); q.pop();
        result.push_back({x,y,z});
        for (auto& o : offs) {
            int32_t nx=x+o[0],ny=y+o[1],nz=z+o[2];
            int64_t k = key(nx,ny,nz);
            if (!visited.count(k) && IsAir(nx,ny,nz)) {
                visited[k] = true;
                q.push({nx,ny,nz});
            }
        }
    }
    return result;
}

// ── Fill sphere ───────────────────────────────────────────────────────────
void VoxelWorld::FillSphere(int32_t cx, int32_t cy, int32_t cz, float radius, VoxelType t) {
    int32_t r = static_cast<int32_t>(std::ceil(radius));
    for (int32_t dx=-r; dx<=r; ++dx)
        for (int32_t dy=-r; dy<=r; ++dy)
            for (int32_t dz=-r; dz<=r; ++dz)
                if (dx*dx+dy*dy+dz*dz <= radius*radius)
                    SetVoxel(cx+dx,cy+dy,cz+dz,t);
}

} // namespace Engine
