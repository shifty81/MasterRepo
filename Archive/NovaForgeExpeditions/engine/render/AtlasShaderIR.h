#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace atlas::render {

/// Shader stages
enum class ShaderStage : uint8_t {
    Vertex,
    Fragment,
    Compute
};

/// Shader IR opcodes
enum class ShaderOp : uint8_t {
    Nop = 0,
    // Data
    LoadConst,      // Push constant value
    LoadInput,      // Load vertex/fragment input
    LoadUniform,    // Load uniform variable
    StoreOutput,    // Store to output
    // Math
    Add,
    Sub,
    Mul,
    Div,
    Dot,            // Dot product
    Cross,          // Cross product
    Normalize,      // Normalize vector
    Lerp,           // Linear interpolation
    Clamp,          // Clamp value
    // Texture
    SampleTexture,  // Sample from texture
    // Control
    Return
};

/// Single IR instruction
struct ShaderInstruction {
    ShaderOp op = ShaderOp::Nop;
    uint16_t operand0 = 0;
    uint16_t operand1 = 0;
    uint16_t result = 0;
    float constValue = 0.0f;  // For LoadConst
};

/// Shader uniform declaration
struct ShaderUniform {
    std::string name;
    uint16_t binding = 0;
    uint16_t size = 0;    // Size in bytes
};

/// Shader input/output declaration
struct ShaderIO {
    std::string name;
    uint16_t location = 0;
    uint8_t componentCount = 4; // vec2=2, vec3=3, vec4=4
};

/// Complete shader IR module
struct ShaderModule {
    static constexpr uint32_t MAGIC = 0x53484452; // "SHDR"
    static constexpr uint32_t VERSION = 1;

    ShaderStage stage = ShaderStage::Vertex;
    std::string name;
    std::vector<ShaderIO> inputs;
    std::vector<ShaderIO> outputs;
    std::vector<ShaderUniform> uniforms;
    std::vector<ShaderInstruction> instructions;

    /// Validate the IR module for correctness
    bool Validate() const;

    /// Compute a deterministic hash of the shader IR
    uint64_t Hash() const;

    /// Serialize to binary format
    std::vector<uint8_t> Serialize() const;

    /// Deserialize from binary format
    static ShaderModule Deserialize(const std::vector<uint8_t>& data);
};

/// Compile a shader IR module from a simple description.
/// This is deterministic — same input always produces same output.
class ShaderIRCompiler {
public:
    /// Create a simple pass-through vertex shader
    static ShaderModule CreatePassthroughVertex();

    /// Create a simple solid-color fragment shader
    static ShaderModule CreateSolidColorFragment(float r, float g, float b, float a);

    /// Compile IR instructions from a list of high-level operations
    /// Returns true if compilation succeeded
    bool Compile(ShaderModule& module);

    /// Get compilation errors
    const std::vector<std::string>& Errors() const;

private:
    std::vector<std::string> m_errors;
    bool ValidateInstruction(const ShaderInstruction& instr, const ShaderModule& module);
};

} // namespace atlas::render
