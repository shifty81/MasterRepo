#pragma once
/**
 * @file LightBaker.h
 * @brief Offline / background light-map baker for static scene geometry.
 *
 * Computes direct + indirect (1-bounce GI) lighting for static meshes and
 * stores results as lightmap textures.  Uses a simple path-tracing approach
 * on CPU with optional multi-threading.
 *
 * Supports:
 *   - Direct shadows from directional, point, and spot lights
 *   - One-bounce indirect illumination (hemisphere sampling)
 *   - Ambient occlusion baking
 *   - Output: per-object lightmap PNG / EXR
 *   - Progressive baking with real-time preview callback
 *
 * Typical usage:
 * @code
 *   LightBaker baker;
 *   BakeSettings s;
 *   s.resolution = 512;
 *   s.samples    = 64;
 *   baker.Init(s);
 *   baker.AddStaticMesh("floor",  meshDataFloor,  lightBounds);
 *   baker.AddLight(LightType::Directional, {1,1,1}, 1.f);
 *   baker.BakeAsync([](float progress){ printf("%.0f%%\n", progress*100); });
 *   baker.WaitForCompletion();
 *   baker.ExportLightmaps("Lightmaps/");
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class BakeLightType : uint8_t { Directional=0, Point, Spot, Area };

struct BakeLight {
    BakeLightType type{BakeLightType::Directional};
    float         colour[3]{1,1,1};
    float         intensity{1.f};
    float         position[3]{};
    float         direction[3]{0,-1,0};
    float         range{20.f};
    float         spotAngleDeg{45.f};
    bool          castShadows{true};
};

struct BakeMeshEntry {
    std::string   name;
    // Geometry is represented minimally for the baker
    std::vector<float>    vertices;   ///< flat xyz triples
    std::vector<float>    normals;
    std::vector<float>    uvs;        ///< lightmap UV channel
    std::vector<uint32_t> indices;
    float                 worldMatrix[16]{};
};

struct BakeSettings {
    uint32_t resolution{256};
    uint32_t samples{32};
    uint32_t indirectBounces{1};
    bool     bakeAO{true};
    float    aoRadius{0.5f};
    uint32_t threads{4};
    bool     denoiseOutput{false};
    std::string outputFormat{"png"};  ///< "png" or "exr"
};

struct BakeResult {
    std::string          meshName;
    uint32_t             width{0}, height{0};
    std::vector<uint8_t> rgbaPixels;
    bool                 succeeded{false};
};

enum class BakeState : uint8_t { Idle=0, Baking, Finished, Failed };

class LightBaker {
public:
    LightBaker();
    ~LightBaker();

    void Init(const BakeSettings& settings = {});
    void Shutdown();

    void AddStaticMesh(const BakeMeshEntry& mesh);
    void RemoveStaticMesh(const std::string& name);
    void AddLight(const BakeLight& light);
    void ClearLights();
    void ClearMeshes();

    void Bake();                         ///< synchronous
    void BakeAsync(std::function<void(float progress)> onProgress = nullptr);
    void WaitForCompletion();
    void Cancel();

    BakeState GetState()    const;
    float     GetProgress() const;

    const std::vector<BakeResult>& GetResults() const;
    BakeResult GetResult(const std::string& meshName) const;

    bool ExportLightmaps(const std::string& outputDir) const;

    void OnComplete(std::function<void(bool succeeded)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
