#include "Renderer/Materials/Material.h"
#include "Renderer/RHI/Shader.h"
#include "Renderer/RHI/Texture.h"

namespace NF {

void Material::SetShader(Shader* shader) {
    m_Shader = shader;
}

void Material::SetTexture(uint32_t slot, Texture* texture) {
    for (auto& [s, t] : m_Textures) {
        if (s == slot) { t = texture; return; }
    }
    m_Textures.emplace_back(slot, texture);
}

void Material::SetFloat(const std::string& name, float value) {
    m_Floats[name] = value;
}

void Material::SetVec3(const std::string& name, const Vector3& value) {
    m_Vec3s[name] = value;
}

void Material::Bind() {
    if (!m_Shader) return;

    m_Shader->Bind();

    for (const auto& [name, val] : m_Floats)
        m_Shader->SetFloat(name, val);

    for (const auto& [name, val] : m_Vec3s)
        m_Shader->SetVec3(name, val);

    for (const auto& [slot, tex] : m_Textures)
        if (tex) tex->Bind(slot);
}

} // namespace NF
