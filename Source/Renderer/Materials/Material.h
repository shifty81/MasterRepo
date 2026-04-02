#pragma once
#include "Core/Math/Vector.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace NF {

class Shader;
class Texture;

/// @brief Describes the surface appearance of a rendered object.
///
/// A Material owns a (non-owning) reference to a Shader, zero or more
/// Texture slots, and a set of scalar/vector uniform overrides.
class Material {
public:
    /// @brief Set the shader program used to render this material.
    /// @param shader Non-owning pointer; must outlive this material.
    void SetShader(Shader* shader);

    /// @brief Assign a texture to a numbered slot.
    /// @param slot    Texture unit index (0-based).
    /// @param texture Non-owning pointer; must outlive this material.
    void SetTexture(uint32_t slot, Texture* texture);

    /// @brief Store a named float override that will be uploaded when Bind() is called.
    void SetFloat(const std::string& name, float value);

    /// @brief Store a named vec3 override that will be uploaded when Bind() is called.
    void SetVec3(const std::string& name, const Vector3& value);

    /// @brief Bind the shader, upload all uniforms, and bind all textures.
    void Bind();

    /// @brief Return the shader currently assigned to this material.
    [[nodiscard]] Shader* GetShader() const noexcept { return m_Shader; }

private:
    Shader*                                    m_Shader{nullptr};
    std::vector<std::pair<uint32_t, Texture*>> m_Textures;
    std::unordered_map<std::string, float>     m_Floats;
    std::unordered_map<std::string, Vector3>   m_Vec3s;
};

} // namespace NF
