#pragma once
/**
 * @file TileGraph.h
 * @brief DAG-based tile-map generation graph with topological execution.
 *
 * Adapted from Nova-Forge-Expeditions (atlas::tile → Engine namespace).
 *
 * TileGraph is a node-based procedural tile generation pipeline:
 *   - TileNode: abstract base — Evaluate() transforms input TileValues to outputs.
 *   - TileEdge: wires an output port of one node to an input port of another.
 *   - TileGraph: owns nodes and edges; Compile() validates the DAG and builds
 *     topological execution order; Execute(ctx) runs nodes in order.
 *
 * TilePinType: TileID, TileMap, Float, Mask, Seed, Metadata.
 * TileGenContext: seed, mapWidth, mapHeight.
 * GetOutput(nodeId, port): retrieve evaluated output value.
 */

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine {

// ── Pin types ─────────────────────────────────────────────────────────────
enum class TilePinType : uint8_t { TileID, TileMap, Float, Mask, Seed, Metadata };

// ── Value ─────────────────────────────────────────────────────────────────
struct TileValue {
    TilePinType        type{TilePinType::Float};
    std::vector<float> data;
};

// ── Port ──────────────────────────────────────────────────────────────────
struct TilePort {
    std::string name;
    TilePinType type{TilePinType::Float};
};

// ── Ids ───────────────────────────────────────────────────────────────────
using TileNodeID = uint32_t;
using TilePortID = uint16_t;

// ── Edge ──────────────────────────────────────────────────────────────────
struct TileEdge {
    TileNodeID fromNode{0};
    TilePortID fromPort{0};
    TileNodeID toNode{0};
    TilePortID toPort{0};
};

// ── Generation context ────────────────────────────────────────────────────
struct TileGenContext {
    uint32_t seed{0};
    int32_t  mapWidth{64};
    int32_t  mapHeight{64};
};

// ── Abstract tile node ────────────────────────────────────────────────────
class TileNode {
public:
    virtual ~TileNode() = default;
    virtual const char*             GetName()     const = 0;
    virtual const char*             GetCategory() const = 0;
    virtual std::vector<TilePort>   Inputs()      const = 0;
    virtual std::vector<TilePort>   Outputs()     const = 0;
    virtual void Evaluate(const TileGenContext& ctx,
                          const std::vector<TileValue>& inputs,
                          std::vector<TileValue>&       outputs) const = 0;
};

// ── Graph ─────────────────────────────────────────────────────────────────
class TileGraph {
public:
    TileGraph()  = default;
    ~TileGraph() = default;

    TileGraph(const TileGraph&) = delete;
    TileGraph& operator=(const TileGraph&) = delete;

    // ── graph construction ────────────────────────────────────
    TileNodeID AddNode(std::unique_ptr<TileNode> node);
    void       RemoveNode(TileNodeID id);
    void       AddEdge(const TileEdge& edge);
    void       RemoveEdge(const TileEdge& edge);

    // ── compilation / execution ───────────────────────────────
    bool Compile();
    bool Execute(const TileGenContext& ctx);

    // ── output queries ────────────────────────────────────────
    const TileValue* GetOutput(TileNodeID node, TilePortID port) const;
    size_t           NodeCount()   const;
    bool             IsCompiled()  const;

private:
    bool HasCycle()          const;
    bool ValidateEdgeTypes() const;

    TileNodeID m_nextID{1};
    std::unordered_map<TileNodeID, std::unique_ptr<TileNode>> m_nodes;
    std::vector<TileEdge>                                     m_edges;
    std::vector<TileNodeID>                                   m_executionOrder;
    bool                                                      m_compiled{false};
    std::unordered_map<uint64_t, TileValue>                   m_outputs;
};

} // namespace Engine
