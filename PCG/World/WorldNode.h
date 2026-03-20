#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace PCG {

enum class WorldPinType : uint8_t { TileID, TileMap, Float, Mask, Seed, Metadata };

struct WorldValue {
    WorldPinType       type = WorldPinType::Float;
    std::vector<float> data;
};

struct WorldPort {
    std::string  name;
    WorldPinType type = WorldPinType::Float;
};

struct WorldGenContext {
    uint32_t seed     = 0;
    int32_t  mapWidth = 64;
    int32_t  mapHeight= 64;
};

using WorldNodeID = uint32_t;
using WorldPortID = uint16_t;

struct WorldEdge {
    WorldNodeID fromNode;
    WorldPortID fromPort;
    WorldNodeID toNode;
    WorldPortID toPort;
};

class IWorldNode {
public:
    virtual ~IWorldNode() = default;
    virtual const char*             GetName()     const = 0;
    virtual const char*             GetCategory() const = 0;
    virtual std::vector<WorldPort>  Inputs()      const = 0;
    virtual std::vector<WorldPort>  Outputs()     const = 0;
    virtual void Evaluate(const WorldGenContext& ctx,
                          const std::vector<WorldValue>& inputs,
                          std::vector<WorldValue>& outputs) const = 0;
};

class WorldGraph {
public:
    WorldNodeID AddNode(std::unique_ptr<IWorldNode> node);
    void        RemoveNode(WorldNodeID id);
    void        AddEdge(const WorldEdge& edge);
    void        RemoveEdge(const WorldEdge& edge);

    bool Compile();
    bool Execute(const WorldGenContext& ctx);

    const WorldValue* GetOutput(WorldNodeID node, WorldPortID port) const;
    size_t            NodeCount()  const;
    bool              IsCompiled() const;

private:
    WorldNodeID m_nextID = 1;
    std::unordered_map<WorldNodeID, std::unique_ptr<IWorldNode>> m_nodes;
    std::vector<WorldEdge>  m_edges;
    std::vector<WorldNodeID> m_executionOrder;
    std::unordered_map<uint64_t, WorldValue> m_outputs;
    bool m_compiled = false;

    bool HasCycle()          const;
    bool ValidateEdgeTypes() const;
};

} // namespace PCG
