#include "PCG/Noise/FractalNoise/FractalNoise.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>

namespace PCG {

// ── Internal math helpers ─────────────────────────────────────────────────
static inline float lerp(float a, float b, float t) { return a + t * (b - a); }
static inline float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
static inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

// ── Hash helpers ──────────────────────────────────────────────────────────
static uint32_t Hash(int x, int y, uint32_t seed) {
    uint32_t h = seed ^ (static_cast<uint32_t>(x) * 1664525u);
    h ^= (static_cast<uint32_t>(y) * 214013u);
    h ^= (h >> 16); h *= 0x45d9f3b; h ^= (h >> 16);
    return h;
}
static uint32_t Hash(int x, int y, int z, uint32_t seed) {
    return Hash(x ^ (Hash(y, z, seed)), 0, seed);
}
static float Rand01(int x, int y, uint32_t seed) {
    return (Hash(x, y, seed) & 0xFFFFu) / 65535.0f;
}
static float Rand01(int x, int y, int z, uint32_t seed) {
    return (Hash(x, y, z, seed) & 0xFFFFu) / 65535.0f;
}

// ── Gradient helper ───────────────────────────────────────────────────────
static float Grad2(uint32_t hash, float dx, float dy) {
    switch (hash & 3) {
        case 0:  return  dx + dy;
        case 1:  return -dx + dy;
        case 2:  return  dx - dy;
        default: return -dx - dy;
    }
}
static float Grad3(uint32_t hash, float dx, float dy, float dz) {
    switch (hash & 7) {
        case 0: return  dx + dy;
        case 1: return -dx + dy;
        case 2: return  dx - dy;
        case 3: return -dx - dy;
        case 4: return  dx + dz;
        case 5: return -dx + dz;
        case 6: return  dx - dz;
        default:return -dx - dz;
    }
}

// ── Value noise ───────────────────────────────────────────────────────────
float FractalNoise::ValueNoise2(float x, float y, uint32_t seed) {
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    float fx = x - ix, fy = y - iy;
    float u = fade(fx), v = fade(fy);
    float v00 = Rand01(ix,   iy,   seed);
    float v10 = Rand01(ix+1, iy,   seed);
    float v01 = Rand01(ix,   iy+1, seed);
    float v11 = Rand01(ix+1, iy+1, seed);
    return lerp(lerp(v00, v10, u), lerp(v01, v11, u), v) * 2.0f - 1.0f;
}

// ── Perlin noise ──────────────────────────────────────────────────────────
float FractalNoise::PerlinNoise2(float x, float y, uint32_t seed) {
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    float fx = x - ix, fy = y - iy;
    float u = fade(fx), v = fade(fy);
    float n00 = Grad2(Hash(ix,   iy,   seed), fx,      fy);
    float n10 = Grad2(Hash(ix+1, iy,   seed), fx-1.0f, fy);
    float n01 = Grad2(Hash(ix,   iy+1, seed), fx,      fy-1.0f);
    float n11 = Grad2(Hash(ix+1, iy+1, seed), fx-1.0f, fy-1.0f);
    return lerp(lerp(n00, n10, u), lerp(n01, n11, u), v);
}

// ── Simplex noise (2-D, classic) ──────────────────────────────────────────
float FractalNoise::SimplexNoise2(float x, float y, uint32_t seed) {
    // Skew 2-D grid
    const float F2 = 0.5f * (std::sqrt(3.0f) - 1.0f);
    const float G2 = (3.0f - std::sqrt(3.0f)) / 6.0f;
    float s  = (x + y) * F2;
    int   ix = static_cast<int>(std::floor(x + s));
    int   iy = static_cast<int>(std::floor(y + s));
    float t  = (ix + iy) * G2;
    float x0 = x - (ix - t), y0 = y - (iy - t);
    int   ix1 = (x0 > y0) ? 1 : 0;
    int   iy1 = (x0 > y0) ? 0 : 1;
    float x1 = x0 - ix1 + G2, y1 = y0 - iy1 + G2;
    float x2 = x0 - 1.0f + 2.0f*G2, y2 = y0 - 1.0f + 2.0f*G2;

    auto contrib = [&](float px, float py, int gx, int gy) -> float {
        float t2 = 0.5f - px*px - py*py;
        if (t2 < 0.0f) return 0.0f;
        t2 *= t2;
        return t2 * t2 * Grad2(Hash(gx, gy, seed), px, py);
    };

    return 70.0f * (contrib(x0, y0, ix,     iy)
                  + contrib(x1, y1, ix+ix1, iy+iy1)
                  + contrib(x2, y2, ix+1,   iy+1));
}

// ── Worley (cellular) noise ───────────────────────────────────────────────
float FractalNoise::WorleyNoise2(float x, float y, uint32_t seed) {
    int cx = static_cast<int>(std::floor(x));
    int cy = static_cast<int>(std::floor(y));
    float minDist = 1e9f;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            float px = cx + dx + Rand01(cx+dx, cy+dy, seed);
            float py = cy + dy + Rand01(cx+dx, cy+dy, seed ^ 0xDEADBEEFu);
            float d  = std::sqrt((x-px)*(x-px) + (y-py)*(y-py));
            minDist  = std::min(minDist, d);
        }
    }
    return minDist * 2.0f - 1.0f;  // map ~[0,1.4] to ~[-1,1.8]
}

// ── 3-D variants (simplified) ─────────────────────────────────────────────
float FractalNoise::ValueNoise3(float x, float y, float z, uint32_t seed) {
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    int iz = static_cast<int>(std::floor(z));
    float fx=x-ix, fy=y-iy, fz=z-iz;
    float u=fade(fx), v=fade(fy), w=fade(fz);
    auto r = [&](int a,int b,int c){ return Rand01(a,b^(c*131), seed); };
    float v000=r(ix,iy,iz),   v100=r(ix+1,iy,iz),
          v010=r(ix,iy+1,iz), v110=r(ix+1,iy+1,iz),
          v001=r(ix,iy,iz+1), v101=r(ix+1,iy,iz+1),
          v011=r(ix,iy+1,iz+1), v111=r(ix+1,iy+1,iz+1);
    return (lerp(lerp(lerp(v000,v100,u),lerp(v010,v110,u),v),
                 lerp(lerp(v001,v101,u),lerp(v011,v111,u),v), w)) * 2.0f - 1.0f;
}

float FractalNoise::PerlinNoise3(float x, float y, float z, uint32_t seed) {
    int ix=static_cast<int>(std::floor(x));
    int iy=static_cast<int>(std::floor(y));
    int iz=static_cast<int>(std::floor(z));
    float fx=x-ix, fy=y-iy, fz=z-iz;
    float u=fade(fx), v=fade(fy), w=fade(fz);
    auto G=[&](int a,int b,int c,float dx,float dy,float dz){
        return Grad3(Hash(a,b,c,seed),dx,dy,dz);
    };
    float n000=G(ix,  iy,  iz,  fx,   fy,   fz),
          n100=G(ix+1,iy,  iz,  fx-1, fy,   fz),
          n010=G(ix,  iy+1,iz,  fx,   fy-1, fz),
          n110=G(ix+1,iy+1,iz,  fx-1, fy-1, fz),
          n001=G(ix,  iy,  iz+1,fx,   fy,   fz-1),
          n101=G(ix+1,iy,  iz+1,fx-1, fy,   fz-1),
          n011=G(ix,  iy+1,iz+1,fx,   fy-1, fz-1),
          n111=G(ix+1,iy+1,iz+1,fx-1, fy-1, fz-1);
    return lerp(lerp(lerp(n000,n100,u),lerp(n010,n110,u),v),
                lerp(lerp(n001,n101,u),lerp(n011,n111,u),v),w);
}

float FractalNoise::SimplexNoise3(float x, float y, float z, uint32_t seed) {
    // Simplified: use 2-D simplex evaluated on the xy plane + z contribution
    return SimplexNoise2(x + z * 0.31f, y + z * 0.17f, seed);
}

float FractalNoise::WorleyNoise3(float x, float y, float z, uint32_t seed) {
    return WorleyNoise2(x + z * 0.31f, y + z * 0.17f, seed);
}

// ── Base dispatch ─────────────────────────────────────────────────────────
float FractalNoise::BaseNoise2(float x, float y, NoiseType type, uint32_t seed) {
    switch (type) {
        case NoiseType::Value:   return ValueNoise2(x, y, seed);
        case NoiseType::Perlin:  return PerlinNoise2(x, y, seed);
        case NoiseType::Simplex: return SimplexNoise2(x, y, seed);
        case NoiseType::Worley:  return WorleyNoise2(x, y, seed);
    }
    return 0.0f;
}
float FractalNoise::BaseNoise3(float x, float y, float z, NoiseType type, uint32_t seed) {
    switch (type) {
        case NoiseType::Value:   return ValueNoise3(x, y, z, seed);
        case NoiseType::Perlin:  return PerlinNoise3(x, y, z, seed);
        case NoiseType::Simplex: return SimplexNoise3(x, y, z, seed);
        case NoiseType::Worley:  return WorleyNoise3(x, y, z, seed);
    }
    return 0.0f;
}

// ── fBm 2-D ───────────────────────────────────────────────────────────────
float FractalNoise::fBm(float x, float y, const FractalParams& p) {
    float freq = p.scale, amp = 1.0f, total = 0.0f, maxAmp = 0.0f;
    float px = x + p.offset[0], py = y + p.offset[1];
    for (int i = 0; i < p.octaves; ++i) {
        total  += amp * BaseNoise2(px * freq, py * freq, p.noiseType, p.seed + i);
        maxAmp += amp;
        freq   *= p.lacunarity;
        amp    *= p.persistence;
    }
    return maxAmp > 0.0f ? total / maxAmp : 0.0f;
}

// ── Turbulence 2-D ────────────────────────────────────────────────────────
float FractalNoise::Turbulence(float x, float y, const FractalParams& p) {
    float freq = p.scale, amp = 1.0f, total = 0.0f, maxAmp = 0.0f;
    float px = x + p.offset[0], py = y + p.offset[1];
    for (int i = 0; i < p.octaves; ++i) {
        total  += amp * std::abs(BaseNoise2(px * freq, py * freq, p.noiseType, p.seed + i));
        maxAmp += amp;
        freq   *= p.lacunarity;
        amp    *= p.persistence;
    }
    return maxAmp > 0.0f ? total / maxAmp : 0.0f;
}

// ── Ridged multi 2-D ──────────────────────────────────────────────────────
float FractalNoise::RidgedMulti(float x, float y, const FractalParams& p) {
    float freq = p.scale, amp = 1.0f, total = 0.0f, maxAmp = 0.0f;
    float px = x + p.offset[0], py = y + p.offset[1];
    for (int i = 0; i < p.octaves; ++i) {
        float n = BaseNoise2(px * freq, py * freq, p.noiseType, p.seed + i);
        total  += amp * (1.0f - std::abs(n));
        maxAmp += amp;
        freq   *= p.lacunarity;
        amp    *= p.persistence;
    }
    return maxAmp > 0.0f ? total / maxAmp : 0.0f;
}

// ── fBm 3-D ───────────────────────────────────────────────────────────────
float FractalNoise::fBm(float x, float y, float z, const FractalParams& p) {
    float freq = p.scale, amp = 1.0f, total = 0.0f, maxAmp = 0.0f;
    float px = x + p.offset[0], py = y + p.offset[1], pz = z + p.offset[2];
    for (int i = 0; i < p.octaves; ++i) {
        total  += amp * BaseNoise3(px*freq, py*freq, pz*freq, p.noiseType, p.seed+i);
        maxAmp += amp;
        freq   *= p.lacunarity;
        amp    *= p.persistence;
    }
    return maxAmp > 0.0f ? total / maxAmp : 0.0f;
}

float FractalNoise::Turbulence(float x, float y, float z, const FractalParams& p) {
    float freq = p.scale, amp = 1.0f, total = 0.0f, maxAmp = 0.0f;
    float px = x + p.offset[0], py = y + p.offset[1], pz = z + p.offset[2];
    for (int i = 0; i < p.octaves; ++i) {
        total  += amp * std::abs(BaseNoise3(px*freq, py*freq, pz*freq, p.noiseType, p.seed+i));
        maxAmp += amp;
        freq   *= p.lacunarity;
        amp    *= p.persistence;
    }
    return maxAmp > 0.0f ? total / maxAmp : 0.0f;
}

float FractalNoise::RidgedMulti(float x, float y, float z, const FractalParams& p) {
    float freq = p.scale, amp = 1.0f, total = 0.0f, maxAmp = 0.0f;
    float px = x + p.offset[0], py = y + p.offset[1], pz = z + p.offset[2];
    for (int i = 0; i < p.octaves; ++i) {
        float n = BaseNoise3(px*freq, py*freq, pz*freq, p.noiseType, p.seed+i);
        total  += amp * (1.0f - std::abs(n));
        maxAmp += amp;
        freq   *= p.lacunarity;
        amp    *= p.persistence;
    }
    return maxAmp > 0.0f ? total / maxAmp : 0.0f;
}

// ── GenerateMap ───────────────────────────────────────────────────────────
std::vector<float> FractalNoise::GenerateMap(size_t width, size_t height,
                                             const FractalParams& p) {
    std::vector<float> out(width * height);
    float invW = 1.0f / static_cast<float>(width);
    float invH = 1.0f / static_cast<float>(height);
    float minV = 1e9f, maxV = -1e9f;
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            float v = fBm(x * invW, y * invH, p);
            out[y * width + x] = v;
            minV = std::min(minV, v);
            maxV = std::max(maxV, v);
        }
    }
    // Normalise to [0, 1]
    float range = maxV - minV;
    if (range > 1e-6f) {
        for (auto& v : out) v = (v - minV) / range;
    }
    return out;
}

// ── GenerateNormalMap ─────────────────────────────────────────────────────
std::vector<float> FractalNoise::GenerateNormalMap(size_t width, size_t height,
                                                   const FractalParams& p,
                                                   float heightScale) {
    auto hmap = GenerateMap(width, height, p);
    std::vector<float> out(width * height * 4);
    float inv = 1.0f / static_cast<float>(width);

    auto sample = [&](int x, int y) -> float {
        x = std::clamp(x, 0, static_cast<int>(width)  - 1);
        y = std::clamp(y, 0, static_cast<int>(height) - 1);
        return hmap[y * width + x] * heightScale;
    };

    for (size_t row = 0; row < height; ++row) {
        for (size_t col = 0; col < width; ++col) {
            float hL = sample(static_cast<int>(col)-1, static_cast<int>(row));
            float hR = sample(static_cast<int>(col)+1, static_cast<int>(row));
            float hD = sample(static_cast<int>(col),   static_cast<int>(row)-1);
            float hU = sample(static_cast<int>(col),   static_cast<int>(row)+1);
            float nx = (hL - hR) * 2.0f;
            float ny = (hD - hU) * 2.0f;
            float nz = 4.0f * inv;
            float len = std::sqrt(nx*nx + ny*ny + nz*nz);
            if (len > 1e-6f) { nx /= len; ny /= len; nz /= len; }
            size_t idx = (row * width + col) * 4;
            out[idx+0] = nx * 0.5f + 0.5f;
            out[idx+1] = ny * 0.5f + 0.5f;
            out[idx+2] = nz * 0.5f + 0.5f;
            out[idx+3] = hmap[row * width + col];
        }
    }
    return out;
}

// ── Domain warp ───────────────────────────────────────────────────────────
WarpResult FractalNoise::Warp(float x, float y, const FractalParams& p, float strength) {
    FractalParams p2 = p;
    p2.offset[0] += 5.2f; p2.offset[1] += 1.3f;
    float wx = fBm(x, y, p);
    float wy = fBm(x, y, p2);
    return { x + strength * wx, y + strength * wy };
}

} // namespace PCG
