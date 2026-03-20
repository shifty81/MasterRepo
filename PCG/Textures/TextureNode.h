#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace PCG {

// RGBA pixel (float, 0–1 range)
struct PixelF {
    float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
};

// Flat RGBA float image
struct TextureBuffer {
    int32_t width  = 0;
    int32_t height = 0;
    std::vector<PixelF> pixels;

    void Resize(int32_t w, int32_t h);
    PixelF  Get(int32_t x, int32_t y) const;
    void    Set(int32_t x, int32_t y, PixelF p);

    // Convert to packed RGBA8 bytes for export
    std::vector<uint8_t> ToRGBA8() const;
    bool SavePPM(const std::string& path) const;   // simple uncompressed export
};

// ──────────────────────────────────────────────────────────────
// Node types in the texture graph
// ──────────────────────────────────────────────────────────────

enum class TextureNodeType {
    Constant,       // solid colour
    Noise,          // value / perlin-like noise
    Gradient,       // linear gradient
    Blend,          // lerp between two inputs
    Multiply,       // pixel-wise multiply
    Add,            // pixel-wise add
    Invert,         // 1-x per channel
    Threshold,      // binarise at a cutoff
    Tile,           // repeat / mirror input
    Output          // final output node
};

struct TextureNodePin {
    uint32_t    nodeID = 0;
    std::string name;
};

struct TextureNode {
    uint32_t        id    = 0;
    TextureNodeType type  = TextureNodeType::Constant;
    std::string     label;

    // parameters (interpreted per node type)
    PixelF      color    = {1,1,1,1};    // Constant / Gradient start
    PixelF      color2   = {0,0,0,1};   // Gradient end
    float       frequency = 4.0f;        // Noise frequency
    float       amplitude = 1.0f;        // Noise amplitude
    uint64_t    seed      = 0;
    float       threshold = 0.5f;
    int32_t     tileX     = 2;
    int32_t     tileY     = 2;
    float       blendT    = 0.5f;        // Blend mix factor

    std::vector<TextureNodePin> inputs;  // connected input pins
};

// ──────────────────────────────────────────────────────────────
// Node graph evaluator
// ──────────────────────────────────────────────────────────────

class TextureGraph {
public:
    uint32_t AddNode(TextureNode node);
    bool     Connect(uint32_t srcID, const std::string& srcPinName,
                     uint32_t dstID, const std::string& dstPinName);
    void     SetOutputNode(uint32_t id);

    // Evaluate the graph at the given resolution; returns the output buffer
    TextureBuffer Evaluate(int32_t width, int32_t height) const;

    // Convenience: generate individual texture types directly
    static TextureBuffer GenerateNoise(int32_t w, int32_t h, uint64_t seed,
                                       float frequency, float amplitude);
    static TextureBuffer GenerateGradient(int32_t w, int32_t h,
                                          PixelF start, PixelF end,
                                          bool vertical = true);
    static TextureBuffer GenerateCheckerboard(int32_t w, int32_t h,
                                               int32_t tilesX, int32_t tilesY,
                                               PixelF colorA, PixelF colorB);
    static TextureBuffer GenerateVoronoi(int32_t w, int32_t h, uint64_t seed,
                                          int32_t numPoints);

    TextureNode* GetNode(uint32_t id);
    size_t       NodeCount() const { return m_nodes.size(); }

private:
    uint32_t                m_nextID    = 1;
    uint32_t                m_outputID  = 0;
    std::vector<TextureNode> m_nodes;

    TextureBuffer evaluateNode(uint32_t nodeID, int32_t w, int32_t h) const;
    const TextureNode* findNode(uint32_t id) const;

    static float valueNoise(float x, float y, uint64_t seed);
};

} // namespace PCG
