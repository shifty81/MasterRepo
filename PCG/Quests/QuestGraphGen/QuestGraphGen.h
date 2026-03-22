#pragma once
/**
 * @file QuestGraphGen.h
 * @brief Procedural quest graph generator: nodes, dependencies, branches, rewards.
 *
 * QuestGraphGen builds directed acyclic graphs (DAGs) representing quest chains:
 *   - QuestNode: unique id, type (Kill/Collect/Escort/Explore/Deliver/Talk),
 *     objective text, location tag, required item/enemy, quantity, and reward.
 *   - QuestEdge: directed dependency (fromId → toId) with an optional condition.
 *   - QuestGraph: node + edge lists, entry nodes, and terminal (reward) nodes.
 *   - GenerateLinear(n): simple n-step chain.
 *   - GenerateBranching(depth, width): tree-shaped graph with merging paths.
 *   - GenerateRandom(cfg): fully parameterised graph from a seed.
 *   - Validate(): checks for cycles, orphan nodes, and unreachable terminals.
 *   - Serialise() / Deserialise(): JSON round-trip for persistence.
 *   - OnGenerated: callback fired after each Generate* call.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace PCG {

// ── Quest node type ────────────────────────────────────────────────────────
enum class QuestNodeType : uint8_t {
    Kill, Collect, Escort, Explore, Deliver, Talk, Craft, Survive
};
const char* QuestNodeTypeName(QuestNodeType t);

// ── Reward ────────────────────────────────────────────────────────────────
struct QuestReward {
    uint32_t    xp{0};
    uint32_t    gold{0};
    std::string itemId;
    uint32_t    itemCount{0};
};

// ── Quest node ────────────────────────────────────────────────────────────
struct QuestNode {
    uint32_t      id{0};
    QuestNodeType type{QuestNodeType::Kill};
    std::string   objective;      ///< Human-readable task description
    std::string   locationTag;    ///< World-space region identifier
    std::string   targetId;       ///< Enemy type, item id, NPC id, etc.
    uint32_t      quantity{1};
    QuestReward   reward{};
    bool          isEntry{false};
    bool          isTerminal{false};
};

// ── Quest edge ────────────────────────────────────────────────────────────
struct QuestEdge {
    uint32_t    fromId{0};
    uint32_t    toId{0};
    std::string condition;  ///< Optional: "choice_a", "player_class==warrior"
};

// ── Quest graph ───────────────────────────────────────────────────────────
struct QuestGraph {
    uint32_t                id{0};
    std::string             title;
    std::vector<QuestNode>  nodes;
    std::vector<QuestEdge>  edges;
    std::vector<uint32_t>   entryIds;
    std::vector<uint32_t>   terminalIds;
    uint64_t                seed{0};

    bool IsValid() const;           ///< True if no cycles and all terminals reachable
    std::vector<uint32_t> DependenciesOf(uint32_t nodeId) const;
    std::vector<uint32_t> DependentsOf(uint32_t nodeId) const;
    std::string Serialise() const;  ///< Minimal JSON export
};

// ── Generator config ──────────────────────────────────────────────────────
struct QuestGraphConfig {
    uint32_t minNodes{4};
    uint32_t maxNodes{12};
    uint32_t branchFactor{2};   ///< Max out-degree per non-terminal node
    float    branchProbability{0.35f};
    float    mergeProbability{0.2f};
    bool     allowOptionalBranches{true};
    uint64_t seed{0};
};

// ── Generator ─────────────────────────────────────────────────────────────
class QuestGraphGen {
public:
    QuestGraphGen();
    ~QuestGraphGen();

    void SetConfig(const QuestGraphConfig& cfg);
    const QuestGraphConfig& Config() const;

    // ── generation ────────────────────────────────────────────
    QuestGraph GenerateLinear(uint32_t steps);
    QuestGraph GenerateBranching(uint32_t depth, uint32_t width);
    QuestGraph GenerateRandom();

    // ── validate ──────────────────────────────────────────────
    bool Validate(const QuestGraph& graph, std::string& outError) const;

    // ── callback ──────────────────────────────────────────────
    using GeneratedCb = std::function<void(const QuestGraph&)>;
    void OnGenerated(GeneratedCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
