#pragma once
/**
 * @file TextureGenerator.h
 * @brief Procedural texture generation using stacked noise and pattern layers.
 *
 * TextureGenerator builds pixel data by evaluating a stack of named layers
 * and blending them together:
 *
 *   LayerType:  Perlin, Simplex, Worley (cellular), Gradient, Checkerboard,
 *               Bricks, FBM (fractional Brownian motion).
 *   BlendMode:  Normal, Add, Multiply, Screen, Overlay, Subtract.
 *
 *   TextureLayer:
 *     - type, blendMode, opacity (0..1)
 *     - scale (UV tiling), offset, rotation (degrees)
 *     - octaves / lacunarity / persistence (noise layers)
 *     - colourA / colourB (gradient endpoints or pattern colours, RGBA)
 *
 *   TextureGenConfig:
 *     - width, height (pixels)
 *     - format: R8, RG8, RGBA8, RF32, RGBAF32
 *     - seed (uint32_t)
 *
 *   GeneratedTexture:
 *     - raw pixel bytes, width, height, format, channels, bytesPerPixel
 *
 *   TextureGenerator:
 *     - AddLayer / RemoveLayer / MoveLayer / ClearLayers
 *     - SetConfig / Generate → GeneratedTexture
 *     - GenerateAsync(callback)
 *     - TextureGenStats: layerCount, lastGenTimeMs, pixelsGenerated
 */

#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <functional>

namespace PCG {

// ── Texel format ──────────────────────────────────────────────────────────
enum class TexelFormat : uint8_t { R8, RG8, RGBA8, RF32, RGBAF32 };
uint8_t TexelChannels(TexelFormat f);
uint8_t TexelBytesPerPixel(TexelFormat f);

// ── Layer type ────────────────────────────────────────────────────────────
enum class LayerType : uint8_t {
    Perlin, Simplex, Worley, Gradient, Checkerboard, Bricks, FBM
};

// ── Blend mode ────────────────────────────────────────────────────────────
enum class BlendMode : uint8_t {
    Normal, Add, Multiply, Screen, Overlay, Subtract
};

// ── Texture layer ─────────────────────────────────────────────────────────
struct TextureLayer {
    LayerType             type{LayerType::Perlin};
    BlendMode             blendMode{BlendMode::Normal};
    float                 opacity{1.0f};
    float                 scale{4.0f};           ///< UV tiling
    float                 offsetU{0.0f};
    float                 offsetV{0.0f};
    float                 rotation{0.0f};        ///< Degrees
    int                   octaves{4};
    float                 lacunarity{2.0f};
    float                 persistence{0.5f};
    std::array<float, 4>  colourA{0.0f, 0.0f, 0.0f, 1.0f}; ///< RGBA [0..1]
    std::array<float, 4>  colourB{1.0f, 1.0f, 1.0f, 1.0f};
    std::string           name;
};

// ── Config ────────────────────────────────────────────────────────────────
struct TextureGenConfig {
    uint32_t    width{256};
    uint32_t    height{256};
    TexelFormat format{TexelFormat::RGBA8};
    uint32_t    seed{12345};
};

// ── Output texture ────────────────────────────────────────────────────────
struct GeneratedTexture {
    std::vector<uint8_t> data;       ///< Raw pixel bytes
    uint32_t             width{0};
    uint32_t             height{0};
    TexelFormat          format{TexelFormat::RGBA8};
    uint8_t              channels{4};
    uint8_t              bytesPerPixel{4};

    bool IsValid() const { return !data.empty() && width > 0 && height > 0; }
};

// ── Stats ─────────────────────────────────────────────────────────────────
struct TextureGenStats {
    uint32_t layerCount{0};
    double   lastGenTimeMs{0.0};
    uint64_t pixelsGenerated{0};
};

// ── Callback ──────────────────────────────────────────────────────────────
using TextureGenCb = std::function<void(GeneratedTexture)>;

// ── TextureGenerator ──────────────────────────────────────────────────────
class TextureGenerator {
public:
    TextureGenerator();
    explicit TextureGenerator(const TextureGenConfig& cfg);
    ~TextureGenerator();

    TextureGenerator(const TextureGenerator&) = delete;
    TextureGenerator& operator=(const TextureGenerator&) = delete;

    // ── configuration ─────────────────────────────────────────
    void SetConfig(const TextureGenConfig& cfg);
    const TextureGenConfig& Config() const;

    // ── layer management ──────────────────────────────────────
    int  AddLayer(const TextureLayer& layer);        ///< Returns layer index
    bool RemoveLayer(int index);
    bool MoveLayer(int fromIndex, int toIndex);
    bool UpdateLayer(int index, const TextureLayer& layer);
    const TextureLayer* GetLayer(int index) const;
    int  LayerCount() const;
    void ClearLayers();

    // ── generation ────────────────────────────────────────────
    /// Synchronous generation — returns the generated texture.
    GeneratedTexture Generate();

    /// Asynchronous generation — callback fired on a background thread.
    void GenerateAsync(TextureGenCb callback);

    // ── stats ─────────────────────────────────────────────────
    TextureGenStats Stats() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
