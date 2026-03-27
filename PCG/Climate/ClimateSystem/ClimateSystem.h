#pragma once
/**
 * @file ClimateSystem.h
 * @brief Biome map, temperature/humidity grid, seasonal tick, per-cell weather state.
 *
 * Features:
 *   - ClimateGrid: width x height cells, each has temperature (°C), humidity [0,1], biome id
 *   - BiomeDef: id, name, tempRange, humidityRange, precipitationRate
 *   - Init(w, h, cellSize)
 *   - RegisterBiome(def)
 *   - Seed(seed) + Generate(): assign biomes via noise-based classification
 *   - Tick(dt): advance seasonal simulation (day/year cycle, weather transitions)
 *   - SetSeason(0=Spring..3=Winter)
 *   - GetTemperature(x, y) / GetHumidity(x, y)
 *   - GetBiome(x, y) → biomeId
 *   - GetWeather(x, y) → WeatherState {sunny, cloudy, rain, snow, storm}
 *   - QueryClimateAtWorld(wx, wy) → ClimateCell
 *   - Per-cell override: SetCell(x, y, temp, humidity)
 *
 * Typical usage:
 * @code
 *   ClimateSystem cs;
 *   cs.Init(64, 64, 100.f);
 *   cs.Seed(42); cs.Generate();
 *   cs.SetSeason(2); // Summer
 *   cs.Tick(dt);
 *   auto cell = cs.QueryClimateAtWorld(500.f, 300.f);
 * @endcode
 */

#include <cstdint>
#include <string>
#include <vector>

namespace PCG {

enum class WeatherState : uint8_t { Sunny=0, Cloudy, Rain, Snow, Storm };

struct BiomeDef {
    std::string id;
    std::string name;
    float       tempMin   {-10.f}, tempMax{30.f};
    float       humidMin  {0.f},   humidMax{1.f};
    float       precipRate{0.2f};
};

struct ClimateCell {
    float        temperature{15.f};
    float        humidity   {0.5f};
    std::string  biomeId;
    WeatherState weather    {WeatherState::Sunny};
};

class ClimateSystem {
public:
    ClimateSystem();
    ~ClimateSystem();

    void Init    (uint32_t w=64, uint32_t h=64, float cellSize=100.f);
    void Shutdown();
    void Tick    (float dt);

    // Biomes
    void RegisterBiome  (const BiomeDef& def);
    void UnregisterBiome(const std::string& id);
    const BiomeDef* GetBiomeDef(const std::string& id) const;

    // Generation
    void Seed    (uint32_t seed);
    void Generate();

    // Season
    void SetSeason(uint8_t s);     ///< 0=Spring, 1=Summer, 2=Autumn, 3=Winter
    uint8_t GetSeason() const;
    float   GetYearProgress() const; ///< [0,1]
    void    SetDayLength(float hours);

    // Cell access
    float        GetTemperature(int32_t x, int32_t y) const;
    float        GetHumidity   (int32_t x, int32_t y) const;
    std::string  GetBiome      (int32_t x, int32_t y) const;
    WeatherState GetWeather    (int32_t x, int32_t y) const;
    void         SetCell       (int32_t x, int32_t y, float temp, float humidity);

    // World query
    ClimateCell QueryClimateAtWorld(float wx, float wy) const;

    uint32_t Width () const;
    uint32_t Height() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
