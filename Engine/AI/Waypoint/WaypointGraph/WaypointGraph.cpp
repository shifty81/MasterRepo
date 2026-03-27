#include "Engine/AI/Waypoint/WaypointGraph/WaypointGraph.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_map>

namespace Engine {

struct WPNode {
    float x{0}, y{0}, z{0};
    bool  enabled{true};
    std::vector<uint32_t> neighbours;
    std::unordered_map<uint32_t,float> costs;
};

struct WaypointGraph::Impl {
    std::unordered_map<uint32_t,WPNode> nodes;
};

WaypointGraph::WaypointGraph()  { m_impl = new Impl; }
WaypointGraph::~WaypointGraph() { delete m_impl; }

void WaypointGraph::Init    () {}
void WaypointGraph::Shutdown() { Reset(); }
void WaypointGraph::Reset   () { m_impl->nodes.clear(); }

bool WaypointGraph::AddWaypoint(uint32_t id, float x, float y, float z) {
    if (m_impl->nodes.count(id)) return false;
    WPNode n; n.x = x; n.y = y; n.z = z;
    m_impl->nodes[id] = n;
    return true;
}
void WaypointGraph::RemoveWaypoint(uint32_t id) {
    m_impl->nodes.erase(id);
    for (auto& [nid, n] : m_impl->nodes) {
        n.neighbours.erase(std::remove(n.neighbours.begin(), n.neighbours.end(), id), n.neighbours.end());
        n.costs.erase(id);
    }
}
bool WaypointGraph::ConnectWaypoints(uint32_t a, uint32_t b, bool bi) {
    auto ia = m_impl->nodes.find(a), ib = m_impl->nodes.find(b);
    if (ia == m_impl->nodes.end() || ib == m_impl->nodes.end()) return false;
    auto addEdge = [&](WPNode& from, uint32_t to) {
        if (std::find(from.neighbours.begin(), from.neighbours.end(), to) == from.neighbours.end())
            from.neighbours.push_back(to);
        if (!from.costs.count(to)) {
            auto& na = m_impl->nodes[a]; auto& nb = m_impl->nodes[b];
            float dx = na.x-nb.x, dy = na.y-nb.y, dz = na.z-nb.z;
            from.costs[to] = std::sqrt(dx*dx+dy*dy+dz*dz);
        }
    };
    addEdge(ia->second, b);
    if (bi) addEdge(ib->second, a);
    return true;
}
void WaypointGraph::DisconnectWaypoints(uint32_t a, uint32_t b) {
    auto remove = [&](uint32_t from, uint32_t to) {
        auto it = m_impl->nodes.find(from);
        if (it == m_impl->nodes.end()) return;
        auto& n = it->second;
        n.neighbours.erase(std::remove(n.neighbours.begin(), n.neighbours.end(), to), n.neighbours.end());
        n.costs.erase(to);
    };
    remove(a,b); remove(b,a);
}
void WaypointGraph::SetEdgeCost(uint32_t a, uint32_t b, float cost) {
    auto it = m_impl->nodes.find(a);
    if (it != m_impl->nodes.end()) it->second.costs[b] = cost;
}
float WaypointGraph::GetEdgeCost(uint32_t a, uint32_t b) const {
    auto it = m_impl->nodes.find(a);
    if (it == m_impl->nodes.end()) return 0;
    auto it2 = it->second.costs.find(b);
    return it2 != it->second.costs.end() ? it2->second : 0;
}
void WaypointGraph::SetEnabled(uint32_t id, bool on) {
    auto it = m_impl->nodes.find(id); if (it != m_impl->nodes.end()) it->second.enabled = on;
}

static float dist3(const WPNode& a, const WPNode& b) {
    float dx = a.x-b.x, dy = a.y-b.y, dz = a.z-b.z;
    return std::sqrt(dx*dx+dy*dy+dz*dz);
}

bool WaypointGraph::FindPath(uint32_t start, uint32_t goal, std::vector<uint32_t>& out) const {
    if (!m_impl->nodes.count(start) || !m_impl->nodes.count(goal)) return false;
    using P = std::pair<float,uint32_t>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
    std::unordered_map<uint32_t,float> dist;
    std::unordered_map<uint32_t,uint32_t> prev;
    for (auto& [id,_] : m_impl->nodes) dist[id] = std::numeric_limits<float>::infinity();
    dist[start] = 0; pq.push({0, start});
    while (!pq.empty()) {
        auto [d, cur] = pq.top(); pq.pop();
        if (cur == goal) break;
        if (d > dist[cur]) continue;
        auto& n = m_impl->nodes.at(cur);
        for (uint32_t nb : n.neighbours) {
            if (!m_impl->nodes.at(nb).enabled) continue;
            float cost = n.costs.count(nb) ? n.costs.at(nb) : dist3(n, m_impl->nodes.at(nb));
            float nd = dist[cur] + cost;
            if (nd < dist[nb]) { dist[nb] = nd; prev[nb] = cur; pq.push({nd, nb}); }
        }
    }
    if (dist[goal] == std::numeric_limits<float>::infinity()) return false;
    for (uint32_t n = goal; n != start; n = prev.at(n)) out.push_back(n);
    out.push_back(start);
    std::reverse(out.begin(), out.end());
    return true;
}
bool WaypointGraph::FindPathAStar(uint32_t start, uint32_t goal, std::vector<uint32_t>& out) const {
    if (!m_impl->nodes.count(start) || !m_impl->nodes.count(goal)) return false;
    const WPNode& gn = m_impl->nodes.at(goal);
    using P = std::pair<float,uint32_t>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> open;
    std::unordered_map<uint32_t,float> g;
    std::unordered_map<uint32_t,uint32_t> prev;
    for (auto& [id,_] : m_impl->nodes) g[id] = std::numeric_limits<float>::infinity();
    g[start] = 0; open.push({dist3(m_impl->nodes.at(start), gn), start});
    while (!open.empty()) {
        auto [_, cur] = open.top(); open.pop();
        if (cur == goal) break;
        auto& n = m_impl->nodes.at(cur);
        for (uint32_t nb : n.neighbours) {
            if (!m_impl->nodes.at(nb).enabled) continue;
            float cost = n.costs.count(nb) ? n.costs.at(nb) : dist3(n, m_impl->nodes.at(nb));
            float ng = g[cur] + cost;
            if (ng < g[nb]) {
                g[nb] = ng; prev[nb] = cur;
                open.push({ng + dist3(m_impl->nodes.at(nb), gn), nb});
            }
        }
    }
    if (g[goal] == std::numeric_limits<float>::infinity()) return false;
    for (uint32_t n = goal; n != start; n = prev.at(n)) out.push_back(n);
    out.push_back(start);
    std::reverse(out.begin(), out.end());
    return true;
}
uint32_t WaypointGraph::GetNeighbours(uint32_t id, std::vector<uint32_t>& out) const {
    auto it = m_impl->nodes.find(id);
    if (it == m_impl->nodes.end()) return 0;
    out = it->second.neighbours;
    return (uint32_t)out.size();
}
uint32_t WaypointGraph::GetNearestWaypoint(float x, float y, float z) const {
    uint32_t best = 0; float bestD = std::numeric_limits<float>::infinity();
    for (auto& [id, n] : m_impl->nodes) {
        float dx = n.x-x, dy = n.y-y, dz = n.z-z;
        float d = dx*dx+dy*dy+dz*dz;
        if (d < bestD) { bestD = d; best = id; }
    }
    return best;
}
uint32_t WaypointGraph::GetWaypointCount() const { return (uint32_t)m_impl->nodes.size(); }
void WaypointGraph::GetWaypointPosition(uint32_t id, float& x, float& y, float& z) const {
    auto it = m_impl->nodes.find(id);
    if (it != m_impl->nodes.end()) { x = it->second.x; y = it->second.y; z = it->second.z; }
    else { x = y = z = 0; }
}

} // namespace Engine
