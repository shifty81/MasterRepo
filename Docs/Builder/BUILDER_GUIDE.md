# Builder System Guide

The Atlas Builder is a **Starbase-inspired modular construction system**
that lets players (and the editor) assemble vehicles, structures, and machines
from discrete parts connected at snap points.

---

## Overview

The Builder consists of:

| Component | Location | Description |
|-----------|----------|-------------|
| Part definitions | `Builder/Parts/PartDef.h` | Static description of each part |
| Assembly | `Builder/Assembly/` | Graph of placed instances and welds |
| Snap system | `Builder/SnapSystem/` | Snap-point compatibility and pairing |
| Damage model | `Builder/Damage/` | Per-part HP, destruction cascade |
| Physics bridge | `Builder/PhysicsData/` | Mass, CoM, joint descriptors |
| Runtime builder | `Builder/RuntimeBuilder/` | In-game placement mode |

---

## Part Definition Format

Each part is described by a `Builder::PartDef` struct:

```cpp
struct PartDef {
    uint32_t         id;
    std::string      name;
    PartCategory     category;   // Hull, Weapon, Engine, Shield, Power …
    std::string      meshPath;
    std::vector<SnapPoint> snapPoints;
    PartStats        stats;      // mass, hitpoints, powerConsumption …
    // …
};
```

Parts are registered in a `Builder::PartLibrary`. Custom parts can be
contributed by `IBuilderPlugin` implementations.

---

## Snap Point System

A **snap point** is a named attachment socket on a part mesh:

```cpp
struct SnapPoint {
    uint32_t id;
    float    localPos[3];
    float    localNormal[3];
    std::vector<std::string> compatibleTypes;
    bool     occupied;
};
```

Two snap points are **compatible** when their `compatibleTypes` lists share at
least one token (e.g. `"hull_frame"`, `"turret_base"`). The snap system
rejects mismatched pairs automatically.

---

## Assembly Workflow

### In the Editor

1. Open the **Builder Editor** panel.
2. Select a part from the **Part Library**.
3. Hover over the existing assembly — valid snap points highlight in blue.
4. Click a snap point to attach the new part (green preview = valid).
5. Press `Enter` to confirm; `Esc` to cancel.
6. Use **Validate Assembly** to run the structural integrity check.

### Programmatically (C++)

```cpp
Builder::Assembly assembly;

uint32_t hullId = assembly.PlacePart(partLibrary.Find("hull_2x2"), Transform{});
uint32_t engId  = assembly.PlacePart(partLibrary.Find("engine_mk1"), Transform{});
assembly.Weld(hullId, 0 /*snapA*/, engId, 0 /*snapB*/);

auto result = assembly.Validate();
if (!result.valid)
    for (const auto& err : result.errors)
        std::cerr << err << "\n";
```

---

## Damage System

Each placed instance has an HP value derived from `PartStats::hitpoints`.
When a part's HP reaches zero:

1. The part is **destroyed** and removed from the assembly graph.
2. Any parts **solely connected** through the destroyed part become detached
   sub-assemblies (structural cascade).
3. Detached sub-assemblies become independent physics objects in the Runtime.

Damage events are forwarded through `Core::EventBus` as `Builder::PartDamagedEvent`
and `Builder::PartDestroyedEvent`.

---

## In-Game Runtime Building

The **RuntimeBuilder** system mirrors the editor workflow in live gameplay:

- The player selects a part from the HUD inventory.
- A ghost preview follows the cursor, snapping to valid attachment points.
- Confirming placement calls `Builder::Assembly::PlacePart` and synchronises
  the result with the ECS via `RuntimeBuilder::Commit()`.
- Parts placed in-game are subject to the same physics and damage model as
  editor-placed parts.

Enable runtime building by setting the `BuilderMode` CVar to `true` and
activating the `RuntimeBuilderSystem` in your `World` setup.
