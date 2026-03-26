#pragma once
/**
 * @file BlueprintCompiler.h
 * @brief Compiles visual Blueprint graphs into executable C++ code or Lua.
 *
 * A Blueprint graph is a node-based script where nodes represent:
 *   - Event entry points (OnBeginPlay, OnHit, OnTick)
 *   - Function calls (engine API, user functions)
 *   - Flow control (Branch, Loop, Sequence, Delay)
 *   - Variable get/set
 *   - Math / string / array operations
 *
 * The compiler validates the graph, performs type checking, and emits:
 *   - C++ source (compilable into a plugin or module)
 *   - Lua source (interpreted at runtime via the existing LuaBinding)
 *
 * Typical usage:
 * @code
 *   BlueprintCompiler compiler;
 *   compiler.Init();
 *   BlueprintGraph graph = LoadGraphFromJSON("Blueprints/npc.bp.json");
 *   BlueprintCompileResult r = compiler.Compile(graph, BlueprintTarget::Lua);
 *   if (r.succeeded) { luaState.Execute(r.source); }
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Builder {

// ── Node types ────────────────────────────────────────────────────────────────

enum class BPNodeType : uint8_t {
    // Events
    EventBeginPlay=0, EventTick, EventHit, EventCustom,
    // Flow
    Branch, ForLoop, WhileLoop, Sequence, Delay, Gate,
    // Variables
    GetVariable, SetVariable,
    // Functions
    CallFunction, CallEngineAPI, PrintString, Spawn, Destroy,
    // Math
    Add, Subtract, Multiply, Divide, Compare, Not, And, Or,
    // Utility
    CastNode, IsValid, ForEachArray, MakeArray, ArrayGet,
    COUNT
};

// ── A blueprint pin ───────────────────────────────────────────────────────────

struct BPPin {
    uint32_t    id{0};
    std::string name;
    std::string typeName;   ///< "bool", "int", "float", "string", "Actor", …
    bool        isExec{false};  ///< execution flow pin
    bool        isOutput{false};
    std::string defaultValue;
};

// ── A blueprint node ──────────────────────────────────────────────────────────

struct BPNode {
    uint32_t               id{0};
    BPNodeType             type{BPNodeType::CallFunction};
    std::string            label;
    std::string            functionName;  ///< for CallFunction / CallEngineAPI
    std::string            variableName;  ///< for Get/SetVariable
    std::vector<BPPin>     inputs;
    std::vector<BPPin>     outputs;
    float                  posX{0.f}, posY{0.f};
};

// ── A wire between pins ───────────────────────────────────────────────────────

struct BPWire {
    uint32_t fromNode{0}, fromPin{0};
    uint32_t toNode{0},   toPin{0};
};

// ── Blueprint graph ───────────────────────────────────────────────────────────

struct BlueprintGraph {
    std::string          name;
    std::string          targetClass;  ///< class this Blueprint extends
    std::vector<BPNode>  nodes;
    std::vector<BPWire>  wires;
};

// ── Compile target ────────────────────────────────────────────────────────────

enum class BlueprintTarget : uint8_t { Lua = 0, CPP = 1 };

// ── Compile result ────────────────────────────────────────────────────────────

struct BlueprintCompileResult {
    bool        succeeded{false};
    std::string source;           ///< generated code
    std::string errorMessage;
    std::vector<std::string> warnings;
    std::vector<std::string> usedFunctions;
};

// ── BlueprintCompiler ─────────────────────────────────────────────────────────

class BlueprintCompiler {
public:
    BlueprintCompiler();
    ~BlueprintCompiler();

    void Init();
    void Shutdown();

    // ── Compile ───────────────────────────────────────────────────────────────

    BlueprintCompileResult Compile(const BlueprintGraph& graph,
                                   BlueprintTarget target = BlueprintTarget::Lua);

    // ── Validation ────────────────────────────────────────────────────────────

    std::vector<std::string> Validate(const BlueprintGraph& graph) const;

    // ── Graph serialization ───────────────────────────────────────────────────

    static bool          SaveGraph(const BlueprintGraph& graph,
                                   const std::string& path);
    static BlueprintGraph LoadGraph(const std::string& path);

    // ── API registry ──────────────────────────────────────────────────────────

    void RegisterEngineAPI(const std::string& funcName,
                           const std::string& signature);
    bool IsKnownAPI(const std::string& funcName) const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnCompileComplete(std::function<void(const BlueprintCompileResult&)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Builder
