#include "PCG/Textures/TextureGenerator/TextureGenerator.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <thread>
#include <stdexcept>

namespace PCG {

// ── Format helpers ────────────────────────────────────────────────────────
uint8_t TexelChannels(TexelFormat f) {
    switch (f) {
    case TexelFormat::R8:     return 1;
    case TexelFormat::RG8:    return 2;
    case TexelFormat::RGBA8:  return 4;
    case TexelFormat::RF32:   return 1;
    case TexelFormat::RGBAF32:return 4;
    }
    return 4;
}
uint8_t TexelBytesPerPixel(TexelFormat f) {
    switch (f) {
    case TexelFormat::R8:     return 1;
    case TexelFormat::RG8:    return 2;
    case TexelFormat::RGBA8:  return 4;
    case TexelFormat::RF32:   return 4;
    case TexelFormat::RGBAF32:return 16;
    }
    return 4;
}

// ── Simple value-noise helpers ────────────────────────────────────────────
namespace detail {

// Hash for noise
static uint32_t hash(uint32_t x, uint32_t y, uint32_t seed) {
    uint32_t h = seed ^ (x * 1664525u + y * 22695477u);
    h ^= h >> 13; h *= 0xbf2a2e0bu; h ^= h >> 15;
    return h;
}

static float hashF(int x, int y, uint32_t seed) {
    return (hash(static_cast<uint32_t>(x), static_cast<uint32_t>(y), seed) & 0xFFFF) / 65535.0f;
}

static float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
static float lerp(float a, float b, float t) { return a + t * (b - a); }

// Value noise (stand-in for Perlin/Simplex)
static float valueNoise(float u, float v, uint32_t seed) {
    int ix = static_cast<int>(std::floor(u));
    int iy = static_cast<int>(std::floor(v));
    float fx = u - std::floor(u);
    float fy = v - std::floor(v);
    float ux = fade(fx), uy = fade(fy);
    float v00 = hashF(ix,   iy,   seed);
    float v10 = hashF(ix+1, iy,   seed);
    float v01 = hashF(ix,   iy+1, seed);
    float v11 = hashF(ix+1, iy+1, seed);
    return lerp(lerp(v00, v10, ux), lerp(v01, v11, ux), uy);
}

static float fbmNoise(float u, float v, uint32_t seed, int octaves, float lac, float pers) {
    float val = 0.0f, amp = 1.0f, freq = 1.0f, max = 0.0f;
    for (int o = 0; o < octaves; ++o) {
        val += valueNoise(u * freq, v * freq, seed + static_cast<uint32_t>(o * 1000)) * amp;
        max += amp; amp *= pers; freq *= lac;
    }
    return max > 0 ? val / max : 0;
}

static float worleyNoise(float u, float v, uint32_t seed) {
    int cx = static_cast<int>(std::floor(u));
    int cy = static_cast<int>(std::floor(v));
    float minDist = 1e9f;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            float px = (cx + dx) + hashF(cx+dx, cy+dy, seed);
            float py = (cy + dy) + hashF(cx+dx, cy+dy, seed ^ 0xABCD1234u);
            float d = std::sqrt((u-px)*(u-px) + (v-py)*(v-py));
            if (d < minDist) minDist = d;
        }
    }
    return std::min(minDist, 1.0f);
}

// Blend two RGBA values
static std::array<float,4> blend(const std::array<float,4>& base,
                                  const std::array<float,4>& layer,
                                  BlendMode mode, float opacity)
{
    auto clamp01 = [](float x){ return x < 0 ? 0.0f : (x > 1 ? 1.0f : x); };
    std::array<float,4> out{};
    for (int c = 0; c < 4; ++c) {
        float b = base[c], l = layer[c], r = 0;
        switch (mode) {
        case BlendMode::Normal:   r = l; break;
        case BlendMode::Add:      r = b + l; break;
        case BlendMode::Multiply: r = b * l; break;
        case BlendMode::Screen:   r = 1 - (1-b)*(1-l); break;
        case BlendMode::Overlay:  r = b < 0.5f ? 2*b*l : 1 - 2*(1-b)*(1-l); break;
        case BlendMode::Subtract: r = b - l; break;
        }
        out[c] = clamp01(b * (1-opacity) + r * opacity);
    }
    return out;
}

} // namespace detail

// ── Impl ──────────────────────────────────────────────────────────────────
struct TextureGenerator::Impl {
    TextureGenConfig      cfg;
    std::vector<TextureLayer> layers;
    TextureGenStats       stats;

    std::array<float,4> evalLayer(const TextureLayer& L, float u, float v, uint32_t seed) const {
        float angle = L.rotation * (3.14159265f / 180.0f);
        float cu = std::cos(angle), su = std::sin(angle);
        float ru = cu*u - su*v + L.offsetU;
        float rv = su*u + cu*v + L.offsetV;
        ru *= L.scale; rv *= L.scale;

        float n = 0;
        switch (L.type) {
        case LayerType::Perlin:
        case LayerType::Simplex:
            n = detail::valueNoise(ru, rv, seed);
            break;
        case LayerType::FBM:
            n = detail::fbmNoise(ru, rv, seed, L.octaves, L.lacunarity, L.persistence);
            break;
        case LayerType::Worley:
            n = detail::worleyNoise(ru, rv, seed);
            break;
        case LayerType::Gradient:
            n = std::fmod(std::abs(ru + rv), 1.0f);
            break;
        case LayerType::Checkerboard:
            n = (static_cast<int>(std::floor(ru)) + static_cast<int>(std::floor(rv))) % 2 == 0 ? 0.0f : 1.0f;
            break;
        case LayerType::Bricks: {
            float brow = std::floor(rv);
            float bu = ru + (static_cast<int>(brow) % 2 == 0 ? 0.5f : 0.0f);
            float fx = bu - std::floor(bu), fy = rv - brow;
            n = (fx < 0.05f || fy < 0.05f) ? 0.0f : 1.0f;
            break;
        }
        }
        // Lerp between colourA and colourB using n
        std::array<float,4> c{};
        for (int i = 0; i < 4; ++i)
            c[i] = L.colourA[i] + n * (L.colourB[i] - L.colourA[i]);
        return c;
    }

    GeneratedTexture build() {
        auto t0 = std::chrono::high_resolution_clock::now();

        uint8_t bpp = TexelBytesPerPixel(cfg.format);
        uint8_t ch  = TexelChannels(cfg.format);
        GeneratedTexture out;
        out.width  = cfg.width;
        out.height = cfg.height;
        out.format = cfg.format;
        out.channels = ch;
        out.bytesPerPixel = bpp;
        out.data.resize(static_cast<size_t>(cfg.width) * cfg.height * bpp, 0);

        for (uint32_t py = 0; py < cfg.height; ++py) {
            for (uint32_t px = 0; px < cfg.width; ++px) {
                float u = (px + 0.5f) / cfg.width;
                float v = (py + 0.5f) / cfg.height;
                std::array<float,4> pixel{0,0,0,1};
                for (size_t li = 0; li < layers.size(); ++li) {
                    const auto& L = layers[li];
                    auto lc = evalLayer(L, u, v, cfg.seed + static_cast<uint32_t>(li * 7919));
                    pixel = detail::blend(pixel, lc, L.blendMode, L.opacity);
                }
                size_t idx = (static_cast<size_t>(py) * cfg.width + px) * bpp;
                if (cfg.format == TexelFormat::RGBA8 || cfg.format == TexelFormat::RG8 || cfg.format == TexelFormat::R8) {
                    for (uint8_t c2 = 0; c2 < ch && c2 < 4; ++c2) {
                        float val = pixel[c2] < 0 ? 0 : (pixel[c2] > 1 ? 1 : pixel[c2]);
                        out.data[idx + c2] = static_cast<uint8_t>(val * 255.0f);
                    }
                } else if (cfg.format == TexelFormat::RF32) {
                    float val = pixel[0];
                    std::memcpy(&out.data[idx], &val, 4);
                } else { // RGBAF32
                    for (uint8_t c2 = 0; c2 < 4; ++c2)
                        std::memcpy(&out.data[idx + c2*4], &pixel[c2], 4);
                }
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        stats.lastGenTimeMs = std::chrono::duration<double, std::milli>(t1-t0).count();
        stats.pixelsGenerated += cfg.width * cfg.height;
        stats.layerCount = static_cast<uint32_t>(layers.size());
        return out;
    }
};

// ── Lifecycle ─────────────────────────────────────────────────────────────
TextureGenerator::TextureGenerator() : m_impl(new Impl()) {}
TextureGenerator::TextureGenerator(const TextureGenConfig& cfg) : m_impl(new Impl()) {
    m_impl->cfg = cfg;
}
TextureGenerator::~TextureGenerator() { delete m_impl; }

void TextureGenerator::SetConfig(const TextureGenConfig& cfg) { m_impl->cfg = cfg; }
const TextureGenConfig& TextureGenerator::Config() const { return m_impl->cfg; }

// ── Layer management ──────────────────────────────────────────────────────
int TextureGenerator::AddLayer(const TextureLayer& layer) {
    m_impl->layers.push_back(layer);
    return static_cast<int>(m_impl->layers.size()) - 1;
}
bool TextureGenerator::RemoveLayer(int index) {
    if (index < 0 || index >= static_cast<int>(m_impl->layers.size())) return false;
    m_impl->layers.erase(m_impl->layers.begin() + index);
    return true;
}
bool TextureGenerator::MoveLayer(int from, int to) {
    int n = static_cast<int>(m_impl->layers.size());
    if (from < 0 || from >= n || to < 0 || to >= n) return false;
    auto layer = m_impl->layers[from];
    m_impl->layers.erase(m_impl->layers.begin() + from);
    m_impl->layers.insert(m_impl->layers.begin() + to, layer);
    return true;
}
bool TextureGenerator::UpdateLayer(int index, const TextureLayer& layer) {
    if (index < 0 || index >= static_cast<int>(m_impl->layers.size())) return false;
    m_impl->layers[index] = layer;
    return true;
}
const TextureLayer* TextureGenerator::GetLayer(int index) const {
    if (index < 0 || index >= static_cast<int>(m_impl->layers.size())) return nullptr;
    return &m_impl->layers[index];
}
int  TextureGenerator::LayerCount() const { return static_cast<int>(m_impl->layers.size()); }
void TextureGenerator::ClearLayers()      { m_impl->layers.clear(); }

// ── Generation ────────────────────────────────────────────────────────────
GeneratedTexture TextureGenerator::Generate() { return m_impl->build(); }

void TextureGenerator::GenerateAsync(TextureGenCb callback) {
    // Copy impl state for the background thread
    Impl copy = *m_impl;
    std::thread([copy = std::move(copy), cb = std::move(callback)]() mutable {
        cb(const_cast<Impl&>(copy).build());
    }).detach();
}

// ── Stats ─────────────────────────────────────────────────────────────────
TextureGenStats TextureGenerator::Stats() const {
    m_impl->stats.layerCount = static_cast<uint32_t>(m_impl->layers.size());
    return m_impl->stats;
}

} // namespace PCG
