#include "HullMeshGenerator.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <utility>

namespace atlas::procedural {

// ---- HullMesh helpers ----

HullVertex HullMesh::Centroid() const {
    HullVertex c{0.0f, 0.0f, 0.0f};
    if (vertices.empty()) return c;
    for (auto& v : vertices) { c.x += v.x; c.y += v.y; c.z += v.z; }
    float n = static_cast<float>(vertices.size());
    c.x /= n; c.y /= n; c.z /= n;
    return c;
}

// ---- Geometry utilities ----

namespace {

float Dot(const HullVertex& a, const HullVertex& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

HullVertex Sub(const HullVertex& a, const HullVertex& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

HullVertex Add(const HullVertex& a, const HullVertex& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

HullVertex Scale(const HullVertex& v, float s) {
    return {v.x * s, v.y * s, v.z * s};
}

HullVertex Cross(const HullVertex& a, const HullVertex& b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

float Length(const HullVertex& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

float Distance(const HullVertex& a, const HullVertex& b) {
    return Length(Sub(a, b));
}

HullVertex Normalize(const HullVertex& v) {
    float len = Length(v);
    if (len < 1e-12f) return {0.0f, 0.0f, 0.0f};
    return Scale(v, 1.0f / len);
}

// Normal of a triangle (outward-facing assuming CCW winding).
HullVertex TriNormal(const HullVertex& a, const HullVertex& b, const HullVertex& c) {
    return Cross(Sub(b, a), Sub(c, a));
}

using Edge = std::pair<uint32_t, uint32_t>;
Edge MakeEdge(uint32_t a, uint32_t b) {
    return (a < b) ? Edge{a, b} : Edge{b, a};
}

} // anonymous namespace

// ---- Convex hull (incremental, 3-D) ----

HullMesh HullMeshGenerator::GenerateConvexHull(const std::vector<HullVertex>& points) {
    HullMesh mesh;
    if (points.size() < 4) {
        // Degenerate: just store points and no triangles.
        mesh.vertices = points;
        if (points.size() == 3) {
            mesh.triangles.push_back({0, 1, 2});
        }
        return mesh;
    }

    // Copy points and build initial tetrahedron from first 4 non-coplanar.
    mesh.vertices = points;
    uint32_t n = static_cast<uint32_t>(points.size());

    // Find 4 non-coplanar points.
    uint32_t idx[4] = {0, 1, 2, 3};

    // Ensure first two are distinct.
    for (uint32_t i = 1; i < n; ++i) {
        if (Distance(points[0], points[i]) > 1e-6f) { idx[1] = i; break; }
    }

    // Third point not collinear with first two.
    for (uint32_t i = 1; i < n; ++i) {
        if (i == idx[1]) continue;
        HullVertex cr = Cross(Sub(points[idx[1]], points[idx[0]]),
                              Sub(points[i], points[idx[0]]));
        if (Length(cr) > 1e-6f) { idx[2] = i; break; }
    }

    // Fourth point not coplanar with first three.
    HullVertex triN = TriNormal(points[idx[0]], points[idx[1]], points[idx[2]]);
    for (uint32_t i = 1; i < n; ++i) {
        if (i == idx[1] || i == idx[2]) continue;
        float d = Dot(triN, Sub(points[i], points[idx[0]]));
        if (std::fabs(d) > 1e-6f) { idx[3] = i; break; }
    }

    // Orient initial tetrahedron so normals point outward.
    {
        HullVertex centroid = Scale(Add(Add(points[idx[0]], points[idx[1]]),
                                        Add(points[idx[2]], points[idx[3]])),
                                    0.25f);
        auto orient = [&](uint32_t a, uint32_t b, uint32_t c) {
            HullVertex normal = TriNormal(points[a], points[b], points[c]);
            HullVertex toCenter = Sub(centroid, points[a]);
            if (Dot(normal, toCenter) > 0.0f) std::swap(b, c);
            mesh.triangles.push_back({a, b, c});
        };
        orient(idx[0], idx[1], idx[2]);
        orient(idx[0], idx[1], idx[3]);
        orient(idx[0], idx[2], idx[3]);
        orient(idx[1], idx[2], idx[3]);
    }

    // Incrementally add remaining points.
    for (uint32_t i = 0; i < n; ++i) {
        if (i == idx[0] || i == idx[1] || i == idx[2] || i == idx[3]) continue;
        const HullVertex& p = points[i];

        // Find visible faces.
        std::vector<bool> visible(mesh.triangles.size(), false);
        bool anyVisible = false;
        for (size_t f = 0; f < mesh.triangles.size(); ++f) {
            auto& tri = mesh.triangles[f];
            HullVertex normal = TriNormal(mesh.vertices[tri.v0],
                                          mesh.vertices[tri.v1],
                                          mesh.vertices[tri.v2]);
            float d = Dot(normal, Sub(p, mesh.vertices[tri.v0]));
            if (d > 1e-8f) { visible[f] = true; anyVisible = true; }
        }
        if (!anyVisible) continue;

        // Collect horizon edges (edges shared by exactly one visible face).
        std::map<Edge, int> edgeCount;
        std::map<Edge, std::pair<uint32_t, uint32_t>> edgeVerts;
        for (size_t f = 0; f < mesh.triangles.size(); ++f) {
            if (!visible[f]) continue;
            auto& tri = mesh.triangles[f];
            uint32_t vs[3] = {tri.v0, tri.v1, tri.v2};
            for (int e = 0; e < 3; ++e) {
                uint32_t a = vs[e], b = vs[(e + 1) % 3];
                Edge edge = MakeEdge(a, b);
                edgeCount[edge]++;
                edgeVerts[edge] = {a, b};
            }
        }

        // Remove visible faces.
        std::vector<HullTriangle> newTris;
        newTris.reserve(mesh.triangles.size());
        for (size_t f = 0; f < mesh.triangles.size(); ++f) {
            if (!visible[f]) newTris.push_back(mesh.triangles[f]);
        }

        // Create new faces from horizon edges to the new point.
        for (auto& [edge, count] : edgeCount) {
            if (count != 1) continue;
            auto [a, b] = edgeVerts[edge];
            // Orient so normal points outward.
            HullVertex normal = TriNormal(points[a], points[b], p);
            HullVertex mid = Scale(Add(points[a], points[b]), 0.5f);
            HullVertex centroid = mesh.Centroid();
            if (Dot(normal, Sub(mid, centroid)) < 0.0f)
                newTris.push_back({b, a, i});
            else
                newTris.push_back({a, b, i});
        }

        mesh.triangles = std::move(newTris);
    }

    return mesh;
}

// ---- Gap filling ----

void HullMeshGenerator::FillGaps(HullMesh& mesh) {
    float threshold = m_config.gapFillThreshold;
    if (threshold <= 0.0f) return;

    // Split edges that exceed the threshold by inserting midpoints.
    // Single pass to avoid infinite expansion.
    std::map<Edge, uint32_t> midpointMap;
    std::vector<HullTriangle> newTriangles;

    for (auto& tri : mesh.triangles) {
        uint32_t vs[3] = {tri.v0, tri.v1, tri.v2};
        bool split[3] = {false, false, false};
        uint32_t mids[3] = {0, 0, 0};

        for (int e = 0; e < 3; ++e) {
            uint32_t a = vs[e], b = vs[(e + 1) % 3];
            float d = Distance(mesh.vertices[a], mesh.vertices[b]);
            if (d > threshold) {
                Edge edge = MakeEdge(a, b);
                auto it = midpointMap.find(edge);
                if (it == midpointMap.end()) {
                    HullVertex mid;
                    mid.x = (mesh.vertices[a].x + mesh.vertices[b].x) * 0.5f;
                    mid.y = (mesh.vertices[a].y + mesh.vertices[b].y) * 0.5f;
                    mid.z = (mesh.vertices[a].z + mesh.vertices[b].z) * 0.5f;
                    uint32_t midIdx = static_cast<uint32_t>(mesh.vertices.size());
                    mesh.vertices.push_back(mid);
                    midpointMap[edge] = midIdx;
                    mids[e] = midIdx;
                } else {
                    mids[e] = it->second;
                }
                split[e] = true;
            }
        }

        int splitCount = (split[0] ? 1 : 0) + (split[1] ? 1 : 0) + (split[2] ? 1 : 0);
        if (splitCount == 0) {
            newTriangles.push_back(tri);
        } else if (splitCount == 3) {
            // All edges split → 4 sub-triangles
            newTriangles.push_back({vs[0], mids[0], mids[2]});
            newTriangles.push_back({mids[0], vs[1], mids[1]});
            newTriangles.push_back({mids[2], mids[1], vs[2]});
            newTriangles.push_back({mids[0], mids[1], mids[2]});
        } else {
            // Partial split — keep original for simplicity.
            newTriangles.push_back(tri);
            for (int e = 0; e < 3; ++e) {
                if (split[e]) {
                    uint32_t a = vs[e], b = vs[(e + 1) % 3], c = vs[(e + 2) % 3];
                    newTriangles.push_back({a, mids[e], c});
                    newTriangles.push_back({mids[e], b, c});
                    // Remove the original (already added) — replace last original.
                    newTriangles.erase(newTriangles.end() - 3);
                    break;  // only handle first split edge for partial
                }
            }
        }
    }

    mesh.triangles = std::move(newTriangles);
}

// ---- Stretch wrap ----

void HullMeshGenerator::ApplyStretchWrap(HullMesh& mesh,
                                          const std::vector<HullVertex>& interiorPoints) {
    if (interiorPoints.empty() || mesh.vertices.empty()) return;
    float factor = m_config.stretchWrapFactor;
    if (factor <= 0.0f) return;

    for (auto& vert : mesh.vertices) {
        // Find the closest interior point.
        float bestDist = Distance(vert, interiorPoints[0]);
        const HullVertex* closest = &interiorPoints[0];
        for (size_t i = 1; i < interiorPoints.size(); ++i) {
            float d = Distance(vert, interiorPoints[i]);
            if (d < bestDist) { bestDist = d; closest = &interiorPoints[i]; }
        }
        // Blend toward the closest interior point.
        vert.x += (closest->x - vert.x) * factor;
        vert.y += (closest->y - vert.y) * factor;
        vert.z += (closest->z - vert.z) * factor;
    }
}

// ---- Loop subdivision ----

void HullMeshGenerator::Subdivide(HullMesh& mesh) {
    if (mesh.triangles.empty()) return;

    std::map<Edge, uint32_t> midpoints;
    std::vector<HullTriangle> newTris;
    newTris.reserve(mesh.triangles.size() * 4);

    auto getOrCreateMid = [&](uint32_t a, uint32_t b) -> uint32_t {
        Edge e = MakeEdge(a, b);
        auto it = midpoints.find(e);
        if (it != midpoints.end()) return it->second;
        HullVertex mid;
        mid.x = (mesh.vertices[a].x + mesh.vertices[b].x) * 0.5f;
        mid.y = (mesh.vertices[a].y + mesh.vertices[b].y) * 0.5f;
        mid.z = (mesh.vertices[a].z + mesh.vertices[b].z) * 0.5f;
        uint32_t idx = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back(mid);
        midpoints[e] = idx;
        return idx;
    };

    for (auto& tri : mesh.triangles) {
        uint32_t m01 = getOrCreateMid(tri.v0, tri.v1);
        uint32_t m12 = getOrCreateMid(tri.v1, tri.v2);
        uint32_t m20 = getOrCreateMid(tri.v2, tri.v0);
        newTris.push_back({tri.v0, m01, m20});
        newTris.push_back({m01, tri.v1, m12});
        newTris.push_back({m20, m12, tri.v2});
        newTris.push_back({m01, m12, m20});
    }

    mesh.triangles = std::move(newTris);
}

// ---- Laplacian smoothing ----

void HullMeshGenerator::Smooth(HullMesh& mesh) {
    if (mesh.vertices.empty()) return;

    uint32_t vertCount = static_cast<uint32_t>(mesh.vertices.size());

    // Build adjacency.
    std::vector<std::set<uint32_t>> adj(vertCount);
    for (auto& tri : mesh.triangles) {
        adj[tri.v0].insert(tri.v1); adj[tri.v0].insert(tri.v2);
        adj[tri.v1].insert(tri.v0); adj[tri.v1].insert(tri.v2);
        adj[tri.v2].insert(tri.v0); adj[tri.v2].insert(tri.v1);
    }

    std::vector<HullVertex> smoothed(vertCount);
    for (uint32_t i = 0; i < vertCount; ++i) {
        if (adj[i].empty()) {
            smoothed[i] = mesh.vertices[i];
            continue;
        }
        HullVertex avg{0.0f, 0.0f, 0.0f};
        for (uint32_t nb : adj[i]) {
            avg.x += mesh.vertices[nb].x;
            avg.y += mesh.vertices[nb].y;
            avg.z += mesh.vertices[nb].z;
        }
        float count = static_cast<float>(adj[i].size());
        smoothed[i] = {avg.x / count, avg.y / count, avg.z / count};
    }

    mesh.vertices = std::move(smoothed);
}

// ---- Full pipeline ----

HullMesh HullMeshGenerator::GenerateFromInterior(const std::vector<HullVertex>& modulePositions) {
    HullMesh mesh = GenerateConvexHull(modulePositions);
    FillGaps(mesh);
    ApplyStretchWrap(mesh, modulePositions);

    for (uint32_t i = 0; i < m_config.subdivisionLevel; ++i) {
        Subdivide(mesh);
    }
    for (uint32_t i = 0; i < m_config.smoothIterations; ++i) {
        Smooth(mesh);
    }

    m_lastMesh = mesh;
    return mesh;
}

} // namespace atlas::procedural
