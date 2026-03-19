#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace atlas::animation {

enum class AnimPinType : uint8_t {
    Float,       // Scalar (time, weight, blend factor)
    Pose,        // Skeletal pose data
    Modifier,    // Animation modifier (limp, recoil, tremor)
    Trigger,     // State transition trigger
    Mask         // Bone mask for partial blending
};

struct AnimValue {
    AnimPinType type;
    std::vector<float> data;
};

struct AnimPort {
    std::string name;
    AnimPinType type;
};

using AnimNodeID = uint32_t;
using AnimPortID = uint16_t;

struct AnimEdge {
    AnimNodeID fromNode;
    AnimPortID fromPort;
    AnimNodeID toNode;
    AnimPortID toPort;
};

struct AnimContext {
    float deltaTime;
    float speed;
    float fatigue;
    float damageLevel;
    uint32_t tick;
};

class AnimNode {
public:
    virtual ~AnimNode() = default;
    virtual const char* GetName() const = 0;
    virtual const char* GetCategory() const = 0;
    virtual std::vector<AnimPort> Inputs() const = 0;
    virtual std::vector<AnimPort> Outputs() const = 0;
    virtual void Evaluate(const AnimContext& ctx,
                          const std::vector<AnimValue>& inputs,
                          std::vector<AnimValue>& outputs) const = 0;
};

class AnimationGraph {
public:
    AnimNodeID AddNode(std::unique_ptr<AnimNode> node);
    void RemoveNode(AnimNodeID id);
    void AddEdge(const AnimEdge& edge);
    void RemoveEdge(const AnimEdge& edge);

    bool Compile();
    bool Execute(const AnimContext& ctx);

    const AnimValue* GetOutput(AnimNodeID node, AnimPortID port) const;
    size_t NodeCount() const;
    bool IsCompiled() const;

private:
    AnimNodeID m_nextID = 1;
    std::unordered_map<AnimNodeID, std::unique_ptr<AnimNode>> m_nodes;
    std::vector<AnimEdge> m_edges;
    std::vector<AnimNodeID> m_executionOrder;
    bool m_compiled = false;

    std::unordered_map<uint64_t, AnimValue> m_outputs;

    bool HasCycle() const;
    bool ValidateEdgeTypes() const;
};

}
