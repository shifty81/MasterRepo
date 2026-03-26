# NovaForge — Design Bible

> Canonical reference for all game systems, extracted from the Master Design Bible
> (standalone shifty81/NovaForge) and consolidated for the MasterRepo.
> **Authoritative source:** `shifty81/NovaForge/docs/design/MASTER_DESIGN_BIBLE.md`

---

## Design Philosophy

### Core Pillars
- **Interior-driven ship construction** — hull shapes derived from interior modules
- **AI and player parity** — if AI can build it, player can build it using the same system
- **Fighter → Titan scale** — one unified system from smallest craft to world-ships
- **Persistent universe** — damage, wrecks, and history never reset
- **PVE co-op focus** — all challenge from physics, AI, and environment
- **Data-driven everything** — JSON/data tables for items, stats, behaviors, seeds
- **Mod-first architecture** — content grows without code changes
- **Deterministic simulation** — same seed always produces identical results

### The Four Factions

| Faction | Style | Specialty | Capital |
|---------|-------|-----------|---------|
| **Solari Dominion** | Golden / elegant | Armor tanking, energy weapons | Solari Prime |
| **Veyren Collective** | Angular / utilitarian | Shield tanking, hybrid turrets | Thyrkstad |
| **Aurelian Enclave** | Sleek / organic | Speed, drones, ECM | Aurendis |
| **Keldari Union** | Rugged / industrial | Missiles, shields, ECM | Duskfall |

Player starts in **Thyrkstad** (Veyren high-sec space, security 1.0).

---

## Ship Construction System

Ships are **module graphs**, not meshes. The hull is derived from the interior.

### Module Size Classes
`XS` → `S` → `M` → `L` → `XL` → `XXL` → `XXXL` → `TITAN`

### Ship Classes
Frigate · Destroyer · Cruiser · Battlecruiser · Battleship · Capital · Titan

### Interior → Exterior Mapping
| Interior Module | Exterior Hull Cue |
|-----------------|-------------------|
| Belly Bay | Ramp doors, hull bulge |
| Drone Bay | Launch hatches, antennae |
| Rover Dock | Hull bulge, ramp slope |
| Player Rig Bay | Hull indentation, docking clamps |

---

## Planet Systems

### Landing
- **Landable planets** (Rocky/Habitable): approach within 50 world units → `[F]` prompt
- **Gas Giants / Ice Giants**: orbital anchor only → deploy harvest drones
- Planet surface uses same modular PCG as ship interiors

### Gas Giant Harvesting
Gas types harvested via orbital drones:

| Gas | Rarity | Price | Product |
|-----|--------|-------|---------|
| Nebuline-C50 | Common | 200 | Polymer materials |
| Nebuline-C72 | Uncommon | 1,500 | Advanced polymers |
| Vaporin | Uncommon | 5,000 | Combat boosters |
| Mistocin | Common | 2,000 | Synth boosters |
| Helion-3 | Rare | 12,000 | Fusion fuel |

---

## Player Rig System

The rig = power armor frame/backpack. 13 module types:
`LifeSupport · PowerCore · JetpackTank · Sensor · Shield · EnvironFilter ·
ToolMount · WeaponMount · DroneController · ScannerSuite · CargoPod · BatteryPack · SolarPanel`

---

## Damage & Cascading Failure
- Each module tracks: HP, power state, connectivity
- Destroying a module removes its function
- May isolate other modules (graph disconnection)
- No scripted events — pure system behavior

| Intent | Target |
|--------|--------|
| Cripple | Engines |
| Disarm | Weapons |
| Blind | Sensors |
| Break | Spines |

---

## Low-Poly Visual System (Surrounded Aesthetic)
- Vertex-color driven shading
- Hard edge normals
- Limited light count per object
- Solid colors + gradients, palette-driven materials
- 10–20 materials max

---

## Related Files
- `Projects/NovaForge/Data/Universe/systems.json` — star system definitions
- `Projects/NovaForge/Data/factions.json` — faction definitions
- `Projects/NovaForge/Data/gas_types.json` — gas type catalog
- `Runtime/Universe/UniverseData.h` — C++ universe data structures
- `Runtime/Universe/UniverseLoader.h` — ECS world population
- `Runtime/Factions/FactionSystem.h` — faction reputation system
- `implement2.md`, `implement3.md` — brainstorming session archives
