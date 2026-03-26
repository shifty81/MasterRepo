#include "Runtime/Universe/UniverseData.h"

namespace Runtime::Universe {

UniverseRegistry BuiltinUniverse() {
    UniverseRegistry reg;

    // ── Factions ─────────────────────────────────────────────────────────────
    reg.factions = {
        { FactionId::Solari,       "Solari Dominion",  "Armor tanking, energy weapons, golden/elegant",
          "armor_tank,energy_weapons",    1.f, 0.85f, 0.20f,    0, "solari_prime" },
        { FactionId::Veyren,       "Veyren Collective","Shield tanking, hybrid turrets, angular/utilitarian",
          "shield_tank,hybrid_turrets",   0.3f, 0.70f, 1.00f,  50, "thyrkstad" },
        { FactionId::Aurelian,     "Aurelian Enclave", "Speed, drones, ECM, sleek/organic",
          "speed,drones,ecm",             0.2f, 1.00f, 0.60f,   0, "aurendis" },
        { FactionId::Keldari,      "Keldari Union",    "Missiles, shields, ECM, rugged/industrial",
          "missiles,shields,ecm",         1.0f, 0.40f, 0.20f,-100, "duskfall" },
        { FactionId::VoidSyndicate,"Void Syndicate",   "Pirate organization \xe2\x80\x94 ambush and electronic warfare",
          "ambush,ecm",                   0.6f, 0.10f, 0.80f,-500, "" },
    };

    // ── Gas Types ─────────────────────────────────────────────────────────────
    reg.gasTypes = {
        { "nebuline_c50","Nebuline-C50","Common gas giant atmosphere gas","common",     200,"gas_giant",{"polymer_reaction_materials"} },
        { "nebuline_c72","Nebuline-C72","Moderately rare gas, advanced polymers","uncommon",1500,"gas_giant",{"polymer_reaction_materials","advanced_polymers"} },
        { "vaporin",     "Vaporin",     "Booster gas from lowsec gas giants","uncommon",5000,"gas_giant_lowsec",{"combat_booster_materials"} },
        { "mistocin",    "Mistocin",    "Common booster gas, outer system","common",    2000,"gas_giant",{"synth_booster_materials"} },
        { "helion_3",    "Helion-3",    "Exotic fusion fuel, deep gas giant","rare",   12000,"gas_giant_deep",{"fusion_fuel","exotic_fuel"} },
    };

    // ── Thyrkstad System (starter — Veyren high-sec) ──────────────────────────
    {
        SystemDef s;
        s.id = "thyrkstad"; s.name = "Thyrkstad"; s.security = 1.0f;
        s.faction = FactionId::Veyren; s.secLevel = SecurityLevel::HighSec;
        s.description = "Major trade hub in Veyren space. Safe starting system.";
        s.star = {"Thyrkstad Prime","G2V", 1.0f,0.92f,0.50f, 4.0f};
        s.planets = {
            {"Thyrkstad I",   PlanetType::Rocky,    0.4f, false, 0.4f},
            {"Thyrkstad II",  PlanetType::Rocky,    0.8f, true,  0.5f},
            {"Thyrkstad III", PlanetType::Habitable, 1.2f, true,  1.0f},
            {"Thyrkstad IV",  PlanetType::GasGiant, 4.5f, false, 2.8f},
            {"Thyrkstad V",   PlanetType::IceGiant, 9.0f, false, 2.0f},
        };
        s.stations = {
            {"thyrkstad_4_4","Thyrkstad IV - Moon 4 - Veyren Fleet Assembly Yard","Station",
             {"market","repair","fitting","missions","cloning"}},
        };
        s.belts = {
            {"belt_alpha","Thyrkstad Belt Alpha",{"dustite","ferrite","ignaite"}},
        };
        s.gates = {"solari_prime","duskfall"};
        reg.systems.push_back(s);
    }

    // ── Solari Prime (Solari capital) ─────────────────────────────────────────
    {
        SystemDef s;
        s.id = "solari_prime"; s.name = "Solari Prime"; s.security = 1.0f;
        s.faction = FactionId::Solari; s.secLevel = SecurityLevel::HighSec;
        s.description = "Capital of the Solari Dominion. Throne world of the largest empire.";
        s.star = {"Auris","F5V", 1.0f,0.98f,0.80f, 5.0f};
        s.planets = {
            {"Auris I",   PlanetType::Rocky,    0.3f, false, 0.3f},
            {"Auris II",  PlanetType::Rocky,    0.7f, false, 0.4f},
            {"Solari Magna",PlanetType::Habitable,1.1f, true,  1.2f},
            {"Auris IV",  PlanetType::GasGiant, 5.2f, false, 3.0f},
            {"Auris V",   PlanetType::GasGiant, 9.5f, false, 2.5f},
        };
        s.stations = {
            {"solari_8","Solari VIII - Emperor Family Academy","Station",
             {"market","repair","fitting","missions","cloning","insurance"}},
        };
        s.gates = {"thyrkstad","aurendis"};
        reg.systems.push_back(s);
    }

    // ── Duskfall (Keldari low-sec) ────────────────────────────────────────────
    {
        SystemDef s;
        s.id = "duskfall"; s.name = "Duskfall"; s.security = 0.4f;
        s.faction = FactionId::Keldari; s.secLevel = SecurityLevel::LowSec;
        s.description = "Notorious lowsec chokepoint \xe2\x80\x94 pirate activity and rich ore belts.";
        s.star = {"Dusk","K4V", 1.0f,0.60f,0.20f, 3.0f};
        s.planets = {
            {"Dusk I",  PlanetType::Rocky,   0.5f, false, 0.4f},
            {"Dusk II", PlanetType::Rocky,   1.0f, false, 0.5f},
            {"Dusk III",PlanetType::GasGiant,3.5f, false, 2.5f},
        };
        s.stations = {
            {"duskfall_station","Duskfall V - Keldari Mining Station","Station",
             {"repair","fitting"}},
        };
        s.belts = {
            {"belt_rich","Duskfall Rich Belt",{"corite","crystite","nocxium_ore"}},
        };
        s.gates = {"thyrkstad"};
        reg.systems.push_back(s);
    }

    // ── Aurendis (Aurelian high-sec) ──────────────────────────────────────────
    {
        SystemDef s;
        s.id = "aurendis"; s.name = "Aurendis"; s.security = 0.9f;
        s.faction = FactionId::Aurelian; s.secLevel = SecurityLevel::HighSec;
        s.description = "Major Aurelian trade hub. Home of the Grand Assembly Plant.";
        s.star = {"Aura","G8V", 1.0f,0.88f,0.55f, 4.0f};
        s.planets = {
            {"Aura I",  PlanetType::Rocky,    0.5f, false, 0.4f},
            {"Aura II", PlanetType::Habitable, 1.3f, true,  0.9f},
            {"Aura III",PlanetType::GasGiant, 4.8f, false, 2.6f},
        };
        s.stations = {
            {"aurendis_9_20","Aurendis IX - Moon 20 - Aurelian Grand Assembly Plant","Station",
             {"market","repair","fitting","missions","cloning","insurance"}},
        };
        s.belts = {
            {"belt_1","Aurendis Mining Belt",{"dustite","ferrite"}},
        };
        s.gates = {"solari_prime"};
        reg.systems.push_back(s);
    }

    return reg;
}

} // namespace Runtime::Universe
