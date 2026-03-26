#pragma once
/**
 * @file UniverseData.h
 * @brief Data definitions for the NovaForge universe.
 *
 * These C++ structures mirror the JSON schemas in Projects/NovaForge/Data/.
 * The UniverseLoader populates ECS entities from these structures.
 *
 * Faction design (from NovaForge Design Bible):
 *   Solari   — armor tank, energy weapons, golden/elegant
 *   Veyren   — shield tank, hybrid turrets, angular/utilitarian
 *   Aurelian — speed+drones+ECM, sleek/organic
 *   Keldari  — missiles+shields+ECM, rugged/industrial
 */

#include <string>
#include <vector>
#include <cstdint>

namespace Runtime::Universe {

// ── Faction ──────────────────────────────────────────────────────────────────

enum class FactionId : uint8_t {
    None = 0,
    Solari,
    Veyren,
    Aurelian,
    Keldari,
    VoidSyndicate,
    COUNT
};

struct FactionDef {
    FactionId   id;
    std::string name;
    std::string description;
    std::string specialty;          ///< comma-separated specialties
    float       colorR, colorG, colorB;
    int         startingRep;        ///< player starting reputation (−1000..+1000)
    std::string capitalSystem;      ///< star system id
};

// ── Planet ───────────────────────────────────────────────────────────────────

enum class PlanetType : uint8_t {
    Rocky, Habitable, GasGiant, IceGiant, Barren, Lava, Ocean, Rogue
};

struct PlanetDef {
    std::string name;
    PlanetType  type       = PlanetType::Rocky;
    float       orbitAU    = 1.f;   ///< orbital radius in AU (1 AU = 500 world units)
    bool        landable   = false; ///< can the player land on this planet?
    float       radius     = 1.f;   ///< visual radius scale (relative to base size)
};

// ── Asteroid Belt ────────────────────────────────────────────────────────────

struct BeltDef {
    std::string              id;
    std::string              name;
    std::vector<std::string> oreTypes;  ///< ore type IDs
};

// ── Station ──────────────────────────────────────────────────────────────────

struct StationDef {
    std::string              id;
    std::string              name;
    std::string              type;       ///< "Station", "Outpost", "JumpGate"
    std::vector<std::string> services;  ///< "market","repair","fitting","missions","cloning"
};

// ── Star ─────────────────────────────────────────────────────────────────────

struct StarDef {
    std::string name;
    std::string spectralClass;  ///< "G2V", "K4V", etc.
    float       colorR = 1.f, colorG = 0.9f, colorB = 0.6f;
    float       radius = 4.f;
};

// ── Star System ──────────────────────────────────────────────────────────────

enum class SecurityLevel : uint8_t { HighSec, LowSec, NullSec, WormholeSpace };

struct SystemDef {
    std::string              id;
    std::string              name;
    std::string              description;
    float                    security    = 1.f;   ///< 0.0 null .. 1.0 perfectly safe
    FactionId                faction     = FactionId::None;
    SecurityLevel            secLevel    = SecurityLevel::HighSec;
    StarDef                  star;
    std::vector<PlanetDef>   planets;
    std::vector<StationDef>  stations;
    std::vector<BeltDef>     belts;
    std::vector<std::string> gates;      ///< connected system IDs
};

// ── Gas Type ─────────────────────────────────────────────────────────────────

struct GasTypeDef {
    std::string              id;
    std::string              name;
    std::string              description;
    std::string              rarity;     ///< "common","uncommon","rare","very_rare"
    int                      basePrice   = 0;
    std::string              source;     ///< "gas_giant","gas_giant_lowsec","wormhole"
    std::vector<std::string> products;
};

// ── Universe Registry ────────────────────────────────────────────────────────

/// All static universe data for the current session.
struct UniverseRegistry {
    std::vector<FactionDef> factions;
    std::vector<SystemDef>  systems;
    std::vector<GasTypeDef> gasTypes;
};

/// Returns the built-in universe definitions (Thyrkstad region + 4 factions).
/// This is the compiled-in fallback; JSON loading overlays on top.
UniverseRegistry BuiltinUniverse();

} // namespace Runtime::Universe
