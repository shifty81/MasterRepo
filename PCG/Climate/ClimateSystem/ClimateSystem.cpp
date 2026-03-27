#include "PCG/Climate/ClimateSystem/ClimateSystem.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <unordered_map>
#include <vector>

namespace PCG {

static float Hash(uint32_t x, uint32_t y, uint32_t seed){
    uint32_t v=x*1664525u+y*22695477u+seed*1013904223u;
    v^=v>>16; v*=0x45d9f3bu; v^=v>>16;
    return (float)(v&0xFFFFFFu)/(float)0x1000000u;
}

struct ClimateCell_ {
    float        temperature{15.f};
    float        humidity   {0.5f};
    std::string  biomeId;
    WeatherState weather{WeatherState::Sunny};
};

struct ClimateSystem::Impl {
    uint32_t w{0}, h{0};
    float    cellSize{100.f};
    uint32_t seed{0};
    uint8_t  season{1}; // Summer default
    float    yearProgress{0.5f};
    float    dayLength{24.f};
    float    elapsed{0.f};
    std::vector<ClimateCell_> grid;
    std::unordered_map<std::string,BiomeDef> biomes;

    ClimateCell_& At(int32_t x, int32_t y){ return grid[(uint32_t)y*w+(uint32_t)x]; }
    const ClimateCell_& At(int32_t x, int32_t y) const { return grid[(uint32_t)y*w+(uint32_t)x]; }
    bool InBounds(int32_t x, int32_t y) const { return x>=0&&y>=0&&(uint32_t)x<w&&(uint32_t)y<h; }
};

ClimateSystem::ClimateSystem()  : m_impl(new Impl){}
ClimateSystem::~ClimateSystem() { Shutdown(); delete m_impl; }

void ClimateSystem::Init(uint32_t w, uint32_t h, float cs){
    m_impl->w=w; m_impl->h=h; m_impl->cellSize=cs;
    m_impl->grid.resize(w*h);
    // Default biomes
    BiomeDef forest; forest.id="forest"; forest.name="Forest"; forest.humidMin=0.4f; forest.humidMax=0.8f; RegisterBiome(forest);
    BiomeDef desert; desert.id="desert"; desert.name="Desert"; desert.tempMin=20.f; desert.tempMax=50.f; desert.humidMax=0.2f; RegisterBiome(desert);
    BiomeDef tundra; tundra.id="tundra"; tundra.name="Tundra"; tundra.tempMin=-30.f; tundra.tempMax=5.f; tundra.humidMax=0.4f; RegisterBiome(tundra);
    BiomeDef plains; plains.id="plains"; plains.name="Plains"; RegisterBiome(plains);
}
void ClimateSystem::Shutdown() { m_impl->grid.clear(); m_impl->biomes.clear(); }

void ClimateSystem::RegisterBiome  (const BiomeDef& d){ m_impl->biomes[d.id]=d; }
void ClimateSystem::UnregisterBiome(const std::string& id){ m_impl->biomes.erase(id); }
const BiomeDef* ClimateSystem::GetBiomeDef(const std::string& id) const {
    auto it=m_impl->biomes.find(id); return it!=m_impl->biomes.end()?&it->second:nullptr;
}

void ClimateSystem::Seed(uint32_t s){ m_impl->seed=s; }
void ClimateSystem::Generate(){
    for(uint32_t y=0;y<m_impl->h;y++) for(uint32_t x=0;x<m_impl->w;x++){
        auto& c=m_impl->grid[y*m_impl->w+x];
        c.temperature=-20.f+60.f*Hash(x,y,m_impl->seed);
        c.humidity=Hash(x+1000,y,m_impl->seed+1);
        // Assign biome
        c.biomeId="plains";
        for(auto& [id,def]:m_impl->biomes){
            if(c.temperature>=def.tempMin&&c.temperature<=def.tempMax&&
               c.humidity>=def.humidMin&&c.humidity<=def.humidMax){ c.biomeId=id; break; }
        }
        c.weather=c.humidity>0.7f?WeatherState::Rain:WeatherState::Sunny;
    }
}

void ClimateSystem::SetSeason(uint8_t s){ m_impl->season=s; }
uint8_t ClimateSystem::GetSeason() const { return m_impl->season; }
float   ClimateSystem::GetYearProgress() const { return m_impl->yearProgress; }
void    ClimateSystem::SetDayLength(float h){ m_impl->dayLength=h; }

float        ClimateSystem::GetTemperature(int32_t x, int32_t y) const { return m_impl->InBounds(x,y)?m_impl->At(x,y).temperature:0; }
float        ClimateSystem::GetHumidity   (int32_t x, int32_t y) const { return m_impl->InBounds(x,y)?m_impl->At(x,y).humidity:0; }
std::string  ClimateSystem::GetBiome      (int32_t x, int32_t y) const { return m_impl->InBounds(x,y)?m_impl->At(x,y).biomeId:""; }
WeatherState ClimateSystem::GetWeather    (int32_t x, int32_t y) const { return m_impl->InBounds(x,y)?m_impl->At(x,y).weather:WeatherState::Sunny; }
void         ClimateSystem::SetCell(int32_t x, int32_t y, float t, float hu){ if(m_impl->InBounds(x,y)){m_impl->At(x,y).temperature=t; m_impl->At(x,y).humidity=hu;} }

ClimateCell ClimateSystem::QueryClimateAtWorld(float wx, float wy) const {
    int32_t gx=(int32_t)(wx/m_impl->cellSize), gy=(int32_t)(wy/m_impl->cellSize);
    if(!m_impl->InBounds(gx,gy)) return {};
    auto& c=m_impl->At(gx,gy);
    return {c.temperature,c.humidity,c.biomeId,c.weather};
}

void ClimateSystem::Tick(float dt){
    m_impl->elapsed+=dt;
    static const float yearSec=3600.f;
    m_impl->yearProgress=std::fmod(m_impl->elapsed,yearSec)/yearSec;
    m_impl->season=(uint8_t)(m_impl->yearProgress*4.f);
    // Seasonal temperature offset
    float off=-10.f*std::cos(m_impl->yearProgress*6.2832f);
    for(auto& c:m_impl->grid){
        c.temperature+=off*dt*0.01f;
        if(c.humidity>0.7f&&c.temperature<0) c.weather=WeatherState::Snow;
        else if(c.humidity>0.7f) c.weather=WeatherState::Rain;
        else if(c.humidity>0.5f) c.weather=WeatherState::Cloudy;
        else c.weather=WeatherState::Sunny;
    }
}

uint32_t ClimateSystem::Width()  const { return m_impl->w; }
uint32_t ClimateSystem::Height() const { return m_impl->h; }

} // namespace PCG
