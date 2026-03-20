#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <functional>

namespace PCG {

enum class NoiseType {
    White,
    Value,
    Perlin,
    Simplex,
    Worley,
    FBM,
    Ridged,
    Billow,
    Cellular,
};

struct NoiseParams {
    NoiseType   type        = NoiseType::Perlin;
    uint64_t    seed        = 12345;
    float       frequency   = 1.0f;
    float       amplitude   = 1.0f;
    int         octaves     = 4;
    float       lacunarity  = 2.0f;
    float       persistence = 0.5f;
    float       warpStrength = 0.0f;
};

struct NoiseMap {
    std::vector<float> data;
    uint32_t width  = 0;
    uint32_t height = 0;
    float    minVal = 0.0f;
    float    maxVal = 1.0f;

    float Sample(uint32_t x, uint32_t y) const;
    float SampleBilinear(float u, float v) const;
    void  Normalize();
    void  Remap(float newMin, float newMax);
};

class NoiseGenerator {
public:
    explicit NoiseGenerator(uint64_t seed = 0);

    float Sample1D(float x, const NoiseParams& p) const;
    float Sample2D(float x, float y, const NoiseParams& p) const;
    float Sample3D(float x, float y, float z, const NoiseParams& p) const;

    NoiseMap GenerateMap(uint32_t width, uint32_t height, const NoiseParams& p) const;
    NoiseMap GenerateHeightmap(uint32_t width, uint32_t height,
                               float scale = 100.0f, int octaves = 6) const;
    NoiseMap GenerateWarped(uint32_t width, uint32_t height, const NoiseParams& p) const;

    static NoiseMap Blend(const NoiseMap& a, const NoiseMap& b, float t);
    static NoiseMap Mask(const NoiseMap& base, const NoiseMap& mask);

    void     SetSeed(uint64_t seed);
    uint64_t GetSeed() const;

private:
    float perlin2D(float x, float y) const;
    float value2D(float x, float y) const;
    float fbm2D(float x, float y, const NoiseParams& p) const;
    float worley2D(float x, float y) const;
    float fade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int hash, float x, float y) const;

    uint64_t m_seed;
    int      m_perm[512];

    void initPerm();
};

} // namespace PCG
