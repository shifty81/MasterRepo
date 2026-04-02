#include "Game/Voxel/ChunkMeshCache.h"
#include "Core/Logging/Log.h"
#include <string>

namespace NF::Game {

// ---------------------------------------------------------------------------
// Voxel GLSL shaders — directional lighting with per-vertex colour
// ---------------------------------------------------------------------------

static const char* kVoxelVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uModel;

out vec3 vNormal;
out vec3 vWorldPos;
out float vTypeId;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    gl_Position   = uProjection * uView * worldPos;
    vNormal       = aNormal;
    vWorldPos     = worldPos.xyz;
    vTypeId       = aTexCoord.x;
}
)";

static const char* kVoxelFragSrc = R"(
#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
in float vTypeId;

out vec4 FragColor;

uniform vec3 uLightDir;       // normalised world-space direction toward light
uniform vec3 uLightColor;     // directional light colour
uniform vec3 uAmbientColor;   // ambient fill colour
uniform vec3 uViewPos;        // camera world position (for specular)

// Simple Phong-ish lighting palette for voxel types 0..7.
vec3 getPalette(int id) {
    // Air=0, Stone=1, Ore=2, Dirt=3, Rock=4, Metal=5, Ice=6, Organic=7
    if (id == 1) return vec3(0.55, 0.55, 0.55);  // Stone   — grey
    if (id == 2) return vec3(0.85, 0.65, 0.25);  // Ore     — gold
    if (id == 3) return vec3(0.45, 0.30, 0.15);  // Dirt    — brown
    if (id == 4) return vec3(0.40, 0.40, 0.42);  // Rock    — dark grey
    if (id == 5) return vec3(0.70, 0.72, 0.75);  // Metal   — silver
    if (id == 6) return vec3(0.70, 0.85, 0.95);  // Ice     — light blue
    if (id == 7) return vec3(0.30, 0.55, 0.20);  // Organic — green
    return vec3(1.0, 0.0, 1.0);                   // Unknown — magenta
}

void main() {
    int typeId = int(vTypeId + 0.5);  // round to nearest integer
    vec3 baseColor = getPalette(typeId);

    vec3 normal = normalize(vNormal);

    // Diffuse (Lambert)
    float diff = max(dot(normal, uLightDir), 0.0);
    vec3 diffuse = diff * uLightColor;

    // Specular (Blinn-Phong) — subtle for blocky voxels
    vec3 viewDir = normalize(uViewPos - vWorldPos);
    vec3 halfDir = normalize(uLightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = spec * uLightColor * 0.15;

    vec3 color = (uAmbientColor + diffuse + specular) * baseColor;

    FragColor = vec4(color, 1.0);
}
)";

// ---------------------------------------------------------------------------
// Init / Shutdown
// ---------------------------------------------------------------------------

void ChunkMeshCache::Init(ForwardRenderer* forwardRenderer)
{
    m_Renderer = forwardRenderer;

    // Compile the voxel shader.
    if (m_Shader.LoadFromSource(kVoxelVertSrc, kVoxelFragSrc))
        NF::Logger::Log(NF::LogLevel::Info, "ChunkMeshCache", "Voxel shader compiled");
    else
        NF::Logger::Log(NF::LogLevel::Warning, "ChunkMeshCache", "Voxel shader compile failed (may be stub renderer)");

    // Set up the material with default lighting values.
    m_Material.SetShader(&m_Shader);
    m_Material.SetVec3("uLightDir",     NF::Vector3{0.4f, 0.8f, 0.3f}.Normalized());  // sun from upper-right
    m_Material.SetVec3("uLightColor",   {1.0f, 0.95f, 0.85f});
    m_Material.SetVec3("uAmbientColor", {0.15f, 0.17f, 0.20f});
    m_Material.SetVec3("uViewPos",      {0.f, 0.f, 0.f});

    NF::Logger::Log(NF::LogLevel::Info, "ChunkMeshCache", "Initialised");
}

void ChunkMeshCache::Shutdown()
{
    m_Meshes.clear();
    NF::Logger::Log(NF::LogLevel::Info, "ChunkMeshCache", "Shut down");
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void ChunkMeshCache::RebuildDirty(ChunkMap& map)
{
    auto dirtyChunks = map.GetDirtyChunks();
    if (dirtyChunks.empty()) return;

    for (Chunk* chunk : dirtyChunks) {
        if (!chunk) continue;

        const ChunkCoord coord = chunk->GetCoord();

        // Generate mesh data on the CPU.
        MeshData meshData = m_Mesher.Generate(*chunk);
        chunk->ClearDirty();

        if (meshData.Indices.empty()) {
            // No visible geometry — evict from cache.
            m_Meshes.erase(coord);
            continue;
        }

        // Upload to GPU.
        auto& cached = m_Meshes[coord];
        if (!cached.GpuMesh)
            cached.GpuMesh = std::make_unique<Mesh>();
        cached.GpuMesh->Upload(meshData);
        cached.TriCount = static_cast<int>(meshData.Indices.size()) / 3;
    }

    NF::Logger::Log(NF::LogLevel::Debug, "ChunkMeshCache",
        "Rebuilt " + std::to_string(dirtyChunks.size()) + " chunk mesh(es)");
}

void ChunkMeshCache::Render()
{
    if (!m_Renderer) return;

    for (auto& [coord, cached] : m_Meshes) {
        if (!cached.GpuMesh) continue;
        // Chunk meshes are already in world space (origin baked into vertices).
        m_Renderer->Submit(*cached.GpuMesh, m_Material, Matrix4x4::Identity());
    }
}

void ChunkMeshCache::Evict(const ChunkCoord& coord)
{
    m_Meshes.erase(coord);
}

void ChunkMeshCache::Clear()
{
    m_Meshes.clear();
}

int ChunkMeshCache::TotalTriangles() const noexcept
{
    int total = 0;
    for (const auto& [_, cached] : m_Meshes)
        total += cached.TriCount;
    return total;
}

} // namespace NF::Game
