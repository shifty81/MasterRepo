#include "PCG/Planet/PlanetGenerator.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace PCG {

// ── Minimal fBm noise (value noise) ──────────────────────────────────────────

static float Hash(uint32_t n) {
    n = (n << 13u) ^ n;
    n = n * (n * n * 15731u + 789221u) + 1376312589u;
    return static_cast<float>(n & 0x7fffffffu) / 2147483648.f;
}

static float Noise3(int x, int y, int z, uint64_t seed) {
    return Hash(static_cast<uint32_t>(x * 1619 + y * 31337 + z * 6271 + (uint32_t)seed));
}

static float Lerp(float a, float b, float t) { return a + t * (b - a); }

static float SmoothNoise(float x, float y, float z, uint64_t seed) {
    int ix = (int)std::floor(x), iy = (int)std::floor(y), iz = (int)std::floor(z);
    float fx = x - ix, fy = y - iy, fz = z - iz;
    fx = fx*fx*(3-2*fx); fy = fy*fy*(3-2*fy); fz = fz*fz*(3-2*fz);
    return Lerp(Lerp(Lerp(Noise3(ix,iy,iz,seed), Noise3(ix+1,iy,iz,seed), fx),
                     Lerp(Noise3(ix,iy+1,iz,seed), Noise3(ix+1,iy+1,iz,seed), fx), fy),
                Lerp(Lerp(Noise3(ix,iy,iz+1,seed), Noise3(ix+1,iy,iz+1,seed), fx),
                     Lerp(Noise3(ix,iy+1,iz+1,seed), Noise3(ix+1,iy+1,iz+1,seed), fx), fy),
                fz);
}

static float FBM(float x, float y, float z, uint32_t octaves, uint64_t seed) {
    float value = 0.f, amplitude = 0.5f, freq = 1.f, maxVal = 0.f;
    for (uint32_t i = 0; i < octaves; ++i) {
        value   += amplitude * SmoothNoise(x*freq, y*freq, z*freq, seed + i);
        maxVal  += amplitude;
        amplitude *= 0.5f;
        freq     *= 2.f;
    }
    return maxVal > 0.f ? value / maxVal : 0.f;
}

// ── Biome lookup ──────────────────────────────────────────────────────────────

static BiomeType ClassifyBiome(float height, float latitudeFrac, float oceanLevel,
                               bool hasIce) {
    if (height < oceanLevel * 0.5f) return BiomeType::DeepOcean;
    if (height < oceanLevel)        return BiomeType::Ocean;
    float land = (height - oceanLevel) / (1.f - oceanLevel);
    if (hasIce && std::abs(latitudeFrac - 0.5f) > 0.45f) return BiomeType::Arctic;
    if (land < 0.05f)  return BiomeType::Beach;
    if (land < 0.2f)   return latitudeFrac < 0.3f || latitudeFrac > 0.7f ? BiomeType::Tundra : BiomeType::Grassland;
    if (land < 0.5f)   return latitudeFrac > 0.3f && latitudeFrac < 0.7f ? BiomeType::Forest : BiomeType::Tundra;
    if (land < 0.75f)  return BiomeType::Desert;
    return BiomeType::Volcanic;
}

// ── Biome colour ──────────────────────────────────────────────────────────────

static uint32_t BiomeColour(BiomeType b) {
    switch (b) {
        case BiomeType::DeepOcean: return 0xFF5533CCu;
        case BiomeType::Ocean:     return 0xFF7755EEu;
        case BiomeType::Beach:     return 0xFFDDCC99u;
        case BiomeType::Grassland: return 0xFF44BB44u;
        case BiomeType::Forest:    return 0xFF226622u;
        case BiomeType::Desert:    return 0xFFCCCC55u;
        case BiomeType::Tundra:    return 0xFF889988u;
        case BiomeType::Arctic:    return 0xFFEEEEEEu;
        case BiomeType::Volcanic:  return 0xFF443322u;
        default:                   return 0xFF888888u;
    }
}

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct PlanetGenerator::Impl {
    PlanetConfig config;
    PlanetData   lastResult;
    std::function<void(float)>               onProgress;
    std::function<void(const PlanetData&)>   onComplete;
};

PlanetGenerator::PlanetGenerator() : m_impl(new Impl()) {}
PlanetGenerator::~PlanetGenerator() { delete m_impl; }

void PlanetGenerator::Init(const PlanetConfig& cfg) { m_impl->config = cfg; }
void PlanetGenerator::Shutdown() {}

PlanetData PlanetGenerator::Generate() {
    return GenerateWithSeed(m_impl->config.seed);
}

PlanetData PlanetGenerator::GenerateWithSeed(uint64_t seed) {
    PlanetData d;
    d.seed       = seed;
    uint32_t res = m_impl->config.mapResolution;
    d.resolution = res;
    d.heightmap.resize((size_t)res * res);
    d.biomemap.resize((size_t)res * res);
    d.albedoRGBA.resize((size_t)res * res * 4);
    d.minHeight  = 1.f;
    d.maxHeight  = 0.f;

    float oc = m_impl->config.oceanLevel;
    float rough = m_impl->config.roughness;

    for (uint32_t y = 0; y < res; ++y) {
        if (m_impl->onProgress)
            m_impl->onProgress(static_cast<float>(y) / res);
        float latFrac = static_cast<float>(y) / (res - 1);
        for (uint32_t x = 0; x < res; ++x) {
            float lon = static_cast<float>(x) / (res - 1);
            // Convert equirectangular to 3D sphere point for noise
            float theta = latFrac * 3.14159265f;
            float phi   = lon * 6.28318530f;
            float sx = std::sin(theta) * std::cos(phi) * rough;
            float sy = std::sin(theta) * std::sin(phi) * rough;
            float sz = std::cos(theta) * rough;

            float h = FBM(sx + 5.f, sy + 5.f, sz + 5.f,
                          m_impl->config.octaves, seed);
            size_t idx = y * res + x;
            d.heightmap[idx] = h;
            d.minHeight = std::min(d.minHeight, h);
            d.maxHeight = std::max(d.maxHeight, h);

            BiomeType b = ClassifyBiome(h, latFrac, oc, m_impl->config.hasIce);
            d.biomemap[idx] = b;
            if (b == BiomeType::Ocean || b == BiomeType::DeepOcean) ++d.oceanTexelCount;
            else ++d.landTexelCount;

            uint32_t col = BiomeColour(b);
            d.albedoRGBA[idx * 4 + 0] = (col >> 16) & 0xFF; // R
            d.albedoRGBA[idx * 4 + 1] = (col >>  8) & 0xFF; // G
            d.albedoRGBA[idx * 4 + 2] = (col >>  0) & 0xFF; // B
            d.albedoRGBA[idx * 4 + 3] = 0xFF;                // A
        }
    }
    if (m_impl->onProgress) m_impl->onProgress(1.f);
    m_impl->lastResult = d;
    if (m_impl->onComplete) m_impl->onComplete(d);
    return d;
}

bool PlanetGenerator::ExportHeightmap(const std::string& path) const {
    auto& d = m_impl->lastResult;
    if (d.heightmap.empty()) return false;
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    // Write PGM (greyscale portable bitmap)
    f << "P5\n" << d.resolution << " " << d.resolution << "\n255\n";
    for (float h : d.heightmap) {
        uint8_t v = static_cast<uint8_t>(std::min(h, 1.f) * 255.f);
        f.write(reinterpret_cast<char*>(&v), 1);
    }
    return true;
}

bool PlanetGenerator::ExportAlbedo(const std::string& path) const {
    auto& d = m_impl->lastResult;
    if (d.albedoRGBA.empty()) return false;
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    f << "P6\n" << d.resolution << " " << d.resolution << "\n255\n";
    for (size_t i = 0; i + 3 < d.albedoRGBA.size(); i += 4)
        f.write(reinterpret_cast<const char*>(&d.albedoRGBA[i]), 3);
    return true;
}

bool PlanetGenerator::ExportBiomeMap(const std::string& path) const {
    auto& d = m_impl->lastResult;
    if (d.biomemap.empty()) return false;
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    f << "P5\n" << d.resolution << " " << d.resolution << "\n255\n";
    for (BiomeType b : d.biomemap) {
        uint8_t v = static_cast<uint8_t>(b) * 30;
        f.write(reinterpret_cast<char*>(&v), 1);
    }
    return true;
}

PlanetData   PlanetGenerator::LastResult() const { return m_impl->lastResult; }
PlanetConfig PlanetGenerator::GetConfig()  const { return m_impl->config; }
void         PlanetGenerator::SetConfig(const PlanetConfig& c) { m_impl->config = c; }

float PlanetGenerator::SampleHeight(float lonDeg, float latDeg) const {
    auto& d = m_impl->lastResult;
    if (d.heightmap.empty()) return 0.f;
    uint32_t x = static_cast<uint32_t>((lonDeg + 180.f) / 360.f * (d.resolution - 1));
    uint32_t y = static_cast<uint32_t>((latDeg + 90.f)  / 180.f * (d.resolution - 1));
    x = std::min(x, d.resolution - 1);
    y = std::min(y, d.resolution - 1);
    return d.heightmap[y * d.resolution + x];
}

BiomeType PlanetGenerator::SampleBiome(float lonDeg, float latDeg) const {
    auto& d = m_impl->lastResult;
    if (d.biomemap.empty()) return BiomeType::Ocean;
    uint32_t x = static_cast<uint32_t>((lonDeg + 180.f) / 360.f * (d.resolution - 1));
    uint32_t y = static_cast<uint32_t>((latDeg + 90.f)  / 180.f * (d.resolution - 1));
    x = std::min(x, d.resolution - 1);
    y = std::min(y, d.resolution - 1);
    return d.biomemap[y * d.resolution + x];
}

void PlanetGenerator::OnProgress(std::function<void(float)> cb)           { m_impl->onProgress = std::move(cb); }
void PlanetGenerator::OnComplete(std::function<void(const PlanetData&)> cb) { m_impl->onComplete = std::move(cb); }

} // namespace PCG
