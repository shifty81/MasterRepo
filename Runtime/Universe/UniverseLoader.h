#pragma once
/**
 * @file UniverseLoader.h
 * @brief Populates the ECS world from UniverseData definitions.
 *
 * Each star, planet, belt, and station becomes an ECS entity with:
 *   - Tag component: name + appropriate type tags
 *   - Transform component: 3D position (AU-scaled, Y-up, Z-forward)
 *
 * Factions are registered with FactionSystem.
 *
 * Usage:
 * @code
 *   auto reg = Runtime::Universe::BuiltinUniverse();
 *   UniverseLoader loader(reg);
 *   loader.SetStartSystem("thyrkstad");
 *   loader.PopulateECS(world, factionSystem);
 * @endcode
 */

#include "Runtime/Universe/UniverseData.h"
#include "Runtime/ECS/ECS.h"
#include "Runtime/Components/Components.h"
#include "Runtime/Factions/FactionSystem.h"
#include <cmath>
#include <string>
#include <unordered_map>

namespace Runtime::Universe {

/// Scale factor: 1 AU → world units.  Generous so nearby planets are visible.
static constexpr float kAUtoWorld = 800.f;

/// Returns planet colour (r,g,b) based on planet type.
inline void PlanetColor(PlanetType t, float& r, float& g, float& b) {
    switch (t) {
        case PlanetType::Habitable: r=0.20f; g=0.55f; b=1.00f; return;
        case PlanetType::GasGiant:  r=0.90f; g=0.60f; b=0.25f; return;
        case PlanetType::IceGiant:  r=0.30f; g=0.70f; b=1.00f; return;
        case PlanetType::Lava:      r=1.00f; g=0.25f; b=0.05f; return;
        case PlanetType::Ocean:     r=0.10f; g=0.40f; b=0.90f; return;
        case PlanetType::Barren:    r=0.45f; g=0.42f; b=0.38f; return;
        case PlanetType::Rocky:     r=0.55f; g=0.50f; b=0.45f; return;
        default:                    r=0.50f; g=0.50f; b=0.50f; return;
    }
}

/// Maps PlanetType → tag strings.
inline std::vector<std::string> PlanetTags(PlanetType t, bool landable) {
    std::vector<std::string> tags{"CelestialBody"};
    switch (t) {
        case PlanetType::Habitable: tags.push_back("Planet"); tags.push_back("Habitable"); break;
        case PlanetType::GasGiant:  tags.push_back("Planet"); tags.push_back("GasGiant");  break;
        case PlanetType::IceGiant:  tags.push_back("Planet"); tags.push_back("IceGiant");  break;
        case PlanetType::Rocky:     tags.push_back("Planet"); tags.push_back("Rocky");      break;
        case PlanetType::Lava:      tags.push_back("Planet"); tags.push_back("Lava");        break;
        case PlanetType::Ocean:     tags.push_back("Planet"); tags.push_back("Ocean");       break;
        case PlanetType::Barren:    tags.push_back("Planet"); tags.push_back("Barren");      break;
        default:                    tags.push_back("Planet");                                break;
    }
    if (landable) tags.push_back("Landable");
    return tags;
}

class UniverseLoader {
public:
    explicit UniverseLoader(const UniverseRegistry& reg)
        : m_reg(reg) {}

    void SetStartSystem(const std::string& systemId) {
        m_startSystem = systemId;
    }

    /// Populate the ECS world with entities for the current star system.
    /// Also registers factions with FactionSystem (if provided).
    void PopulateECS(ECS::World& world, Runtime::FactionSystem* factions = nullptr) {
        // ── Register factions ────────────────────────────────────────────
        if (factions) {
            for (auto& fd : m_reg.factions) {
                Runtime::FactionAlignment align = Runtime::FactionAlignment::Neutral;
                if (fd.startingRep > 200)  align = Runtime::FactionAlignment::Friendly;
                if (fd.startingRep < -200) align = Runtime::FactionAlignment::Hostile;
                uint32_t fid = factions->CreateFaction(fd.name, align);
                if (fd.startingRep != 0)
                    factions->AddReputation(fid, (float)fd.startingRep, "initial_standing");
                m_factionMap[fd.name] = fid;
            }
        }

        // ── Find the start system ─────────────────────────────────────────
        const SystemDef* sys = nullptr;
        for (auto& s : m_reg.systems)
            if (s.id == m_startSystem) { sys = &s; break; }
        if (!sys && !m_reg.systems.empty()) sys = &m_reg.systems[0];
        if (!sys) return;

        // ── Star ─────────────────────────────────────────────────────────
        {
            auto e = world.CreateEntity();
            Runtime::Components::Tag t;
            t.name = sys->star.name;
            t.tags = {"Star", "Light", "CelestialBody"};
            world.AddComponent(e, t);
            Runtime::Components::Transform tr;
            tr.position = {0.f, 0.f, 0.f};
            world.AddComponent(e, tr);
        }

        // ── Planets ──────────────────────────────────────────────────────
        // Lay out planets along X axis, spacing by orbit_au
        for (size_t i = 0; i < sys->planets.size(); ++i) {
            const auto& pd = sys->planets[i];
            auto e = world.CreateEntity();
            Runtime::Components::Tag t;
            t.name = pd.name;
            t.tags = PlanetTags(pd.type, pd.landable);
            world.AddComponent(e, t);
            Runtime::Components::Transform tr;
            // Place along X axis at orbital distance; Y staggered slightly for visibility
            float dist = pd.orbitAU * kAUtoWorld;
            float angle = (float)i * 0.62f; // golden-angle spread
            tr.position = { dist * std::cos(angle), dist * 0.05f * std::sin(angle), dist * std::sin(angle) };
            world.AddComponent(e, tr);
        }

        // ── Stations ─────────────────────────────────────────────────────
        for (size_t i = 0; i < sys->stations.size(); ++i) {
            const auto& sd = sys->stations[i];
            auto e = world.CreateEntity();
            Runtime::Components::Tag t;
            t.name = sd.name;
            t.tags = {"Station", "Structure", "Dockable"};
            if (sd.type == "JumpGate") { t.tags.push_back("JumpGate"); t.tags.push_back("Warp"); }
            for (auto& svc : sd.services)
                if (svc == "cloning") t.tags.push_back("Cloning");
            world.AddComponent(e, t);
            Runtime::Components::Transform tr;
            // Place station near the first habitable/third planet
            float stX = 0.f, stZ = 300.f + (float)i * 150.f;
            if (!sys->planets.empty()) {
                size_t idx = std::min(i + 2, sys->planets.size() - 1);
                float dist = sys->planets[idx].orbitAU * kAUtoWorld;
                float a = (float)idx * 0.62f;
                stX = dist * std::cos(a) + 50.f;
                stZ = dist * std::sin(a) + 80.f;
            }
            tr.position = {stX, 5.f + (float)i * 10.f, stZ};
            world.AddComponent(e, tr);
        }

        // ── Asteroid Belts ───────────────────────────────────────────────
        for (size_t i = 0; i < sys->belts.size(); ++i) {
            const auto& bd = sys->belts[i];
            auto e = world.CreateEntity();
            Runtime::Components::Tag t;
            t.name = bd.name;
            t.tags = {"AsteroidBelt", "PCG", "Mineable"};
            world.AddComponent(e, t);
            Runtime::Components::Transform tr;
            tr.position = {200.f + (float)i * 100.f, 20.f, 150.f};
            world.AddComponent(e, tr);
        }

        // ── Jump Gates ───────────────────────────────────────────────────
        for (size_t i = 0; i < sys->gates.size(); ++i) {
            auto e = world.CreateEntity();
            Runtime::Components::Tag t;
            t.name = "Gate \xe2\x86\x92 " + sys->gates[i];
            t.tags = {"JumpGate", "Structure", "Warp"};
            world.AddComponent(e, t);
            Runtime::Components::Transform tr;
            float angle = (float)i * 2.094f; // 120° apart
            float dist  = 1200.f + (float)i * 200.f;
            tr.position = {dist * std::cos(angle), 0.f, dist * std::sin(angle)};
            world.AddComponent(e, tr);
        }
    }

    const std::unordered_map<std::string, uint32_t>& FactionMap() const { return m_factionMap; }

private:
    const UniverseRegistry&                  m_reg;
    std::string                              m_startSystem = "thyrkstad";
    std::unordered_map<std::string, uint32_t> m_factionMap;
};

} // namespace Runtime::Universe
