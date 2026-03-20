#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace PCG {

enum class MeshPinType : uint8_t {
    Mesh,
    Float,
    Vector3,
    Int,
    Seed,
    Mask
};

struct MeshValue {
    MeshPinType              type = MeshPinType::Mesh;
    std::vector<float>       floatData;
    std::vector<float>       vertices;
    std::vector<uint32_t>    indices;
};

struct MeshPort {
    std::string name;
    MeshPinType type = MeshPinType::Mesh;
};

struct MeshGenContext {
    uint64_t seed  = 0;
    uint8_t  lod   = 0;
    float    scale = 1.0f;
};

using MeshNodeID = uint32_t;
using MeshPortID = uint16_t;

struct MeshEdge {
    MeshNodeID fromNode;
    MeshPortID fromPort;
    MeshNodeID toNode;
    MeshPortID toPort;
};

class IMeshNode {
public:
    virtual ~IMeshNode() = default;
    virtual const char*            GetName()     const = 0;
    virtual const char*            GetCategory() const = 0;
    virtual std::vector<MeshPort>  Inputs()      const = 0;
    virtual std::vector<MeshPort>  Outputs()     const = 0;
    virtual void Evaluate(const MeshGenContext& ctx,
                          const std::vector<MeshValue>& inputs,
                          std::vector<MeshValue>& outputs) const = 0;
};

class MeshNodeGraph {
public:
    MeshNodeID AddNode(std::unique_ptr<IMeshNode> node);
    void       RemoveNode(MeshNodeID id);
    void       AddEdge(MeshNodeID from, MeshPortID fromPort,
                       MeshNodeID to,   MeshPortID toPort);
    void       RemoveEdge(MeshNodeID from, MeshPortID fromPort,
                          MeshNodeID to,   MeshPortID toPort);

    bool Compile();
    bool Execute(const MeshGenContext& ctx);

    const MeshValue* GetOutput(MeshNodeID nodeId, MeshPortID portId) const;
    size_t           NodeCount()   const;
    bool             IsCompiled()  const;

private:
    MeshNodeID m_nextID = 1;
    std::unordered_map<MeshNodeID, std::unique_ptr<IMeshNode>> m_nodes;
    std::vector<MeshEdge>  m_edges;
    std::vector<MeshNodeID> m_executionOrder;
    std::unordered_map<uint64_t, MeshValue> m_outputs;
    bool m_compiled = false;

    bool HasCycle()          const;
    bool ValidateEdgeTypes() const;
};

// ---- Concrete nodes --------------------------------------------------------

class HullGeneratorNode : public IMeshNode {
public:
    const char*            GetName()     const override { return "HullGenerator"; }
    const char*            GetCategory() const override { return "Geometry"; }
    std::vector<MeshPort>  Inputs()      const override;
    std::vector<MeshPort>  Outputs()     const override;
    void Evaluate(const MeshGenContext& ctx,
                  const std::vector<MeshValue>& inputs,
                  std::vector<MeshValue>& outputs) const override;
};

class SubdivideNode : public IMeshNode {
public:
    const char*            GetName()     const override { return "Subdivide"; }
    const char*            GetCategory() const override { return "Geometry"; }
    std::vector<MeshPort>  Inputs()      const override;
    std::vector<MeshPort>  Outputs()     const override;
    void Evaluate(const MeshGenContext& ctx,
                  const std::vector<MeshValue>& inputs,
                  std::vector<MeshValue>& outputs) const override;
};

class NoiseDeformNode : public IMeshNode {
public:
    const char*            GetName()     const override { return "NoiseDeform"; }
    const char*            GetCategory() const override { return "Geometry"; }
    std::vector<MeshPort>  Inputs()      const override;
    std::vector<MeshPort>  Outputs()     const override;
    void Evaluate(const MeshGenContext& ctx,
                  const std::vector<MeshValue>& inputs,
                  std::vector<MeshValue>& outputs) const override;
};

} // namespace PCG
