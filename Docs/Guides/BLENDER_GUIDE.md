# Blender Integration Guide

> **Phase 13 — Blender Addon Integration (Deep)**
>
> This guide covers the complete bidirectional asset pipeline between Blender
> and the Atlas Engine, including mesh/scene export and import, rig and
> animation export, material conversion, batch processing, and procedural
> geometry generation.

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Installing the Addons](#installing-the-addons)
3. [Exporting Assets from Blender](#exporting-assets-from-blender)
   - [Export a Single Mesh](#export-a-single-mesh)
   - [Export a Scene](#export-a-scene)
   - [Export a Rig (Armature)](#export-a-rig-armature)
   - [Export an Animation](#export-an-animation)
   - [Export Materials](#export-materials)
   - [Batch Export](#batch-export)
4. [Importing Atlas Assets into Blender](#importing-atlas-assets-into-blender)
   - [Import a Mesh](#import-a-mesh)
   - [Import a Scene](#import-a-scene)
5. [Procedural Geometry Generator](#procedural-geometry-generator)
   - [Hull Plate](#hull-plate)
   - [Corridor Tile](#corridor-tile)
   - [Pipe Segment](#pipe-segment)
   - [Modular Room](#modular-room)
6. [Material Converter](#material-converter)
   - [Interactive Conversion](#interactive-conversion)
   - [CLI Batch Conversion](#cli-batch-conversion)
7. [Atlas JSON Formats](#atlas-json-formats)
   - [Mesh JSON](#mesh-json)
   - [Scene JSON](#scene-json)
   - [Rig JSON](#rig-json)
   - [Animation JSON](#animation-json)
   - [Material JSON](#material-json)
8. [Engine-Side Import (C++)](#engine-side-import-c)

---

## Prerequisites

| Requirement | Version |
|-------------|---------|
| Blender | 3.6 LTS or newer |
| Python | 3.10+ (bundled with Blender) |
| Atlas Engine build | Any recent build |

---

## Installing the Addons

There are three Python addon files in `Scripts/Python/Blender/`:

| File | Purpose |
|------|---------|
| `atlas_blender_addon.py` | Main export/import panel (meshes, rigs, animations, materials, batch) |
| `atlas_procedural_gen.py` | Procedural geometry generator (hull plates, corridors, pipes, rooms) |
| `atlas_material_converter.py` | Batch material conversion to Atlas Engine JSON |

**To install:**

1. Open Blender → **Edit → Preferences → Add-ons → Install…**
2. Select the `.py` file.
3. Enable the addon by checking the box next to its name.
4. Repeat for each of the three addons.

All three addons add panels to **View3D → Sidebar → Atlas** (press `N` to toggle
the sidebar).

---

## Exporting Assets from Blender

All export operators are in the **Atlas Engine** panel under the **Atlas** sidebar tab.

### Export a Single Mesh

1. Select a mesh object in the viewport.
2. Click **Export Selected Mesh**.
3. Choose a file path; the addon appends `.json` if omitted.

The output is an **Atlas Mesh JSON** file (see [Mesh JSON](#mesh-json)).

### Export a Scene

1. Click **Export Scene** (no selection required).
2. All named mesh objects in the current scene are exported together into a
   single **Atlas Scene JSON** file.

### Export a Rig (Armature)

1. Select an **Armature** object.
2. Click **Export Rig (Armature)**.
3. Choose the output path.

The output is an **Atlas Rig JSON** containing bone hierarchy, rest positions,
and rotations (see [Rig JSON](#rig-json)).

### Export an Animation

1. Select the **Armature** that has the action you want to export.
2. Make sure the action is set as the **Active Action** in the *Action Editor*
   or *NLA Editor*.
3. Click **Export Animation (Action)**.

The output is an **Atlas Animation JSON** with per-bone, per-frame keyframe
data (see [Animation JSON](#animation-json)).

> **Tip:** To export multiple actions, use the [Batch Export](#batch-export)
> operator — it reads all NLA strips automatically.

### Export Materials

1. Select an object that has materials assigned.
2. Click **Export Materials**.
3. Choose the output path.

All materials on the object are converted from Principled BSDF nodes to the
flat Atlas material format and saved in a single JSON file.

### Batch Export

1. Select all objects you want to export (meshes and/or armatures).
2. Click **Batch Export Selected**.
3. Choose an **output directory** (not a file).
4. Toggle the checkboxes in the operator options panel (F9 after clicking):
   - **Export Meshes** — write `<name>.mesh.json` for each mesh
   - **Export Rigs** — write `<name>.rig.json` for each armature
   - **Export Animations** — write `<name>_<action>.anim.json` for each action
   - **Export Materials** — write `<name>.mat.json` for each material set

All files land in the chosen directory.

---

## Importing Atlas Assets into Blender

### Import a Mesh

1. Click **Import Atlas Mesh**.
2. Navigate to a `.json` file exported by the Atlas Engine.
3. The mesh is created at the origin and selected automatically.

### Import a Scene

1. Click **Import Atlas Scene**.
2. Navigate to a scene JSON file.
3. All mesh objects are created and linked into the current collection.

---

## Procedural Geometry Generator

The **Procedural** sub-panel (from `atlas_procedural_gen.py`) generates
parametric geometry primitives ready for engine export.

### Hull Plate

Generates a box with an optional chamfer bevel — the basic building block for
ship hull sections.

| Parameter | Default | Description |
|-----------|---------|-------------|
| Width | 2.0 m | X extent |
| Height | 1.0 m | Z extent |
| Depth | 0.1 m | Y extent (thickness) |
| Bevel | 0.02 m | Chamfer amount (0 = sharp) |
| Segments | 2 | Bevel loop cuts |

### Corridor Tile

A hollow rectangular tube open at both Y ends — use for modular corridor snapping.

| Parameter | Default |
|-----------|---------|
| Width | 3.0 m |
| Length | 4.0 m |
| Height | 2.5 m |
| Wall Thickness | 0.2 m |

### Pipe Segment

A cylinder with optional end caps for conduit / pipe networks.

| Parameter | Default |
|-----------|---------|
| Radius | 0.15 m |
| Length | 2.0 m |
| Segments | 12 |
| Cap Ends | false |

### Modular Room

A room box (no ceiling) with optional door openings on N/S/E/W faces.

| Parameter | Default |
|-----------|---------|
| Width / Depth | 6.0 m |
| Height | 3.0 m |
| Wall Thickness | 0.25 m |
| Door North/South/East/West | false |
| Door Width | 1.0 m |
| Door Height | 2.0 m |

---

## Material Converter

`atlas_material_converter.py` converts Blender's Principled BSDF node graph
to the Atlas Engine flat PBR material format.

### Interactive Conversion

From the **Materials** sub-panel in the Atlas sidebar:

- **Export Active Material** — convert and save the active object's material.
- **Export All Materials** — batch-convert every material in the `.blend` file
  into a chosen directory as `<name>.mat.json` files.

### CLI Batch Conversion

Run from the command line (useful for CI pipelines):

```bash
blender --background my_project.blend \
        --python Scripts/Python/Blender/atlas_material_converter.py \
        -- --output /path/to/output_dir
```

Options:

| Flag | Description |
|------|-------------|
| `--output DIR` | (required) output directory |
| `--no-overwrite` | skip already-converted files |

---

## Atlas JSON Formats

### Mesh JSON

```json
{
  "name": "hull_plate_01",
  "vertices": [0.0, 0.0, 0.0, ...],
  "normals":  [0.0, 1.0, 0.0, ...],
  "uvs":      [0.0, 0.0, ...],
  "indices":  [0, 1, 2, ...],
  "materials": ["hull_steel"]
}
```

Arrays are flat (3 floats per vertex for positions/normals, 2 floats per loop
for UVs).

### Scene JSON

```json
{
  "scene": "DockingBay",
  "objects": [ /* array of Mesh JSON objects */ ]
}
```

### Rig JSON

```json
{
  "name": "player_rig",
  "bones": [
    {
      "name": "root",
      "parent": "",
      "head": [0.0, 0.0, 0.0],
      "tail": [0.0, 0.0, 1.0],
      "rotation": [1.0, 0.0, 0.0, 0.0],
      "length": 1.0
    }
  ]
}
```

`rotation` is `[w, x, y, z]` quaternion in rest pose (local space).

### Animation JSON

```json
{
  "name": "walk_cycle",
  "fps": 24,
  "frame_start": 1,
  "frame_end": 24,
  "duration": 1.0,
  "channels": [
    {
      "bone": "hip",
      "curves": {
        "location": {
          "0": [{"frame": 1, "value": 0.0}, {"frame": 24, "value": 0.0}],
          "1": [{"frame": 1, "value": 0.0}],
          "2": [{"frame": 1, "value": 0.1}]
        }
      }
    }
  ]
}
```

Channel curve keys are the array index (0 = X, 1 = Y, 2 = Z; or WXYZ for
quaternions).

### Material JSON

```json
{
  "name": "hull_steel",
  "shader": "pbr_metallic_roughness",
  "base_color": [0.4, 0.4, 0.45, 1.0],
  "base_color_texture": "",
  "metallic": 0.9,
  "roughness": 0.3,
  "metallic_roughness_texture": "",
  "normal_map": "//textures/hull_normal.png",
  "normal_scale": 1.0,
  "emission": [0.0, 0.0, 0.0],
  "emission_strength": 0.0,
  "alpha": 1.0,
  "ior": 1.45,
  "double_sided": false,
  "alpha_mode": "OPAQUE"
}
```

---

## Engine-Side Import (C++)

The `Tools::AssetImporter` class handles `.json` files produced by the Blender
addon.  Supported extensions are detected automatically:

```cpp
#include "Tools/Importer/AssetImporter.h"

Tools::AssetImporter importer;
auto asset = importer.ImportFile("Assets/Meshes/hull_plate_01.mesh.json");
// asset.type == AssetType::Mesh
```

The `Tools::AssetProcessor` can post-process the imported asset
(optimise mesh, generate mipmaps, etc.):

```cpp
#include "Tools/AssetTools/AssetProcessor.h"

Tools::ProcessOptions opts;
opts.op = Tools::ProcessOp::OptimiseMesh;
auto result = Tools::AssetProcessor::Process(asset.outputPath, opts);
```

Documentation for all engine asset types is auto-generated by the
`Tools::DocGenerator` module — see `Docs/API/` after running a build.
