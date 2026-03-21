#include "PCG/Weather/WeatherSystem.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <stdexcept>

namespace PCG {

// ── WeatherState::Lerp ────────────────────────────────────────────────────
static float wlerp(float a, float b, float t) { return a + (b - a) * t; }

WeatherState WeatherState::Lerp(const WeatherState& target, float t) const {
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    WeatherState s;
    s.cloudCover            = wlerp(cloudCover,            target.cloudCover,            t);
    s.rainIntensity         = wlerp(rainIntensity,         target.rainIntensity,         t);
    s.snowIntensity         = wlerp(snowIntensity,         target.snowIntensity,         t);
    s.fogDensity            = wlerp(fogDensity,            target.fogDensity,            t);
    s.windSpeed             = wlerp(windSpeed,             target.windSpeed,             t);
    s.windDirDeg            = wlerp(windDirDeg,            target.windDirDeg,            t);
    s.temperature           = wlerp(temperature,           target.temperature,           t);
    s.humidity              = wlerp(humidity,              target.humidity,              t);
    s.lightningProbability  = wlerp(lightningProbability,  target.lightningProbability,  t);
    s.visibilityMetres      = wlerp(visibilityMetres,      target.visibilityMetres,      t);
    s.fogColorR             = wlerp(fogColorR,             target.fogColorR,             t);
    s.fogColorG             = wlerp(fogColorG,             target.fogColorG,             t);
    s.fogColorB             = wlerp(fogColorB,             target.fogColorB,             t);
    return s;
}

// ── LCG RNG ───────────────────────────────────────────────────────────────
static uint64_t rngNext(uint64_t& s) {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    return s;
}
static float rngFloat(uint64_t& s) {
    return static_cast<float>(rngNext(s) >> 11) / static_cast<float>(1ULL << 53);
}
static float rngRange(uint64_t& s, float lo, float hi) {
    return lo + rngFloat(s) * (hi - lo);
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct WeatherSystem::Impl {
    std::unordered_map<std::string, WeatherPreset> presets;
    WeatherState   current;
    WeatherState   source;
    WeatherState   targetState;
    std::string    currentPresetId;
    std::string    targetPresetId;
    float          blendDuration{10.0f};
    float          blendElapsed{0.0f};
    bool           blending{false};
    bool           running{false};
    float          holdElapsed{0.0f};
    float          holdDuration{60.0f};
    float          timeOfDay{12.0f};
    uint64_t       rng{0};

    std::vector<WeatherChangeCb> changeCbs;
    std::vector<WeatherTickCb>   tickCbs;
};

WeatherSystem::WeatherSystem() : m_impl(new Impl()) {
    m_impl->rng = static_cast<uint64_t>(std::time(nullptr));
}
WeatherSystem::~WeatherSystem() { delete m_impl; }

void WeatherSystem::RegisterPreset(WeatherPreset preset) {
    m_impl->presets[preset.id] = std::move(preset);
}
bool WeatherSystem::HasPreset(const std::string& id) const {
    return m_impl->presets.count(id) > 0;
}
size_t WeatherSystem::PresetCount() const { return m_impl->presets.size(); }
const WeatherPreset* WeatherSystem::GetPreset(const std::string& id) const {
    auto it = m_impl->presets.find(id);
    return it != m_impl->presets.end() ? &it->second : nullptr;
}
std::vector<std::string> WeatherSystem::PresetIds() const {
    std::vector<std::string> out;
    for (const auto& [k,_] : m_impl->presets) out.push_back(k);
    std::sort(out.begin(), out.end());
    return out;
}

void WeatherSystem::Start(const std::string& presetId) {
    m_impl->running = true;
    std::string pid = presetId;
    if (pid.empty() && !m_impl->presets.empty())
        pid = m_impl->presets.begin()->first;
    if (m_impl->presets.count(pid)) {
        m_impl->current        = m_impl->presets[pid].state;
        m_impl->currentPresetId= pid;
        m_impl->holdDuration   = rngRange(m_impl->rng,
            m_impl->presets[pid].minDurationSec,
            m_impl->presets[pid].maxDurationSec);
        m_impl->holdElapsed    = 0.0f;
    }
    m_impl->blending = false;
}

void WeatherSystem::Stop() { m_impl->running = false; }
bool WeatherSystem::IsRunning() const { return m_impl->running; }

void WeatherSystem::TransitionTo(const std::string& presetId,
                                   float blendDurationSec) {
    if (!m_impl->presets.count(presetId)) return;
    m_impl->source         = m_impl->current;
    m_impl->targetPresetId = presetId;
    m_impl->targetState    = m_impl->presets[presetId].state;
    m_impl->blendDuration  = blendDurationSec > 0.0f ? blendDurationSec : 0.001f;
    m_impl->blendElapsed   = 0.0f;
    m_impl->blending       = true;

    WeatherEvent ev;
    ev.fromPreset      = m_impl->currentPresetId;
    ev.toPreset        = presetId;
    ev.blendDurationSec= blendDurationSec;
    for (auto& cb : m_impl->changeCbs) cb(ev);
}

void WeatherSystem::SetCustomState(const WeatherState& state) {
    m_impl->current  = state;
    m_impl->blending = false;
}

void WeatherSystem::Update(float dt) {
    if (!m_impl->running) return;

    if (m_impl->blending) {
        m_impl->blendElapsed += dt;
        float t = m_impl->blendElapsed / m_impl->blendDuration;
        if (t >= 1.0f) {
            t = 1.0f;
            m_impl->blending       = false;
            m_impl->currentPresetId= m_impl->targetPresetId;
            // Recalculate hold duration for the new preset
            if (m_impl->presets.count(m_impl->currentPresetId)) {
                const auto& p = m_impl->presets[m_impl->currentPresetId];
                m_impl->holdDuration = rngRange(m_impl->rng,
                    p.minDurationSec, p.maxDurationSec);
            }
            m_impl->holdElapsed = 0.0f;
        }
        m_impl->current = m_impl->source.Lerp(m_impl->targetState, t);
    } else {
        // Auto-transition after hold time
        m_impl->holdElapsed += dt;
        if (m_impl->holdElapsed >= m_impl->holdDuration && m_impl->presets.size() > 1) {
            // Pick next preset by weighted random (skip current)
            float totalWeight = 0.0f;
            for (const auto& [id, p] : m_impl->presets)
                if (id != m_impl->currentPresetId) totalWeight += p.weight;
            float r = rngFloat(m_impl->rng) * totalWeight;
            float cum = 0.0f;
            std::string next = m_impl->presets.begin()->first;
            for (const auto& [id, p] : m_impl->presets) {
                if (id == m_impl->currentPresetId) continue;
                cum += p.weight;
                if (r <= cum) { next = id; break; }
            }
            TransitionTo(next, 10.0f);
        }
    }

    // Time-of-day temperature modulation
    float hour = m_impl->timeOfDay;
    float todMod = std::sin((hour - 6.0f) / 24.0f * 6.2831853f) * 5.0f;
    m_impl->current.temperature += todMod * dt / 3600.0f; // very gentle drift

    for (auto& cb : m_impl->tickCbs) cb(m_impl->current);
}

const WeatherState& WeatherSystem::CurrentState() const { return m_impl->current; }
std::string WeatherSystem::CurrentPresetId()  const { return m_impl->currentPresetId; }
std::string WeatherSystem::TargetPresetId()   const { return m_impl->targetPresetId; }
float WeatherSystem::BlendFraction() const {
    if (!m_impl->blending) return 1.0f;
    return m_impl->blendElapsed / m_impl->blendDuration;
}

void WeatherSystem::SetTimeOfDay(float hour) { m_impl->timeOfDay = hour; }
float WeatherSystem::TimeOfDay() const       { return m_impl->timeOfDay; }
void WeatherSystem::SetSeed(uint64_t seed)   { m_impl->rng = seed; }

void WeatherSystem::OnWeatherChange(WeatherChangeCb cb) { m_impl->changeCbs.push_back(std::move(cb)); }
void WeatherSystem::OnTick(WeatherTickCb cb)            { m_impl->tickCbs.push_back(std::move(cb)); }

std::vector<WeatherPreset> WeatherSystem::BuiltinPresets() {
    std::vector<WeatherPreset> out;
    auto add = [&](const char* id, const char* name, float cloud, float rain,
                   float snow, float fog, float wind, float temp, float vis) {
        WeatherPreset p;
        p.id = id; p.displayName = name;
        p.state.cloudCover = cloud; p.state.rainIntensity = rain;
        p.state.snowIntensity = snow; p.state.fogDensity = fog;
        p.state.windSpeed = wind; p.state.temperature = temp;
        p.state.visibilityMetres = vis;
        p.weight = 1.0f;
        out.push_back(p);
    };
    add("clear",   "Clear",       0.05f, 0.0f, 0.0f, 0.0f,  2.0f, 20.0f, 10000.0f);
    add("cloudy",  "Cloudy",      0.7f,  0.0f, 0.0f, 0.05f, 5.0f, 15.0f,  8000.0f);
    add("rain",    "Rain",        0.9f,  0.6f, 0.0f, 0.1f,  8.0f, 12.0f,  3000.0f);
    add("storm",   "Storm",       1.0f,  0.9f, 0.0f, 0.2f, 18.0f,  8.0f,  1000.0f);
    add("fog",     "Foggy",       0.8f,  0.1f, 0.0f, 0.85f, 1.0f, 10.0f,   200.0f);
    add("snow",    "Snowfall",    0.8f,  0.0f, 0.7f, 0.15f, 6.0f, -3.0f,  4000.0f);
    add("blizzard","Blizzard",    1.0f,  0.0f, 1.0f, 0.5f, 20.0f, -8.0f,   500.0f);
    return out;
}

} // namespace PCG
