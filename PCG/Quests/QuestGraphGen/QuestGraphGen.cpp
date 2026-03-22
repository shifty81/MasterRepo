#include "PCG/Quests/QuestGraphGen/QuestGraphGen.h"
#include <algorithm>
#include <queue>
#include <random>
#include <sstream>
#include <unordered_set>

namespace PCG {

const char* QuestNodeTypeName(QuestNodeType t) {
    switch (t) {
    case QuestNodeType::Kill:    return "Kill";
    case QuestNodeType::Collect: return "Collect";
    case QuestNodeType::Escort:  return "Escort";
    case QuestNodeType::Explore: return "Explore";
    case QuestNodeType::Deliver: return "Deliver";
    case QuestNodeType::Talk:    return "Talk";
    case QuestNodeType::Craft:   return "Craft";
    case QuestNodeType::Survive: return "Survive";
    }
    return "Unknown";
}

// ── QuestGraph helpers ────────────────────────────────────────────────────
bool QuestGraph::IsValid() const {
    if (nodes.empty()) return false;
    // BFS from entry nodes; check all terminals reachable
    std::unordered_set<uint32_t> reachable;
    std::queue<uint32_t> q;
    for (auto id : entryIds) { q.push(id); reachable.insert(id); }
    while (!q.empty()) {
        uint32_t cur = q.front(); q.pop();
        for (const auto& e : edges) {
            if (e.fromId == cur && !reachable.count(e.toId)) {
                reachable.insert(e.toId);
                q.push(e.toId);
            }
        }
    }
    for (auto id : terminalIds)
        if (!reachable.count(id)) return false;
    return true;
}

std::vector<uint32_t> QuestGraph::DependenciesOf(uint32_t nodeId) const {
    std::vector<uint32_t> deps;
    for (const auto& e : edges) if (e.toId == nodeId) deps.push_back(e.fromId);
    return deps;
}

std::vector<uint32_t> QuestGraph::DependentsOf(uint32_t nodeId) const {
    std::vector<uint32_t> deps;
    for (const auto& e : edges) if (e.fromId == nodeId) deps.push_back(e.toId);
    return deps;
}

std::string QuestGraph::Serialise() const {
    std::ostringstream ss;
    ss << "{\"id\":" << id << ",\"title\":\"" << title << "\",\"nodes\":[";
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& n = nodes[i];
        ss << "{\"id\":" << n.id
           << ",\"type\":\"" << QuestNodeTypeName(n.type) << "\""
           << ",\"obj\":\"" << n.objective << "\""
           << ",\"xp\":" << n.reward.xp
           << ",\"entry\":" << (n.isEntry ? "true" : "false")
           << ",\"terminal\":" << (n.isTerminal ? "true" : "false")
           << "}";
        if (i + 1 < nodes.size()) ss << ",";
    }
    ss << "],\"edges\":[";
    for (size_t i = 0; i < edges.size(); ++i) {
        ss << "{\"from\":" << edges[i].fromId << ",\"to\":" << edges[i].toId << "}";
        if (i + 1 < edges.size()) ss << ",";
    }
    ss << "]}";
    return ss.str();
}

// ── Node factory helpers ──────────────────────────────────────────────────
static const QuestNodeType kNodeTypes[] = {
    QuestNodeType::Kill, QuestNodeType::Collect, QuestNodeType::Explore,
    QuestNodeType::Talk, QuestNodeType::Deliver, QuestNodeType::Craft,
    QuestNodeType::Escort, QuestNodeType::Survive
};
static const char* kLocations[] = {
    "dungeon_entrance", "marketplace", "ancient_ruins", "forest_edge",
    "mountain_pass", "abandoned_mine", "harbour", "throne_room"
};
static const char* kTargets[] = {
    "goblin", "iron_ore", "wolf_pelt", "ancient_scroll",
    "healing_herb", "stolen_gem", "bandit_captain", "rare_flower"
};

static QuestNode makeNode(uint32_t id, std::mt19937_64& rng, bool entry, bool terminal) {
    QuestNode n;
    n.id         = id;
    n.type       = kNodeTypes[rng() % 8];
    n.objective  = std::string("Complete ") + QuestNodeTypeName(n.type) +
                   " objective " + std::to_string(id);
    n.locationTag = kLocations[rng() % 8];
    n.targetId   = kTargets[rng() % 8];
    n.quantity   = static_cast<uint32_t>(1 + rng() % 5);
    n.reward.xp  = static_cast<uint32_t>(50 + rng() % 200);
    n.reward.gold = static_cast<uint32_t>(10 + rng() % 100);
    n.isEntry    = entry;
    n.isTerminal = terminal;
    return n;
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct QuestGraphGen::Impl {
    QuestGraphConfig cfg;
    std::vector<QuestGraphGen::GeneratedCb> callbacks;
    uint32_t nextGraphId{1};

    void fire(const QuestGraph& g) {
        for (auto& cb : callbacks) cb(g);
    }
};

QuestGraphGen::QuestGraphGen() : m_impl(new Impl()) {}
QuestGraphGen::~QuestGraphGen() { delete m_impl; }

void QuestGraphGen::SetConfig(const QuestGraphConfig& cfg) { m_impl->cfg = cfg; }
const QuestGraphConfig& QuestGraphGen::Config() const { return m_impl->cfg; }

QuestGraph QuestGraphGen::GenerateLinear(uint32_t steps) {
    std::mt19937_64 rng(m_impl->cfg.seed ^ static_cast<uint64_t>(steps));
    QuestGraph g;
    g.id   = m_impl->nextGraphId++;
    g.seed = m_impl->cfg.seed;
    g.title = "Linear Quest " + std::to_string(g.id);
    for (uint32_t i = 0; i < steps; ++i) {
        bool entry = (i == 0), terminal = (i == steps - 1);
        g.nodes.push_back(makeNode(i + 1, rng, entry, terminal));
        if (i > 0) g.edges.push_back({i, i + 1, {}});
    }
    if (!g.nodes.empty()) {
        g.entryIds    = {g.nodes.front().id};
        g.terminalIds = {g.nodes.back().id};
    }
    m_impl->fire(g);
    return g;
}

QuestGraph QuestGraphGen::GenerateBranching(uint32_t depth, uint32_t width) {
    std::mt19937_64 rng(m_impl->cfg.seed ^ (static_cast<uint64_t>(depth) << 32) ^ width);
    QuestGraph g;
    g.id   = m_impl->nextGraphId++;
    g.seed = m_impl->cfg.seed;
    g.title = "Branching Quest " + std::to_string(g.id);

    uint32_t idCounter = 1;
    std::vector<uint32_t> prevLayer;

    // Entry node
    g.nodes.push_back(makeNode(idCounter++, rng, true, false));
    g.entryIds = {g.nodes.back().id};
    prevLayer  = {g.nodes.back().id};

    for (uint32_t d = 1; d < depth; ++d) {
        std::vector<uint32_t> thisLayer;
        bool isLast = (d == depth - 1);
        for (uint32_t w = 0; w < width; ++w) {
            g.nodes.push_back(makeNode(idCounter++, rng, false, isLast));
            uint32_t newId = g.nodes.back().id;
            thisLayer.push_back(newId);
            // Connect from all previous layer nodes
            for (auto prev : prevLayer)
                g.edges.push_back({prev, newId, {}});
        }
        if (isLast) for (auto id : thisLayer) g.terminalIds.push_back(id);
        prevLayer = thisLayer;
    }
    m_impl->fire(g);
    return g;
}

QuestGraph QuestGraphGen::GenerateRandom() {
    const auto& cfg = m_impl->cfg;
    std::mt19937_64 rng(cfg.seed);
    uint32_t nodeCount = cfg.minNodes + static_cast<uint32_t>(
        rng() % (cfg.maxNodes - cfg.minNodes + 1));

    QuestGraph g;
    g.id   = m_impl->nextGraphId++;
    g.seed = cfg.seed;
    g.title = "Random Quest " + std::to_string(g.id);

    for (uint32_t i = 0; i < nodeCount; ++i) {
        bool entry = (i == 0), terminal = (i == nodeCount - 1);
        g.nodes.push_back(makeNode(i + 1, rng, entry, terminal));
    }
    g.entryIds    = {g.nodes.front().id};
    g.terminalIds = {g.nodes.back().id};

    // Forward edges (no back-edges to keep DAG property)
    for (uint32_t i = 0; i + 1 < nodeCount; ++i) {
        g.edges.push_back({i + 1, i + 2, {}});
        // Optional extra branch
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        if (dist(rng) < cfg.branchProbability && i + 2 < nodeCount) {
            uint32_t target = i + 2 + static_cast<uint32_t>(rng() % (nodeCount - i - 2));
            g.edges.push_back({i + 1, target + 1, "optional"});
        }
    }
    m_impl->fire(g);
    return g;
}

bool QuestGraphGen::Validate(const QuestGraph& graph, std::string& outError) const {
    if (graph.nodes.empty()) { outError = "Empty graph"; return false; }
    if (graph.entryIds.empty()) { outError = "No entry nodes"; return false; }
    if (graph.terminalIds.empty()) { outError = "No terminal nodes"; return false; }
    if (!graph.IsValid()) { outError = "Terminals not reachable from entries"; return false; }
    return true;
}

void QuestGraphGen::OnGenerated(GeneratedCb cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

} // namespace PCG
