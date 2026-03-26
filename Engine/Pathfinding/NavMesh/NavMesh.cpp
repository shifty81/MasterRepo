#include "Engine/Pathfinding/NavMesh/NavMesh.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

namespace Engine {

// ── Simple 3D distance helper ─────────────────────────────────────────────────

static float Dist3(const float* a, const float* b) {
    float dx=a[0]-b[0], dy=a[1]-b[1], dz=a[2]-b[2];
    return std::sqrt(dx*dx+dy*dy+dz*dz);
}

static void Centroid(const NavPoly& p, float out[3]) {
    out[0]=out[1]=out[2]=0.f;
    size_t n = p.verts.size() / 3;
    if (!n) return;
    for (size_t i=0;i<n;++i) { out[0]+=p.verts[i*3]; out[1]+=p.verts[i*3+1]; out[2]+=p.verts[i*3+2]; }
    out[0]/=n; out[1]/=n; out[2]/=n;
}

// ── A* node ───────────────────────────────────────────────────────────────────

struct AStarNode {
    uint32_t polyId;
    float    g{0.f}, f{0.f};
    bool operator>(const AStarNode& o) const { return f > o.f; }
};

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct NavMesh::Impl {
    NavMeshBuildConfig                     config;
    std::unordered_map<uint32_t, NavPoly>  polys;
    uint32_t                               nextPolyId{1};
    bool                                   built{false};

    std::function<void(const NavPath&)>    onPath;

    // Build stub: voxelise triangle soup → create one poly per triangle
    void BuildFromTrianglesImpl(const float* verts, uint32_t vc,
                                const uint32_t* idx, uint32_t ic) {
        polys.clear();
        nextPolyId = 1;
        for (uint32_t i = 0; i + 2 < ic; i += 3) {
            NavPoly p;
            p.id = nextPolyId++;
            for (int k = 0; k < 3; ++k) {
                uint32_t vi = idx[i+k] * 3;
                if (vi + 2 < vc * 3) {
                    p.verts.push_back(verts[vi]);
                    p.verts.push_back(verts[vi+1]);
                    p.verts.push_back(verts[vi+2]);
                }
            }
            polys[p.id] = std::move(p);
        }
        // Build adjacency: polys sharing an edge are neighbours
        // (full impl; here we link sequential polys as a simple chain stub)
        for (auto& [id, poly] : polys) {
            if (id > 1) poly.neighbours.push_back(id - 1);
            if (polys.count(id + 1)) poly.neighbours.push_back(id + 1);
        }
        built = !polys.empty();
    }

    NavPath AStarSearch(const float start[3], const float end[3]) const {
        NavPath result;
        if (polys.empty()) { result.errorMessage = "NavMesh not built"; return result; }

        // Find start / end polys
        uint32_t startPoly = 0, endPoly = 0;
        float bestS = 1e30f, bestE = 1e30f;
        for (auto& [id, p] : polys) {
            if (p.blocked) continue;
            float c[3]; Centroid(p, c);
            float ds = Dist3(c, start), de = Dist3(c, end);
            if (ds < bestS) { bestS = ds; startPoly = id; }
            if (de < bestE) { bestE = de; endPoly   = id; }
        }
        if (!startPoly || !endPoly) {
            result.errorMessage = "No reachable polygon found";
            return result;
        }
        if (startPoly == endPoly) {
            result.succeeded = true;
            for (int i=0;i<3;++i) result.waypoints.push_back(start[i]);
            for (int i=0;i<3;++i) result.waypoints.push_back(end[i]);
            result.nodeCount = 2;
            result.totalLength = Dist3(start, end);
            return result;
        }

        // A* over polygon graph
        std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open;
        std::unordered_map<uint32_t, uint32_t>  cameFrom;
        std::unordered_map<uint32_t, float>     gScore;

        gScore[startPoly] = 0.f;
        float ec[3]; Centroid(polys.at(endPoly), ec);
        float sc[3]; Centroid(polys.at(startPoly), sc);
        open.push({startPoly, 0.f, Dist3(sc, ec)});

        while (!open.empty()) {
            AStarNode cur = open.top(); open.pop();
            if (cur.polyId == endPoly) {
                // Reconstruct path
                std::vector<uint32_t> path;
                uint32_t p = endPoly;
                while (p != startPoly) {
                    path.push_back(p);
                    auto it = cameFrom.find(p);
                    if (it == cameFrom.end()) break;
                    p = it->second;
                }
                path.push_back(startPoly);
                std::reverse(path.begin(), path.end());

                for (int i=0;i<3;++i) result.waypoints.push_back(start[i]);
                for (uint32_t pid : path) {
                    float cc[3]; Centroid(polys.at(pid), cc);
                    for (int i=0;i<3;++i) result.waypoints.push_back(cc[i]);
                }
                for (int i=0;i<3;++i) result.waypoints.push_back(end[i]);
                result.nodeCount    = static_cast<uint32_t>(result.waypoints.size() / 3);
                result.succeeded    = true;
                // Compute length
                for (uint32_t i = 0; i + 1 < result.nodeCount; ++i)
                    result.totalLength += Dist3(&result.waypoints[i*3],
                                                &result.waypoints[(i+1)*3]);
                return result;
            }
            auto it2 = polys.find(cur.polyId);
            if (it2 == polys.end()) continue;
            for (uint32_t nid : it2->second.neighbours) {
                auto nit = polys.find(nid);
                if (nit == polys.end() || nit->second.blocked) continue;
                float nc[3]; Centroid(nit->second, nc);
                float cc2[3]; Centroid(it2->second, cc2);
                float ng = gScore[cur.polyId] + Dist3(cc2, nc) * nit->second.areaCost;
                if (!gScore.count(nid) || ng < gScore[nid]) {
                    gScore[nid]    = ng;
                    cameFrom[nid]  = cur.polyId;
                    open.push({nid, ng, ng + Dist3(nc, ec)});
                }
            }
        }
        result.errorMessage = "No path found";
        return result;
    }
};

NavMesh::NavMesh() : m_impl(new Impl()) {}
NavMesh::~NavMesh() { delete m_impl; }

void NavMesh::Init(const NavMeshBuildConfig& cfg) { m_impl->config = cfg; }
void NavMesh::Shutdown() { m_impl->polys.clear(); m_impl->built = false; }

bool NavMesh::BuildFromTriangles(const float* v, uint32_t vc,
                                 const uint32_t* idx, uint32_t ic) {
    m_impl->BuildFromTrianglesImpl(v, vc, idx, ic);
    return m_impl->built;
}

bool NavMesh::LoadFromFile(const std::string& path) {
    std::ifstream f(path); return f.is_open(); // full impl: parse binary/JSON
}

bool NavMesh::SaveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << "{\"polyCount\":" << m_impl->polys.size() << "}\n";
    return true;
}

bool     NavMesh::IsBuilt()    const { return m_impl->built; }
uint32_t NavMesh::PolyCount()  const { return static_cast<uint32_t>(m_impl->polys.size()); }

NavPath NavMesh::FindPath(const float start[3], const float end[3]) const {
    NavPath p = m_impl->AStarSearch(start, end);
    if (m_impl->onPath) m_impl->onPath(p);
    return p;
}

NavPath NavMesh::SmoothPath(const NavPath& raw) const {
    // String-pull stub: return the raw path unchanged
    NavPath out = raw;
    return out;
}

void NavMesh::BlockArea(const float centre[3], float radius) {
    for (auto& [id, p] : m_impl->polys) {
        float c[3]; Centroid(p, c);
        if (Dist3(c, centre) <= radius) p.blocked = true;
    }
}

void NavMesh::UnblockArea(const float centre[3], float radius) {
    for (auto& [id, p] : m_impl->polys) {
        float c[3]; Centroid(p, c);
        if (Dist3(c, centre) <= radius) p.blocked = false;
    }
}

void NavMesh::SetPolyBlocked(uint32_t id, bool blocked) {
    auto it = m_impl->polys.find(id);
    if (it != m_impl->polys.end()) it->second.blocked = blocked;
}

void NavMesh::SetPolyAreaCost(uint32_t id, float cost) {
    auto it = m_impl->polys.find(id);
    if (it != m_impl->polys.end()) it->second.areaCost = cost;
}

bool NavMesh::FindNearestPoint(const float pos[3], float result[3]) const {
    if (m_impl->polys.empty()) return false;
    float best = 1e30f; uint32_t bestId = 0;
    for (auto& [id, p] : m_impl->polys) {
        float c[3]; Centroid(p, c);
        float d = Dist3(pos, c);
        if (d < best) { best = d; bestId = id; }
    }
    if (!bestId) return false;
    float c[3]; Centroid(m_impl->polys.at(bestId), c);
    result[0]=c[0]; result[1]=c[1]; result[2]=c[2];
    return true;
}

bool NavMesh::IsPointOnMesh(const float pos[3]) const {
    for (auto& [id, p] : m_impl->polys) {
        if (p.blocked) continue;
        float c[3]; Centroid(p, c);
        if (Dist3(pos, c) < m_impl->config.agentRadius * 2.f) return true;
    }
    return false;
}

NavMeshBuildConfig NavMesh::GetConfig() const { return m_impl->config; }

void NavMesh::OnPathFound(std::function<void(const NavPath&)> cb) {
    m_impl->onPath = std::move(cb);
}

} // namespace Engine
