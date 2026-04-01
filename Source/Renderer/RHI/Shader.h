#pragma once
#include "Core/Math/Vector.h"
#include "Core/Math/Matrix.h"
#include <cstdint>
#include <string>

namespace NF {

/// @brief Stage within the programmable pipeline that a shader occupies.
enum class ShaderStage { Vertex, Fragment, Geometry, Compute };

/// @brief Opaque handle to a compiled GPU shader program.
using ShaderId = uint32_t;

/// @brief Encapsulates a linked GPU shader program.
class Shader {
public:
    /// @brief Compile and link a shader program from GLSL source strings.
    /// @param vertSrc GLSL source for the vertex stage.
    /// @param fragSrc GLSL source for the fragment stage.
    /// @return True on success.
    bool LoadFromSource(const std::string& vertSrc, const std::string& fragSrc);

    /// @brief Bind this shader program for subsequent draw calls.
    void Bind();

    /// @brief Unbind the current shader program.
    void Unbind();

    /// @brief Upload a signed integer uniform.
    void SetInt(const std::string& name, int val);

    /// @brief Upload a float uniform.
    void SetFloat(const std::string& name, float val);

    /// @brief Upload a vec3 uniform.
    void SetVec3(const std::string& name, const Vector3& val);

    /// @brief Upload a mat4 uniform.
    void SetMat4(const std::string& name, const Matrix4x4& val);

    /// @brief Return the underlying GPU program handle.
    [[nodiscard]] ShaderId GetId() const noexcept { return m_Id; }

private:
    ShaderId m_Id{0};
};

} // namespace NF
