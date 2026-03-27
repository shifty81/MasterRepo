#include "PCG/WorldGenerator/WorldGenerator/WorldGenerator.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>

namespace PCG {

// ── Minimal Perlin-like noise (value noise) ──────────────────────────────────

static float Fade(float t) { return t*t*t*(t*(t*6.f-15.f)+10.f); }
static float LerpF(float a, float b, float t) { return a+t*(b-a); }

static float ValueNoise2D(float x, float y, uint64_t seed) {
    auto hash = [&](int32_t ix, int32_t iy) -> float {
        uint64_t h = (uint64_t)(ix*73856093) ^ (uint64_t)(iy*19349663) ^ seed;
        h ^= h >> 33; h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ULL;
        h ^= h >> 33;
        return (float)(h >> 11) / (float)(1ull<<53);
    };
    int32_t ix=(int32_t)std::floor(x), iy=(int32_t)std::floor(y);
    float fx=x-ix, fy=y-iy;
    float u=Fade(fx), v=Fade(fy);
    float a=LerpF(hash(ix,iy),   hash(ix+1,iy),   u);
    float b=LerpF(hash(ix,iy+1), hash(ix+1,iy+1), u);
    return LerpF(a,b,v);
}

static float Octaves(float x, float y, uint64_t seed, uint32_t oct,
                      float pers, float lac, float scale)
{
    float val=0, amp=1, freq=scale, maxAmp=0;
    for (uint32_t i=0;i<oct;i++) {
        val   += ValueNoise2D(x*freq, y*freq, seed+i) * amp;
        maxAmp+= amp; amp*=pers; freq*=lac;
    }
    return maxAmp>0.f ? val/maxAmp : 0.f;
}

// ── WorldData helpers ─────────────────────────────────────────────────────────

float WorldData::GetHeight(int32_t x, int32_t y) const {
    if(x<0||y<0||(uint32_t)x>=width||(uint32_t)y>=height) return 0.f;
    return heightMap[(uint32_t)y*width+(uint32_t)x];
}
BiomeType WorldData::GetBiome(int32_t x, int32_t y) const {
    if(x<0||y<0||(uint32_t)x>=width||(uint32_t)y>=height) return BiomeType::Ocean;
    return biomeMap[(uint32_t)y*width+(uint32_t)x];
}
std::vector<const WorldFeature*> WorldData::GetFeatures(BiomeType biome) const {
    std::vector<const WorldFeature*> out;
    for (auto& f:features) if(f.biome==biome) out.push_back(&f);
    return out;
}
std::vector<const WorldFeature*> WorldData::GetFeaturesByType(const std::string& type) const {
    std::vector<const WorldFeature*> out;
    for (auto& f:features) if(f.type==type) out.push_back(&f);
    return out;
}
WorldData WorldData::GetRegion(uint32_t rx, uint32_t ry, uint32_t rw, uint32_t rh) const {
    WorldData sub; sub.width=rw; sub.height=rh;
    sub.heightMap.resize((size_t)rw*rh);
    sub.biomeMap.resize((size_t)rw*rh);
    for (uint32_t y=0;y<rh;y++) for (uint32_t x=0;x<rw;x++) {
        uint32_t sx=rx+x, sy=ry+y;
        sub.heightMap[y*rw+x] = GetHeight((int32_t)sx,(int32_t)sy);
        sub.biomeMap [y*rw+x] = GetBiome ((int32_t)sx,(int32_t)sy);
    }
    return sub;
}

// ── WorldGenerator ────────────────────────────────────────────────────────────

struct WorldGenerator::Impl {};

WorldGenerator::WorldGenerator()  : m_impl(new Impl) {}
WorldGenerator::~WorldGenerator() { delete m_impl; }

WorldData WorldGenerator::Generate(const WorldGenParams& p)
{
    WorldData world;
    world.width  = p.width;
    world.height = p.height;
    uint32_t n   = p.width * p.height;
    world.heightMap .resize(n);
    world.temperature.resize(n);
    world.humidity   .resize(n);
    world.biomeMap   .resize(n, BiomeType::Ocean);

    // Height map
    for (uint32_t y=0;y<p.height;y++) for (uint32_t x=0;x<p.width;x++) {
        float h = Octaves((float)x,(float)y, p.seed, p.octaves, p.persistence, p.lacunarity, p.scale);
        world.heightMap[y*p.width+x] = h;
        if (p.onProgress && (y*p.width+x)%(n/10+1)==0)
            p.onProgress(0.4f*(float)(y*p.width+x)/(float)n);
    }
    // Temperature (decreasing with height & latitude)
    for (uint32_t y=0;y<p.height;y++) for (uint32_t x=0;x<p.width;x++) {
        float h = world.heightMap[y*p.width+x];
        float lat= (float)y/(float)(p.height-1); // 0=south, 1=north
        float temp= 1.f - std::abs(lat-0.5f)*2.f - h*0.5f
                   + 0.1f*Octaves((float)x,(float)y, p.seed+1, 3, 0.5f, 2.f, p.scale*2.f);
        world.temperature[y*p.width+x] = std::max(0.f,std::min(1.f,temp));
    }
    // Humidity
    for (uint32_t y=0;y<p.height;y++) for (uint32_t x=0;x<p.width;x++) {
        float humid = Octaves((float)x,(float)y, p.seed+2, 4, 0.5f, 2.f, p.scale*1.5f);
        world.humidity[y*p.width+x] = humid;
    }
    // Biome assignment
    for (uint32_t i=0;i<n;i++) {
        float h=world.heightMap[i], t=world.temperature[i], hu=world.humidity[i];
        if      (h < p.seaLevel)     world.biomeMap[i] = BiomeType::Ocean;
        else if (h > p.mountainLevel)world.biomeMap[i] = BiomeType::Mountain;
        else if (t < 0.2f)           world.biomeMap[i] = BiomeType::Tundra;
        else if (t > 0.8f && hu<0.3f) world.biomeMap[i]= BiomeType::Desert;
        else if (t > 0.8f && hu>0.6f) world.biomeMap[i]= BiomeType::Jungle;
        else if (hu > 0.6f)          world.biomeMap[i] = BiomeType::Swamp;
        else if (hu > 0.3f)          world.biomeMap[i] = BiomeType::Forest;
        else                         world.biomeMap[i] = BiomeType::Grassland;
    }
    // Feature placement
    std::mt19937_64 rng(p.seed+99);
    auto rndF=[&]()->float{ return (float)(rng()>>11)/(float)(1ull<<53); };
    auto placeFeatures=[&](const std::string& type, float density, BiomeType preferBiome){
        uint32_t count=(uint32_t)(p.width*p.height*density);
        for(uint32_t i=0;i<count;i++){
            uint32_t x=(uint32_t)(rndF()*p.width);
            uint32_t y=(uint32_t)(rndF()*p.height);
            WorldFeature f; f.x=(float)x; f.y=(float)y; f.type=type;
            f.biome=world.biomeMap[y*p.width+x]; f.importance=rndF();
            world.features.push_back(f);
        }
    };
    placeFeatures("dungeon",  p.dungeon_density,  BiomeType::Mountain);
    placeFeatures("town",     p.town_density,     BiomeType::Grassland);
    placeFeatures("resource", p.resource_density, BiomeType::Forest);

    if (p.onProgress) p.onProgress(1.f);
    return world;
}

bool WorldGenerator::SaveJSON(const WorldData& world, const std::string& path) const {
    std::ofstream f(path); if(!f) return false;
    f << "{\"width\":" << world.width << ",\"height\":" << world.height
      << ",\"features\":" << world.features.size() << "}";
    return true;
}
bool WorldGenerator::LoadJSON(const std::string& path, WorldData& outWorld) const {
    std::ifstream f(path); if(!f) return false;
    (void)outWorld; return true;
}

} // namespace PCG
