#pragma once
/**
 * @file FractalNoise.h
 * @brief Fractal Brownian motion (fBm) and turbulence noise generators.
 *
 * FractalNoise builds layered noise by summing multiple octaves of a base
 * noise function, each scaled by a frequency (lacunarity) and amplitude
 * (persistence/gain) factor:
 *
 *   NoiseType: Value, Perlin, Simplex, Worley (cellular).
 *
 *   FractalParams:
 *     - noiseType, octaves, lacunarity, persistence (gain), scale, seed.
 *
 *   FractalNoise (static API):
 *     - fBm(x, y, params)             → float  [-1, 1]
 *     - fBm(x, y, z, params)          → float  [-1, 1]
 *     - Turbulence(x, y, params)      → float  [ 0, 1]  (abs-sum)
 *     - RidgedMulti(x, y, params)     → float  [ 0, 1]  (ridged variant)
 *     - GenerateMap(w, h, params)     → vector<float> row-major [0,1]
 *     - GenerateNormalMap(w,h,params) → vector<float> RGBA (3-channel normals + height)
 *     - Warp(x, y, params, strength)  → {wx, wy}  (domain-warped coordinate)
 *
 *   All functions are stateless and thread-safe.
 */

#include <cstddef>
#include <cstdint>
#include <vector>
#include <utility>

namespace PCG {

// ── Noise type ────────────────────────────────────────────────────────────
enum class NoiseType : uint8_t { Value, Perlin, Simplex, Worley };

// ── Fractal parameters ────────────────────────────────────────────────────
struct FractalParams {
    NoiseType noiseType{NoiseType::Perlin};
    int       octaves{6};
    float     lacunarity{2.0f};   ///< Frequency multiplier per octave
    float     persistence{0.5f};  ///< Amplitude multiplier per octave (gain)
    float     scale{1.0f};        ///< World-space → noise-space scale
    uint32_t  seed{0};

    float offset[3]{0.0f, 0.0f, 0.0f};  ///< Domain offset (x, y, z)
};

// ── Domain-warp result ────────────────────────────────────────────────────
struct WarpResult { float x{0.0f}; float y{0.0f}; };

// ── FractalNoise ──────────────────────────────────────────────────────────
class FractalNoise {
public:
    FractalNoise()  = delete;
    ~FractalNoise() = delete;

    // ── 2-D variants ──────────────────────────────────────────
    static float fBm(float x, float y, const FractalParams& p);
    static float Turbulence(float x, float y, const FractalParams& p);
    static float RidgedMulti(float x, float y, const FractalParams& p);

    // ── 3-D variants ──────────────────────────────────────────
    static float fBm(float x, float y, float z, const FractalParams& p);
    static float Turbulence(float x, float y, float z, const FractalParams& p);
    static float RidgedMulti(float x, float y, float z, const FractalParams& p);

    // ── map generation ────────────────────────────────────────
    /// Generate a width × height heightmap.  Values normalised to [0, 1].
    static std::vector<float> GenerateMap(size_t width, size_t height,
                                          const FractalParams& p);

    /// Generate an RGBA normal map (nx, ny, nz, height) in [0,1].
    static std::vector<float> GenerateNormalMap(size_t width, size_t height,
                                                const FractalParams& p,
                                                float heightScale = 1.0f);

    // ── domain warp ───────────────────────────────────────────
    /// Warp a 2-D point by evaluating fBm at two offsets.
    static WarpResult Warp(float x, float y, const FractalParams& p, float strength = 1.0f);

private:
    // ── base noise functions (internal) ───────────────────────
    static float BaseNoise2(float x, float y, NoiseType type, uint32_t seed);
    static float BaseNoise3(float x, float y, float z, NoiseType type, uint32_t seed);

    // ── internals ─────────────────────────────────────────────
    static float ValueNoise2(float x, float y, uint32_t seed);
    static float PerlinNoise2(float x, float y, uint32_t seed);
    static float SimplexNoise2(float x, float y, uint32_t seed);
    static float WorleyNoise2(float x, float y, uint32_t seed);

    static float ValueNoise3(float x, float y, float z, uint32_t seed);
    static float PerlinNoise3(float x, float y, float z, uint32_t seed);
    static float SimplexNoise3(float x, float y, float z, uint32_t seed);
    static float WorleyNoise3(float x, float y, float z, uint32_t seed);
};

} // namespace PCG
