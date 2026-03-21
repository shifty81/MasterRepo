#pragma once
// RenderPipeline — Advanced rendering pipeline extension for Phase Blueprint
//
// Extends Engine::Render::Renderer with:
//   • Level-of-Detail (LOD) system
//   • Global Illumination (GI) probe grid
//   • Ray-tracing / screen-space reflections stub
//   • Volumetric fog / lighting
//   • Particle system
//
// All GPU heavy-lifting is delegated to the underlying renderer backend.
// This header provides the CPU-side pipeline configuration and draw-list
// management.  It is intentionally backend-agnostic (OpenGL / Vulkan stubs).

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <array>

namespace Engine::Render {

// ─────────────────────────────────────────────────────────────────────────────
// Level-of-Detail (LOD)
// ─────────────────────────────────────────────────────────────────────────────

struct LODLevel {
    float      minDistance = 0.f;   // metres from camera
    float      maxDistance = 100.f;
    std::string meshId;              // mesh handle at this detail level
    float      screenSizeFraction = 1.0f; // 0..1 fraction of screen height
};

struct LODGroup {
    std::string          id;
    std::vector<LODLevel> levels;    // sorted by minDistance ascending
};

class LODSystem {
public:
    void RegisterGroup(const LODGroup& group);
    void UnregisterGroup(const std::string& id);

    /// Given camera position and instance world position, returns the
    /// best-fitting LOD level index for a group.
    int SelectLevel(const std::string& groupId,
                    float cameraDistanceSq) const;

    void SetBias(float bias); // > 1 = prefer lower detail (performance)
    float GetBias() const;

    size_t GroupCount() const;

private:
    std::vector<LODGroup> m_groups;
    float                 m_bias = 1.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
// Global Illumination (probe grid)
// ─────────────────────────────────────────────────────────────────────────────

struct GIProbe {
    std::array<float,3> position = {};
    float               radius   = 5.f;
    bool                dynamic  = false; // re-baked each frame when true
    uint32_t            texId    = 0;     // GPU irradiance cubemap handle
};

struct GIConfig {
    int   probeGridX = 8;
    int   probeGridY = 4;
    int   probeGridZ = 8;
    float probeSpacing = 4.f;       // metres between probes
    bool  enabled     = true;
    float intensity   = 1.0f;
    bool  useIrradiance = true;
    bool  useReflection = true;
};

class GISystem {
public:
    explicit GISystem(const GIConfig& cfg = {});

    void Init();
    void Shutdown();

    /// Bake static probes (expensive, call once or on scene change).
    void BakeProbes();

    /// Update dynamic probes (call each frame).
    void UpdateDynamic();

    void AddProbe(const GIProbe& probe);
    void RemoveProbe(uint32_t texId);
    const std::vector<GIProbe>& GetProbes() const;

    void SetConfig(const GIConfig& cfg);
    const GIConfig& GetConfig() const;

    bool IsEnabled() const;
    void SetEnabled(bool v);

private:
    GIConfig            m_cfg;
    std::vector<GIProbe> m_probes;
    bool                m_initialised = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// Ray-tracing / Screen-Space Reflections (SSR)
// ─────────────────────────────────────────────────────────────────────────────

enum class ReflectionMode {
    None,
    ScreenSpace,    // SSR — fast, GPU-computed, limited to on-screen geometry
    RayTraced,      // Hardware RT — requires DXR / VK_KHR_ray_tracing (stub)
};

struct ReflectionConfig {
    ReflectionMode mode         = ReflectionMode::ScreenSpace;
    int            maxSteps     = 64;       // SSR ray march steps
    float          maxDistance  = 50.f;     // max SSR ray length (metres)
    float          thickness    = 0.1f;     // SSR depth-test thickness
    float          intensity    = 1.0f;
    bool           blurFalloff  = true;
    int            rtSamples    = 1;        // RT samples per pixel
};

class ReflectionSystem {
public:
    explicit ReflectionSystem(const ReflectionConfig& cfg = {});

    void Init();
    void Shutdown();
    void Render(uint32_t depthTexId, uint32_t normalTexId, uint32_t colorTexId);

    void SetConfig(const ReflectionConfig& cfg);
    const ReflectionConfig& GetConfig() const;

    bool SupportsRayTracing() const;

private:
    ReflectionConfig m_cfg;
};

// ─────────────────────────────────────────────────────────────────────────────
// Volumetric fog / lighting
// ─────────────────────────────────────────────────────────────────────────────

struct VolumetricConfig {
    bool  enabled        = false;
    float density        = 0.02f;   // fog density coefficient
    float scattering     = 0.5f;    // in-scattering amount
    float absorption     = 0.1f;
    int   numSlices      = 64;      // froxel Z slices
    int   froxelW        = 160;
    int   froxelH        = 90;
    std::array<float,3> albedo     = {0.8f, 0.8f, 0.9f};
    float               ambientMix = 0.3f;
};

class VolumetricSystem {
public:
    explicit VolumetricSystem(const VolumetricConfig& cfg = {});

    void Init();
    void Shutdown();
    void Render();   // inject into current light pass

    void SetConfig(const VolumetricConfig& cfg);
    const VolumetricConfig& GetConfig() const;

private:
    VolumetricConfig m_cfg;
    uint32_t         m_froxelTex = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Particle system
// ─────────────────────────────────────────────────────────────────────────────

struct ParticleEmitterDesc {
    std::string  id;
    std::string  materialId;

    uint32_t     maxParticles = 1000;
    float        emitRate     = 50.f;   // particles/second
    float        lifetime     = 2.f;    // seconds
    float        lifetimeVar  = 0.5f;   // +/- randomisation

    std::array<float,3> position   = {};
    std::array<float,3> velocity   = {0.f, 1.f, 0.f};
    std::array<float,3> velocityVar = {0.5f, 0.5f, 0.5f};
    std::array<float,3> gravity    = {0.f,-9.8f, 0.f};

    float  startSize    = 0.1f;
    float  endSize      = 0.f;
    float  startAlpha   = 1.f;
    float  endAlpha     = 0.f;
    std::array<float,4> startColor = {1,1,1,1};
    std::array<float,4> endColor   = {1,1,1,0};

    bool  looping   = true;
    bool  localSpace = false;
    bool  gpuSim    = false;    // GPU compute simulation (stub)
};

struct Particle {
    std::array<float,3> position;
    std::array<float,3> velocity;
    float age   = 0.f;
    float life  = 0.f;
    float size  = 0.f;
    float alpha = 1.f;
    std::array<float,4> color;
    bool  alive = false;
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    uint32_t    CreateEmitter(const ParticleEmitterDesc& desc);
    void        DestroyEmitter(uint32_t id);
    ParticleEmitterDesc* GetEmitter(uint32_t id);

    void Update(float dt);
    void Render();      // submits billboard quads to Renderer

    size_t TotalAlive() const;
    void   SetGlobalGravity(const std::array<float,3>& g);

private:
    struct EmitterState {
        ParticleEmitterDesc     desc;
        std::vector<Particle>   particles;
        float                   emitAccum = 0.f;
        uint32_t                id = 0;
    };

    std::vector<EmitterState>   m_emitters;
    uint32_t                    m_nextId = 1;
    std::array<float,3>         m_globalGravity = {0.f,-9.8f,0.f};
};

// ─────────────────────────────────────────────────────────────────────────────
// RenderPipeline — master pipeline controller
// ─────────────────────────────────────────────────────────────────────────────

struct RenderPipelineConfig {
    GIConfig          gi;
    ReflectionConfig  reflections;
    VolumetricConfig  volumetrics;
    bool              enableLOD       = true;
    bool              enableParticles = true;
};

class RenderPipeline {
public:
    explicit RenderPipeline(const RenderPipelineConfig& cfg = {});
    ~RenderPipeline();

    void Init();
    void Shutdown();

    void Update(float dt);
    void BeginFrame();
    void Render();
    void EndFrame();

    LODSystem&        GetLOD();
    GISystem&         GetGI();
    ReflectionSystem& GetReflections();
    VolumetricSystem& GetVolumetrics();
    ParticleSystem&   GetParticles();

    const RenderPipelineConfig& GetConfig() const;
    void SetConfig(const RenderPipelineConfig& cfg);

private:
    RenderPipelineConfig m_cfg;
    LODSystem            m_lod;
    GISystem             m_gi;
    ReflectionSystem     m_reflections;
    VolumetricSystem     m_volumetrics;
    ParticleSystem       m_particles;
    bool                 m_initialised = false;
};

} // namespace Engine::Render
