#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace atlas::tile {

enum class TilePinType : uint8_t {
    TileID,
    TileMap,
    Float,
    Mask,
    Seed,
    Metadata
};

struct TileValue {
    TilePinType type;
    std::vector<float> data;
};

struct TilePort {
    std::string name;
    TilePinType type;
};

using TileNodeID = uint32_t;
using TilePortID = uint16_t;

struct TileEdge {
    TileNodeID fromNode;
    TilePortID fromPort;
    TileNodeID toNode;
    TilePortID toPort;
};

struct TileGenContext {
    uint32_t seed;
    int32_t mapWidth;
    int32_t mapHeight;
};

class TileNode {
public:
    virtual ~TileNode() = default;
    virtual const char* GetName() const = 0;
    virtual const char* GetCategory() const = 0;
    virtual std::vector<TilePort> Inputs() const = 0;
    virtual std::vector<TilePort> Outputs() const = 0;
    virtual void Evaluate(const TileGenContext& ctx,
                          const std::vector<TileValue>& inputs,
                          std::vector<TileValue>& outputs) const = 0;
};

class TileGraph {
public:
    TileNodeID AddNode(std::unique_ptr<TileNode> node);
    void RemoveNode(TileNodeID id);
    void AddEdge(const TileEdge& edge);
    void RemoveEdge(const TileEdge& edge);

    bool Compile();
    bool Execute(const TileGenContext& ctx);

    const TileValue* GetOutput(TileNodeID node, TilePortID port) const;
    size_t NodeCount() const;
    bool IsCompiled() const;

private:
    TileNodeID m_nextID = 1;
    std::unordered_map<TileNodeID, std::unique_ptr<TileNode>> m_nodes;
    std::vector<TileEdge> m_edges;
    std::vector<TileNodeID> m_executionOrder;
    bool m_compiled = false;

    std::unordered_map<uint64_t, TileValue> m_outputs;

    bool HasCycle() const;
    bool ValidateEdgeTypes() const;
};

}
