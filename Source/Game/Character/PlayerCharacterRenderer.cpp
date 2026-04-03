#include "Game/Character/PlayerCharacterRenderer.h"
#include "Game/Character/CharacterMeshBuilder.h"
#include "Game/Character/CharacterDef.h"
#include "Renderer/Pipeline/ForwardRenderer.h"
#include "Renderer/Materials/Material.h"
#include "Core/Logging/Log.h"
#include <cmath>

namespace NF::Game {

// ---------------------------------------------------------------------------
// Character GLSL shaders
// Palette-based Phong lighting that mirrors the voxel shader convention.
// TexCoord.X = CharacterColor index (0–7).
// ---------------------------------------------------------------------------

static const char* kCharVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uModel;

out vec3 vNormal;
out vec3 vWorldPos;
out float vColorId;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    gl_Position   = uProjection * uView * worldPos;
    // Transform normal to world space (uniform scale assumed)
    vNormal       = mat3(uModel) * aNormal;
    vWorldPos     = worldPos.xyz;
    vColorId      = aTexCoord.x;
}
)";

static const char* kCharFragSrc = R"(
#version 330 core
in  vec3  vNormal;
in  vec3  vWorldPos;
in  float vColorId;
out vec4  FragColor;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uViewPos;

// 8-entry character colour palette (matches CharacterColor enum order):
//  0 Hair        1 Skin       2 CoatGreen  3 CoatDark
//  4 InnerShirt  5 Pants      6 Boots      7 Belt
vec3 charPalette(int id) {
    if (id == 0) return vec3(0.15, 0.12, 0.10);  // Hair       — dark charcoal
    if (id == 1) return vec3(0.78, 0.60, 0.48);  // Skin       — warm tan
    if (id == 2) return vec3(0.32, 0.42, 0.22);  // CoatGreen  — military green
    if (id == 3) return vec3(0.22, 0.30, 0.15);  // CoatDark   — collar shadow
    if (id == 4) return vec3(0.48, 0.30, 0.16);  // InnerShirt — warm brown
    if (id == 5) return vec3(0.12, 0.12, 0.18);  // Pants      — near-black navy
    if (id == 6) return vec3(0.38, 0.24, 0.14);  // Boots      — tan-brown leather
    if (id == 7) return vec3(0.20, 0.15, 0.10);  // Belt       — dark leather
    return vec3(1.0, 0.0, 1.0);                   // Unknown    — magenta debug
}

void main() {
    vec3  baseColor = charPalette(int(vColorId + 0.5));
    vec3  N         = normalize(vNormal);
    vec3  L         = normalize(-uLightDir);
    float diff      = max(dot(N, L), 0.0);

    // Soft specular for the coat fabric / leather
    vec3  V         = normalize(uViewPos - vWorldPos);
    vec3  H         = normalize(L + V);
    float spec      = pow(max(dot(N, H), 0.0), 24.0) * 0.15;

    vec3  ambient   = uAmbientColor * baseColor;
    vec3  diffuse   = uLightColor   * baseColor * diff;
    vec3  specular  = uLightColor   * spec;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
)";

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
bool PlayerCharacterRenderer::Init(RenderDevice* /*renderDevice*/)
{
    if (!m_Shader.LoadFromSource(kCharVertSrc, kCharFragSrc))
    {
        NF_LOG_ERROR("Character", "PlayerCharacterRenderer: shader compile failed");
        return false;
    }

    CharacterMeshBuilder builder;
    MeshData meshData = builder.BuildFullCharacter();
    m_Mesh.Upload(meshData);

    NF_LOG_INFO("Character",
                "PlayerCharacterRenderer: ready ("
                + std::to_string(meshData.Vertices.size()) + " verts, "
                + std::to_string(meshData.Indices.size() / 3) + " tris)");

    m_Ready = true;
    return true;
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------
void PlayerCharacterRenderer::Shutdown()
{
    m_Ready = false;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------
void PlayerCharacterRenderer::Render(ForwardRenderer& renderer,
                                     const NF::Vector3& feetPosition,
                                     float yawRadians)
{
    if (!m_Ready)
    {
        return;
    }

    // Upload per-frame lighting defaults via shader uniforms.
    // The ForwardRenderer sets uView / uProjection / uModel from
    // the material bind + Submit path; we set the lighting uniforms here
    // directly since they are not part of the NF::Material interface yet.
    m_Shader.Bind();
    m_Shader.SetVec3("uLightDir",     { 0.5f, -1.0f, 0.3f});
    m_Shader.SetVec3("uLightColor",   { 1.0f,  0.97f, 0.90f});
    m_Shader.SetVec3("uAmbientColor", { 0.18f, 0.20f, 0.25f});
    m_Shader.SetVec3("uViewPos",      feetPosition);

    // Build a minimal Material wrapper so the ForwardRenderer can bind our
    // shader and receive the model matrix through its standard Submit path.
    Material mat;
    mat.SetShader(&m_Shader);

    const Matrix4x4 transform = BuildTransform(feetPosition, yawRadians);
    renderer.Submit(m_Mesh, mat, transform);
}

// ---------------------------------------------------------------------------
// BuildTransform
// ---------------------------------------------------------------------------
Matrix4x4 PlayerCharacterRenderer::BuildTransform(const NF::Vector3& pos, float yaw)
{
    // Translation
    Matrix4x4 T = Matrix4x4::Identity();
    T.M[3][0] = pos.X;
    T.M[3][1] = pos.Y;
    T.M[3][2] = pos.Z;

    // Y-axis rotation (yaw)
    const float s = std::sin(yaw);
    const float c = std::cos(yaw);
    Matrix4x4 R = Matrix4x4::Identity();
    R.M[0][0] =  c;  R.M[2][0] = s;
    R.M[0][2] = -s;  R.M[2][2] = c;

    return T.Multiply(R);
}

} // namespace NF::Game
