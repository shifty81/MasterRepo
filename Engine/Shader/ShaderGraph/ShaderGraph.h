#pragma once
/**
 * @file ShaderGraph.h
 * @brief Node-based shader graph for visual material authoring.
 *
 * Each node has typed input/output pins. A Compiler pass traverses the DAG and
 * emits GLSL (or HLSL) source code. Supports:
 *   - Texture sample, math, colour mix, fresnel, UV transform nodes
 *   - Master surface output node (albedo, roughness, metallic, emissive, normal)
 *   - Subgraph (reusable node group) support
 *   - JSON-based serialization for save/load
 *
 * Typical usage:
 * @code
 *   ShaderGraph graph;
 *   graph.Init();
 *   uint32_t texNode   = graph.AddNode(NodeType::TextureSample);
 *   uint32_t mixNode   = graph.AddNode(NodeType::Mix);
 *   uint32_t outNode   = graph.AddNode(NodeType::SurfaceOutput);
 *   graph.Connect(texNode, 0, mixNode, 0);
 *   graph.Connect(mixNode, 0, outNode, 0);   // albedo pin
 *   ShaderCompileResult r = graph.Compile(ShaderTarget::GLSL);
 *   // r.vertexSource / r.fragmentSource
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

// ── Node types ────────────────────────────────────────────────────────────────

enum class ShaderNodeType : uint8_t {
    // Inputs
    Float = 0, Vec2, Vec3, Vec4, Colour, TextureSample, Time, UV, VertexNormal,
    // Math
    Add, Subtract, Multiply, Divide, Dot, Cross, Normalize, Lerp, Clamp,
    Pow, Abs, Frac, Floor, Ceil, Sin, Cos, Sqrt, OneMinus,
    // Utility
    Split, Combine, Remap, Fresnel, Step, SmoothStep,
    // Output
    SurfaceOutput,
    COUNT
};

// ── Pin data type ─────────────────────────────────────────────────────────────

enum class PinType : uint8_t { Float1=0, Float2, Float3, Float4, Sampler2D };

// ── A pin on a node ───────────────────────────────────────────────────────────

struct ShaderPin {
    uint32_t    nodeId{0};
    uint32_t    pinIndex{0};
    std::string name;
    PinType     type{PinType::Float4};
    bool        isOutput{false};
    float       defaultValue[4]{0,0,0,0};
};

// ── A connection between two pins ─────────────────────────────────────────────

struct ShaderEdge {
    uint32_t fromNode{0};
    uint32_t fromPin{0};
    uint32_t toNode{0};
    uint32_t toPin{0};
};

// ── A graph node ──────────────────────────────────────────────────────────────

struct ShaderNode {
    uint32_t        id{0};
    ShaderNodeType  type{ShaderNodeType::Float};
    std::string     label;
    float           posX{0.f}, posY{0.f};  ///< visual editor position
    std::vector<ShaderPin> inputs;
    std::vector<ShaderPin> outputs;
    std::string     textureAsset;          ///< for TextureSample nodes
    float           floatValue{0.f};       ///< for constant Float nodes
    float           vecValue[4]{};         ///< for Vec2/Vec3/Vec4 nodes
};

// ── Compile target ────────────────────────────────────────────────────────────

enum class ShaderTarget : uint8_t { GLSL = 0, HLSL = 1 };

// ── Compile result ────────────────────────────────────────────────────────────

struct ShaderCompileResult {
    bool        succeeded{false};
    std::string vertexSource;
    std::string fragmentSource;
    std::string errorMessage;
    std::vector<std::string> warnings;
    std::vector<std::string> uniformNames;
};

// ── ShaderGraph ───────────────────────────────────────────────────────────────

class ShaderGraph {
public:
    ShaderGraph();
    ~ShaderGraph();

    void Init();
    void Shutdown();

    // ── Node management ───────────────────────────────────────────────────────

    uint32_t AddNode(ShaderNodeType type, const std::string& label = "");
    void     RemoveNode(uint32_t nodeId);
    bool     HasNode(uint32_t nodeId) const;
    ShaderNode  GetNode(uint32_t nodeId) const;
    void        SetNodePosition(uint32_t nodeId, float x, float y);
    void        SetNodeValue(uint32_t nodeId, float value);
    void        SetNodeVecValue(uint32_t nodeId, const float value[4]);
    void        SetNodeTexture(uint32_t nodeId, const std::string& assetPath);

    std::vector<ShaderNode> AllNodes() const;

    // ── Connections ───────────────────────────────────────────────────────────

    bool Connect(uint32_t fromNode, uint32_t fromPin,
                 uint32_t toNode,   uint32_t toPin);
    void Disconnect(uint32_t fromNode, uint32_t fromPin,
                    uint32_t toNode,   uint32_t toPin);
    bool IsConnected(uint32_t fromNode, uint32_t fromPin,
                     uint32_t toNode,   uint32_t toPin) const;
    std::vector<ShaderEdge> AllEdges() const;

    // ── Compile ───────────────────────────────────────────────────────────────

    ShaderCompileResult Compile(ShaderTarget target = ShaderTarget::GLSL) const;

    // ── Serialization ─────────────────────────────────────────────────────────

    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);
    std::string ToJSON() const;
    bool FromJSON(const std::string& json);

    // ── Utility ───────────────────────────────────────────────────────────────

    void Clear();
    bool Validate(std::vector<std::string>& errors) const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnNodeAdded(std::function<void(uint32_t id)> cb);
    void OnEdgeAdded(std::function<void(const ShaderEdge&)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
