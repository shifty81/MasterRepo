#include "PCG/Noise/NoiseGenerator.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace PCG {

// ── NoiseMap helpers ─────────────────────────────────────────────────────────

float NoiseMap::Sample(uint32_t x, uint32_t y) const {
    if (x >= width || y >= height) return 0.0f;
    return data[y * width + x];
}

float NoiseMap::SampleBilinear(float u, float v) const {
    float px = u * static_cast<float>(width  - 1);
    float py = v * static_cast<float>(height - 1);
    int   x0 = static_cast<int>(px);
    int   y0 = static_cast<int>(py);
    int   x1 = std::min(x0 + 1, static_cast<int>(width  - 1));
    int   y1 = std::min(y0 + 1, static_cast<int>(height - 1));
    float fx = px - static_cast<float>(x0);
    float fy = py - static_cast<float>(y0);
    float a  = data[y0 * width + x0];
    float b  = data[y0 * width + x1];
    float c  = data[y1 * width + x0];
    float d  = data[y1 * width + x1];
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}

void NoiseMap::Normalize() {
    if (data.empty()) return;
    float lo = *std::min_element(data.begin(), data.end());
    float hi = *std::max_element(data.begin(), data.end());
    float range = hi - lo;
    if (range < 1e-9f) return;
    for (auto& v : data) v = (v - lo) / range;
    minVal = 0.0f;
    maxVal = 1.0f;
}

void NoiseMap::Remap(float newMin, float newMax) {
    Normalize();
    float range = newMax - newMin;
    for (auto& v : data) v = newMin + v * range;
    minVal = newMin;
    maxVal = newMax;
}

// ── Constructor ───────────────────────────────────────────────────────────────

NoiseGenerator::NoiseGenerator(uint64_t seed) : m_seed(seed) {
    initPerm();
}

void NoiseGenerator::SetSeed(uint64_t seed) {
    m_seed = seed;
    initPerm();
}

uint64_t NoiseGenerator::GetSeed() const { return m_seed; }

// ── Permutation table ─────────────────────────────────────────────────────────

void NoiseGenerator::initPerm() {
    // Fill 0..255
    for (int i = 0; i < 256; ++i) m_perm[i] = i;

    // Knuth shuffle seeded by m_seed
    uint64_t rng = m_seed ^ 0x9e3779b97f4a7c15ULL;
    auto next = [&](int n) -> int {
        rng ^= rng >> 33;
        rng *= 0xff51afd7ed558ccdULL;
        rng ^= rng >> 33;
        rng *= 0xc4ceb9fe1a85ec53ULL;
        rng ^= rng >> 33;
        return static_cast<int>(rng % static_cast<uint64_t>(n));
    };

    for (int i = 255; i > 0; --i) {
        int j = next(i + 1);
        std::swap(m_perm[i], m_perm[j]);
    }
    // Duplicate
    for (int i = 0; i < 256; ++i) m_perm[256 + i] = m_perm[i];
}

// ── Math helpers ──────────────────────────────────────────────────────────────

float NoiseGenerator::fade(float t) const {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float NoiseGenerator::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

float NoiseGenerator::grad(int hash, float x, float y) const {
    // 8 gradient directions in 2D
    switch (hash & 7) {
        case 0: return  x + y;
        case 1: return -x + y;
        case 2: return  x - y;
        case 3: return -x - y;
        case 4: return  x;
        case 5: return -x;
        case 6: return  y;
        case 7: return -y;
        default: return 0.0f;
    }
}

// ── Perlin 2D ────────────────────────────────────────────────────────────────

float NoiseGenerator::perlin2D(float x, float y) const {
    int xi = static_cast<int>(std::floor(x)) & 255;
    int yi = static_cast<int>(std::floor(y)) & 255;
    float xf = x - std::floor(x);
    float yf = y - std::floor(y);
    float u  = fade(xf);
    float v  = fade(yf);

    int aa = m_perm[m_perm[xi]     + yi    ];
    int ab = m_perm[m_perm[xi]     + yi + 1];
    int ba = m_perm[m_perm[xi + 1] + yi    ];
    int bb = m_perm[m_perm[xi + 1] + yi + 1];

    float x1 = lerp(grad(aa, xf,       yf    ),
                    grad(ba, xf - 1.0f, yf    ), u);
    float x2 = lerp(grad(ab, xf,       yf - 1.0f),
                    grad(bb, xf - 1.0f, yf - 1.0f), u);
    return lerp(x1, x2, v);
}

// ── Value noise 2D ────────────────────────────────────────────────────────────

float NoiseGenerator::value2D(float x, float y) const {
    int xi = static_cast<int>(std::floor(x)) & 255;
    int yi = static_cast<int>(std::floor(y)) & 255;
    float xf = x - std::floor(x);
    float yf = y - std::floor(y);
    float u  = fade(xf);
    float v  = fade(yf);

    // Hash corners to values in [-1,1]
    auto hashToVal = [&](int h) -> float {
        return static_cast<float>(m_perm[h & 255]) / 127.5f - 1.0f;
    };
    float v00 = hashToVal(m_perm[xi    ] + yi    );
    float v10 = hashToVal(m_perm[xi + 1] + yi    );
    float v01 = hashToVal(m_perm[xi    ] + yi + 1);
    float v11 = hashToVal(m_perm[xi + 1] + yi + 1);

    float top    = lerp(v00, v10, u);
    float bottom = lerp(v01, v11, u);
    return lerp(top, bottom, v);
}

// ── FBM 2D ────────────────────────────────────────────────────────────────────

float NoiseGenerator::fbm2D(float x, float y, const NoiseParams& p) const {
    float value = 0.0f;
    float amp   = 1.0f;
    float freq  = 1.0f;
    float max   = 0.0f;
    for (int i = 0; i < p.octaves; ++i) {
        value += perlin2D(x * freq, y * freq) * amp;
        max   += amp;
        amp   *= p.persistence;
        freq  *= p.lacunarity;
    }
    return max > 0.0f ? value / max : 0.0f;
}

// ── Worley (cellular) 2D ──────────────────────────────────────────────────────

float NoiseGenerator::worley2D(float x, float y) const {
    int xi = static_cast<int>(std::floor(x));
    int yi = static_cast<int>(std::floor(y));
    float minDist = 1e9f;

    for (int oy = -1; oy <= 1; ++oy) {
        for (int ox = -1; ox <= 1; ++ox) {
            int cx = xi + ox;
            int cy = yi + oy;
            // Pseudo-random feature point in cell [cx, cy].
            int h = m_perm[(m_perm[((cx) & 255)] + ((cy) & 255)) & 255];
            float fx = cx + static_cast<float>(h) / 255.0f;
            int h2 = m_perm[(m_perm[((cx + 1) & 255)] + ((cy + 1) & 255)) & 255];
            float fy = cy + static_cast<float>(h2) / 255.0f;
            float dx = x - fx;
            float dy = y - fy;
            float d  = std::sqrt(dx * dx + dy * dy);
            minDist  = std::min(minDist, d);
        }
    }
    return std::min(minDist, 1.0f);
}

// ── Sample2D dispatch ────────────────────────────────────────────────────────

float NoiseGenerator::Sample2D(float x, float y, const NoiseParams& p) const {
    float freq = p.frequency;
    float amp  = p.amplitude;

    switch (p.type) {
        case NoiseType::White: {
            // Deterministic white noise via hash.
            int xi = static_cast<int>(std::floor(x * freq)) & 255;
            int yi = static_cast<int>(std::floor(y * freq)) & 255;
            return (static_cast<float>(m_perm[m_perm[xi] + yi]) / 255.0f) * amp;
        }
        case NoiseType::Value:
            return value2D(x * freq, y * freq) * amp;

        case NoiseType::Perlin:
            return perlin2D(x * freq, y * freq) * amp;

        case NoiseType::Simplex:
            // Use Perlin as a stand-in (full simplex omitted for brevity).
            return perlin2D(x * freq, y * freq) * amp;

        case NoiseType::Worley:
            return worley2D(x * freq, y * freq) * amp;

        case NoiseType::FBM:
            return fbm2D(x * freq, y * freq, p) * amp;

        case NoiseType::Ridged: {
            float val = 0.0f, a = 1.0f, f = freq, mx = 0.0f;
            for (int i = 0; i < p.octaves; ++i) {
                val += (1.0f - std::abs(perlin2D(x * f, y * f))) * a;
                mx  += a;
                a   *= p.persistence;
                f   *= p.lacunarity;
            }
            return mx > 0.0f ? (val / mx) * amp : 0.0f;
        }

        case NoiseType::Billow: {
            float val = 0.0f, a = 1.0f, f = freq, mx = 0.0f;
            for (int i = 0; i < p.octaves; ++i) {
                val += std::abs(perlin2D(x * f, y * f)) * a;
                mx  += a;
                a   *= p.persistence;
                f   *= p.lacunarity;
            }
            return mx > 0.0f ? (val / mx) * amp : 0.0f;
        }

        case NoiseType::Cellular:
            return worley2D(x * freq, y * freq) * amp;
    }
    return 0.0f;
}

float NoiseGenerator::Sample1D(float x, const NoiseParams& p) const {
    return Sample2D(x, 0.0f, p);
}

float NoiseGenerator::Sample3D(float x, float y, float z, const NoiseParams& p) const {
    // Combine two 2D samples for a 3D approximation.
    return (Sample2D(x, y, p) + Sample2D(y, z, p)) * 0.5f;
}

// ── Map generation ────────────────────────────────────────────────────────────

NoiseMap NoiseGenerator::GenerateMap(uint32_t width, uint32_t height,
                                      const NoiseParams& p) const {
    NoiseMap map;
    map.width  = width;
    map.height = height;
    map.data.resize(static_cast<size_t>(width) * height);

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            float fx = static_cast<float>(x) / static_cast<float>(width);
            float fy = static_cast<float>(y) / static_cast<float>(height);
            map.data[y * width + x] = Sample2D(fx, fy, p);
        }
    }
    map.Normalize();
    return map;
}

NoiseMap NoiseGenerator::GenerateHeightmap(uint32_t width, uint32_t height,
                                            float scale, int octaves) const {
    NoiseParams p;
    p.type        = NoiseType::FBM;
    p.seed        = m_seed;
    p.frequency   = 1.0f / scale;
    p.amplitude   = 1.0f;
    p.octaves     = octaves;
    p.lacunarity  = 2.0f;
    p.persistence = 0.5f;
    return GenerateMap(width, height, p);
}

NoiseMap NoiseGenerator::GenerateWarped(uint32_t width, uint32_t height,
                                         const NoiseParams& p) const {
    NoiseMap map;
    map.width  = width;
    map.height = height;
    map.data.resize(static_cast<size_t>(width) * height);

    NoiseParams warp = p;
    warp.type = NoiseType::Perlin;

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            float fx = static_cast<float>(x) / static_cast<float>(width);
            float fy = static_cast<float>(y) / static_cast<float>(height);
            // Domain warp: offset sample coordinates by another noise field.
            float wx = Sample2D(fx + 1.7f, fy + 9.2f, warp) * p.warpStrength;
            float wy = Sample2D(fx + 8.3f, fy + 2.8f, warp) * p.warpStrength;
            map.data[y * width + x] = Sample2D(fx + wx, fy + wy, p);
        }
    }
    map.Normalize();
    return map;
}

// ── Static utilities ──────────────────────────────────────────────────────────

NoiseMap NoiseGenerator::Blend(const NoiseMap& a, const NoiseMap& b, float t) {
    if (a.width != b.width || a.height != b.height) return a;
    NoiseMap out;
    out.width  = a.width;
    out.height = a.height;
    out.data.resize(a.data.size());
    for (size_t i = 0; i < a.data.size(); ++i)
        out.data[i] = a.data[i] + t * (b.data[i] - a.data[i]);
    out.Normalize();
    return out;
}

NoiseMap NoiseGenerator::Mask(const NoiseMap& base, const NoiseMap& mask) {
    if (base.width != mask.width || base.height != mask.height) return base;
    NoiseMap out;
    out.width  = base.width;
    out.height = base.height;
    out.data.resize(base.data.size());
    for (size_t i = 0; i < base.data.size(); ++i)
        out.data[i] = base.data[i] * mask.data[i];
    out.Normalize();
    return out;
}

} // namespace PCG
